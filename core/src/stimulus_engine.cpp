#include "coverage/stimulus_engine.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

namespace Coverage {

StimulusEngine::StimulusEngine(QObject* parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    // 自动寻找并初始化 Python 解释器路径
    m_pythonExecutable = autoDetectPython();

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &StimulusEngine::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &StimulusEngine::onProcessReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &StimulusEngine::onProcessFinished);
}

StimulusEngine::~StimulusEngine() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void StimulusEngine::setPythonExecutable(const QString& pythonPath) {
    m_pythonExecutable = pythonPath;
}

QString StimulusEngine::pythonExecutable() const {
    return m_pythonExecutable;
}

QString StimulusEngine::autoDetectPython() {
    // 0. 优先使用软件安装包内置打包的独立便携式 Python 运行环境
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList bundledCandidates = {
        appDir + QStringLiteral("/python/python.exe"),
        appDir + QStringLiteral("/../tools/python/python.exe"),
        QDir::currentPath() + QStringLiteral("/tools/python/python.exe"),
        QDir::currentPath() + QStringLiteral("/bin/python/python.exe"),
        QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis/tools/python/python.exe")
    };
    for (const QString& cand : bundledCandidates) {
        if (QFile::exists(cand)) {
            return QDir::toNativeSeparators(cand);
        }
    }

    // 1. 检查 uv 管理的 Python 独立环境
    QString uvPythonPath = QDir::homePath() + QStringLiteral("/AppData/Roaming/uv/python/cpython-3.11.16-windows-x86_64-none/python.exe");
    if (QFile::exists(uvPythonPath)) {
        return QDir::toNativeSeparators(uvPythonPath);
    }

    // 2. 检索其他版本的 uv Python
    QDir uvDir(QDir::homePath() + QStringLiteral("/AppData/Roaming/uv/python"));
    if (uvDir.exists()) {
        QStringList entries = uvDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& d : entries) {
            QString cand = uvDir.filePath(d) + QStringLiteral("/python.exe");
            if (QFile::exists(cand)) {
                return QDir::toNativeSeparators(cand);
            }
        }
    }

    // 3. 检索 Windows 用户默认安装路径
    QDir localPythonDir(QDir::homePath() + QStringLiteral("/AppData/Local/Programs/Python"));
    if (localPythonDir.exists()) {
        QStringList entries = localPythonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& d : entries) {
            QString cand = localPythonDir.filePath(d) + QStringLiteral("/python.exe");
            if (QFile::exists(cand)) {
                return QDir::toNativeSeparators(cand);
            }
        }
    }

    // 4. 回退到环境变量 PATH 中的 python
    return QStringLiteral("python");
}

bool StimulusEngine::checkPythonSyntax(const QString& pythonCode,
                                       QString* outErrorMessage,
                                       int* outErrorLine,
                                       int* outErrorCol)
{
    if (outErrorLine) *outErrorLine = 0;
    if (outErrorCol) *outErrorCol = 0;

    if (pythonCode.trimmed().isEmpty()) {
        if (outErrorMessage) *outErrorMessage = QStringLiteral("Python 代码内容为空。");
        return false;
    }

    // 通过 ast.parse 进行完整的语法树分析，并捕获 SyntaxError 报错行列号
    QProcess proc;
    QString pyOneLiner = QStringLiteral(
        "import ast, sys, json\n"
        "try:\n"
        "    code = sys.stdin.read()\n"
        "    ast.parse(code)\n"
        "    print(json.dumps({'valid': True}))\n"
        "except SyntaxError as e:\n"
        "    print(json.dumps({'valid': False, 'msg': e.msg, 'line': e.lineno, 'col': e.offset, 'text': e.text or ''}))\n"
        "except Exception as e:\n"
        "    print(json.dumps({'valid': False, 'msg': str(e), 'line': 0, 'col': 0, 'text': ''}))\n"
    );

    proc.start(m_pythonExecutable, QStringList() << QStringLiteral("-c") << pyOneLiner);
    if (!proc.waitForStarted(3000)) {
        if (outErrorMessage) {
            *outErrorMessage = QString("无法拉起 Python 解释器 (%1) 进行语法分析。").arg(m_pythonExecutable);
        }
        return false;
    }

    proc.write(pythonCode.toUtf8());
    proc.closeWriteChannel();

    if (!proc.waitForFinished(5000)) {
        proc.kill();
        if (outErrorMessage) *outErrorMessage = QStringLiteral("语法校验超时。");
        return false;
    }

    QByteArray out = proc.readAllStandardOutput().trimmed();
    QJsonDocument doc = QJsonDocument::fromJson(out);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        bool valid = obj[QStringLiteral("valid")].toBool();
        if (valid) {
            return true;
        }

        QString msg = obj[QStringLiteral("msg")].toString();
        int line = obj[QStringLiteral("line")].toInt();
        int col = obj[QStringLiteral("col")].toInt();
        QString text = obj[QStringLiteral("text")].toString().trimmed();

        if (outErrorLine) *outErrorLine = line;
        if (outErrorCol) *outErrorCol = col;
        if (outErrorMessage) {
            *outErrorMessage = QString("语法错误 [第 %1 行, 第 %2 列]: %3%4")
                                   .arg(line).arg(col).arg(msg)
                                   .arg(text.isEmpty() ? QString() : QString(" (位于: \"%1\")").arg(text));
        }
        return false;
    }

    QString err = QString::fromUtf8(proc.readAllStandardError());
    if (outErrorMessage) *outErrorMessage = err.isEmpty() ? QStringLiteral("语法分析未知异常") : err;
    return false;
}

bool StimulusEngine::executeScript(const QString& scriptPath, const QStringList& args) {
    if (m_process->state() != QProcess::NotRunning) {
        cancelExecution();
    }

    m_currentScriptPath = scriptPath;
    m_accumulatedOutput.clear();
    m_accumulatedError.clear();

    QFileInfo fi(scriptPath);
    m_process->setWorkingDirectory(fi.absolutePath());

    QStringList fullArgs;
    fullArgs.append(scriptPath);
    fullArgs.append(args);

    emit executionStarted(scriptPath);
    m_process->start(m_pythonExecutable, fullArgs);

    return m_process->waitForStarted(3000);
}

bool StimulusEngine::executeScriptContent(const QString& pythonCode,
                                          const QStringList& args,
                                          QString* outCheckError)
{
    // 1. 严格语法语义校验
    int errLine = 0, errCol = 0;
    if (!checkPythonSyntax(pythonCode, outCheckError, &errLine, &errCol)) {
        return false;
    }

    // 2. 写入临时工作脚本并执行
    QString tempScriptPath = QDir::tempPath() + QDir::separator() + QStringLiteral("embedded_stimulus_interactive.py");
    QFile file(tempScriptPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (outCheckError) *outCheckError = QStringLiteral("无法创建交互式运行临时脚本文件。");
        return false;
    }

    QTextStream out(&file);
    out << pythonCode;
    file.close();

    return executeScript(tempScriptPath, args);
}

void StimulusEngine::cancelExecution() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

bool StimulusEngine::isRunning() const {
    return m_process && m_process->state() != QProcess::NotRunning;
}

void StimulusEngine::onProcessReadyReadStandardOutput() {
    QByteArray data = m_process->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    m_accumulatedOutput.append(text);
    emit executionOutput(text);
}

void StimulusEngine::onProcessReadyReadStandardError() {
    QByteArray data = m_process->readAllStandardError();
    QString text = QString::fromUtf8(data);
    m_accumulatedError.append(text);
    emit executionError(text);
}

void StimulusEngine::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);

    // 解析激励 JSON 输出数据
    QString errorMsg;
    StimulusPlan plan = parseStimulusJson(m_accumulatedOutput, &errorMsg);
    if (!plan.steps.isEmpty()) {
        if (plan.scriptPath.isEmpty()) {
            plan.scriptPath = m_currentScriptPath;
        }
        emit stimulusPlanLoaded(plan);
    }

    emit executionFinished(exitCode, success);
}

StimulusPlan StimulusEngine::parseStimulusJson(const QString& jsonString, QString* errorMsg) {
    QString raw = jsonString.trimmed();

    // 查找 [STIMULUS_START] 标记
    int startIdx = raw.indexOf(QStringLiteral("[STIMULUS_START]"));
    int endIdx = raw.indexOf(QStringLiteral("[STIMULUS_END]"));
    if (startIdx != -1 && endIdx != -1 && endIdx > startIdx) {
        startIdx += QStringLiteral("[STIMULUS_START]").length();
        raw = raw.mid(startIdx, endIdx - startIdx).trimmed();
    } else {
        int firstBrace = raw.indexOf(QLatin1Char('{'));
        int lastBrace = raw.lastIndexOf(QLatin1Char('}'));
        if (firstBrace != -1 && lastBrace != -1 && lastBrace > firstBrace) {
            raw = raw.mid(firstBrace, (lastBrace - firstBrace) + 1);
        }
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMsg) {
            *errorMsg = parseError.errorString();
        }
        return StimulusPlan();
    }

    return StimulusPlan::fromJson(doc.object());
}

StimulusPlan StimulusEngine::loadStimulusFile(const QString& filePath, QString* errorMsg) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = f.errorString();
        return StimulusPlan();
    }
    QString content = QString::fromUtf8(f.readAll());
    f.close();
    return parseStimulusJson(content, errorMsg);
}

bool StimulusEngine::saveStimulusFile(const QString& filePath, const StimulusPlan& plan, QString* errorMsg) {
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = f.errorString();
        return false;
    }
    QJsonDocument doc(plan.toJson());
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

} // namespace Coverage
