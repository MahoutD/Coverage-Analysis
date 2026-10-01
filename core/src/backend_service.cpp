#include "coverage/backend_service.h"
#include "coverage/logger.h"
#include "coverage/project.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace Coverage {

BackendService& BackendService::instance() {
    static BackendService s_instance;
    return s_instance;
}

BackendService::BackendService(QObject* parent)
    : QObject(parent)
    , m_coverageEngine(this)
    , m_stimulusEngine(this)
{
    // 连接 Python 激励引擎信号至门面服务
    connect(&m_stimulusEngine, &StimulusEngine::executionStarted,
            this, &BackendService::stimulusScriptStarted);
    connect(&m_stimulusEngine, &StimulusEngine::executionOutput,
            this, &BackendService::onStimulusOutput);
    connect(&m_stimulusEngine, &StimulusEngine::executionError,
            this, &BackendService::onStimulusError);
    connect(&m_stimulusEngine, &StimulusEngine::stimulusPlanLoaded,
            this, &BackendService::onStimulusPlanLoaded);
    connect(&m_stimulusEngine, &StimulusEngine::executionFinished,
            this, &BackendService::onStimulusFinished);
}

void BackendService::postLog(LogLevel level, const QString& module, const QString& message) {
    Logger::instance().log(level, module, message);
    emit logMessage(level, module, message);
}

void BackendService::loadSourceFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        postLog(LogLevel::Error, QStringLiteral("Backend"),
                        QStringLiteral("加载源文件失败: ") + filePath);
        return;
    }

    QTextStream in(&file);
    m_currentSourceContent = in.readAll();
    file.close();

    m_currentSourceFile = filePath;
    postLog(LogLevel::Info, QStringLiteral("Backend"),
                    QStringLiteral("成功加载嵌入式 C 源码文件: ") + filePath);

    // 内存中执行初次插桩建立探针元数据表
    auto instResult = m_instrumenter.instrumentSource(m_currentSourceContent, filePath);
    if (instResult.success) {
        m_latestMetadata = instResult.metadata;
        m_coverageEngine.setMetadata(m_latestMetadata);
        m_latestCoverageReport = m_coverageEngine.generateReport(m_currentSourceContent);
        m_latestCoverageReport.fileReports[QDir::cleanPath(filePath)] = m_latestCoverageReport;
    }

    emit sourceFileLoaded(filePath, m_currentSourceContent);
    emit coverageReportUpdated(m_latestCoverageReport);
}

void BackendService::setSourceContent(const QString& content, const QString& fileName) {
    m_currentSourceContent = content;
    m_currentSourceFile = fileName;

    auto instResult = m_instrumenter.instrumentSource(m_currentSourceContent, fileName);
    if (instResult.success) {
        m_latestMetadata = instResult.metadata;
        m_coverageEngine.setMetadata(m_latestMetadata);
        m_latestCoverageReport = m_coverageEngine.generateReport(m_currentSourceContent);
        m_latestCoverageReport.fileReports[QDir::cleanPath(fileName)] = m_latestCoverageReport;
        emit coverageReportUpdated(m_latestCoverageReport);
    }
}

void BackendService::runStaticAnalysis() {
    emit staticAnalysisStarted();
    emit progressUpdated(10, QStringLiteral("正在执行嵌入式静态安全规则分析..."));

    m_latestStaticReport = m_staticAnalyzer.analyzeSource(m_currentSourceContent, m_currentSourceFile);

    emit progressUpdated(100, QStringLiteral("静态分析完成。"));
    emit staticAnalysisFinished(m_latestStaticReport);

    postLog(LogLevel::Info, QStringLiteral("StaticAnalyzer"),
                    QString("静态分析完毕。共检出 %1 项缺陷 (%2 严重/错误, %3 警告)。")
                        .arg(m_latestStaticReport.issues.size())
                        .arg(m_latestStaticReport.errorCount())
                        .arg(m_latestStaticReport.warningCount()));
}

void BackendService::runInstrumentation(const QString& outputPath) {
    emit progressUpdated(20, QStringLiteral("正在执行嵌入式 C 代码探针自动插桩..."));

    auto result = m_instrumenter.instrumentFile(m_currentSourceFile, outputPath);
    if (result.success) {
        m_latestMetadata = result.metadata;
        m_coverageEngine.setMetadata(m_latestMetadata);
        m_latestCoverageReport = m_coverageEngine.generateReport(m_currentSourceContent);

        emit progressUpdated(100, QStringLiteral("源码插桩成功。"));
        emit instrumentationFinished(true, m_latestMetadata, result.metadata.instrumentedFilePath);
        emit coverageReportUpdated(m_latestCoverageReport);

        postLog(LogLevel::Info, QStringLiteral("Instrumenter"),
                        QString("插桩完成：注入 %1 个语句探针、%2 个判定分支探针、%3 个函数探针。")
                            .arg(m_latestMetadata.totalProbeLines)
                            .arg(m_latestMetadata.totalProbeBranches)
                            .arg(m_latestMetadata.totalProbeFunctions));
    } else {
        emit progressUpdated(100, QStringLiteral("源码插桩失败。"));
        emit instrumentationFinished(false, m_latestMetadata, QString());
        postLog(LogLevel::Error, QStringLiteral("Instrumenter"), result.errorMessage);
    }
}

void BackendService::runStimulusScript(const QString& scriptPath, const QStringList& args) {
    postLog(LogLevel::Info, QStringLiteral("StimulusEngine"),
                    QStringLiteral("启动执行外部 Python 激励生成脚本: ") + scriptPath);
    m_stimulusEngine.executeScript(scriptPath, args);
}

bool BackendService::runStimulusCodeContent(const QString& pythonCode, const QStringList& args, QString* outSyntaxError) {
    postLog(LogLevel::Info, QStringLiteral("StimulusEngine"),
                    QStringLiteral("正在校验输入框中编写的 Python 代码语法与语义..."));
    bool ok = m_stimulusEngine.executeScriptContent(pythonCode, args, outSyntaxError);
    if (!ok && outSyntaxError) {
        postLog(LogLevel::Error, QStringLiteral("StimulusEngine"), *outSyntaxError);
    }
    return ok;
}

QString BackendService::generateCallGraphDot() const {
    return GraphGenerator::generateCallGraphDot(m_latestStaticReport, m_currentSourceContent);
}

QString BackendService::generateFunctionCfgDot(const QString& funcName) const {
    for (const auto& f : m_latestStaticReport.functions) {
        if (f.name == funcName) {
            return GraphGenerator::generateFunctionCfgDot(f, m_currentSourceContent);
        }
    }
    if (!m_latestStaticReport.functions.isEmpty()) {
        return GraphGenerator::generateFunctionCfgDot(m_latestStaticReport.functions.first(), m_currentSourceContent);
    }
    return QStringLiteral("digraph Empty { bgcolor=\"#1e1e2e\"; node [fontcolor=\"white\"]; \"暂无函数数据\"; }");
}

QString BackendService::renderGraphToSvg(const QString& dotContent, QString* outError) const {
    return GraphGenerator::renderDotToSvg(dotContent, QString(), outError);
}

void BackendService::runSimulationWithLatestStimulus() {
    if (m_latestStimulusPlan.steps.isEmpty()) {
        postLog(LogLevel::Warning, QStringLiteral("CoverageEngine"),
                        QStringLiteral("当前暂无已生成的激励测试向量，无法执行仿真。"));
        return;
    }

    emit progressUpdated(30, QStringLiteral("正在将 Python 激励注入嵌入式仿真内核..."));

    QString log;
    CoverageReport rep;
    bool ok = m_coverageEngine.runSimulationWithStimulus(m_currentSourceContent, m_latestStimulusPlan, log, rep);

    if (ok) {
        rep.fileReports[QDir::cleanPath(m_currentSourceFile)] = rep;

        // 为工程内其他关联源文件分别生成独立覆盖分析报告 (彻底避免多源文件行号交叉污染)
        QSet<QString> otherFiles;
        if (ProjectManager::instance().hasActiveProject()) {
            const auto& proj = ProjectManager::instance().currentProject();
            for (const QString& sf : proj.sourceFiles) {
                QString c = QDir::cleanPath(sf);
                if (c != QDir::cleanPath(m_currentSourceFile) && QFile::exists(c)) {
                    otherFiles.insert(c);
                }
            }
            if (!proj.rootDir.isEmpty()) {
                QDir srcDir(proj.rootDir + QStringLiteral("/Source"));
                if (!srcDir.exists()) srcDir = QDir(proj.rootDir + QStringLiteral("/source"));
                if (srcDir.exists()) {
                    for (const auto& fi : srcDir.entryInfoList({QStringLiteral("*.c"), QStringLiteral("*.cpp")}, QDir::Files)) {
                        QString c = QDir::cleanPath(fi.absoluteFilePath());
                        if (c != QDir::cleanPath(m_currentSourceFile)) {
                            otherFiles.insert(c);
                        }
                    }
                }
            }
        }

        for (const QString& otherFile : otherFiles) {
            QFile of(otherFile);
            if (of.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = QString::fromUtf8(of.readAll());
                of.close();

                Instrumenter subInst;
                auto subRes = subInst.instrumentSource(content, otherFile);
                CoverageEngine subEngine;
                subEngine.setMetadata(subRes.metadata);

                CoverageReport subRep;
                QString subLog;
                subEngine.runSimulationWithStimulus(content, m_latestStimulusPlan, subLog, subRep);
                rep.fileReports[otherFile] = subRep;
            }
        }

        // 如果未执行静态分析，先快速提取函数列表
        if (m_latestStaticReport.functions.isEmpty() && !m_currentSourceContent.isEmpty()) {
            m_latestStaticReport = m_staticAnalyzer.analyzeSource(m_currentSourceContent, m_currentSourceFile);
        }

        // 构造各函数的覆盖度量明细 (用于覆盖统计大表及柱状图)
        int idx = 0;
        rep.functionCoverages.clear();
        for (const auto& f : m_latestStaticReport.functions) {
            FunctionCoverageInfo fc;
            fc.index = idx++;
            fc.functionName = f.name;
            fc.fileName = !f.filePath.isEmpty() ? f.filePath : m_currentSourceFile;
            fc.startLine = f.startLine;
            fc.endLine = f.endLine;
            fc.statementCount = qMax(1, f.statementCount);
            fc.branchCount = f.branchCount;

            int hitStmts = 0;
            int hitBranches = 0;
            int branchCountInFunc = 0;

            QString cleanFnFile = QDir::cleanPath(fc.fileName);
            const CoverageReport& targetRep = rep.fileReports.contains(cleanFnFile)
                                                ? rep.fileReports.value(cleanFnFile)
                                                : rep;

            for (int l = f.startLine; l <= f.endLine; ++l) {
                if (targetRep.lineDetails.contains(l)) {
                    const auto& ld = targetRep.lineDetails.value(l);
                    if (ld.hitCount > 0) {
                        hitStmts++;
                    }
                    for (const auto& bp : ld.branches) {
                        branchCountInFunc++;
                        if (bp.isFullyCovered() || bp.isPartiallyCovered()) {
                            hitBranches++;
                        }
                    }
                }
            }

            fc.coveredStatements = qMin(fc.statementCount, hitStmts);
            fc.statementCoveragePercent = (fc.statementCount > 0) ? (100.0 * fc.coveredStatements / fc.statementCount) : 100.0;
            if (fc.branchCount <= 0 && branchCountInFunc > 0) {
                fc.branchCount = branchCountInFunc;
            }
            fc.coveredBranches = qMin(fc.branchCount, hitBranches);
            fc.branchCoveragePercent = (fc.branchCount > 0) ? (100.0 * fc.coveredBranches / fc.branchCount) : 100.0;

            rep.functionCoverages.append(fc);
        }

        // 准确更新报告的全局函数总数与覆盖函数数
        rep.totalFunctions = rep.functionCoverages.size();
        int covFuncs = 0;
        int totStmts = 0;
        int covStmts = 0;
        int totBr = 0;
        int covBr = 0;
        for (const auto& fc : rep.functionCoverages) {
            if (fc.coveredStatements > 0) covFuncs++;
            totStmts += fc.statementCount;
            covStmts += fc.coveredStatements;
            totBr += fc.branchCount;
            covBr += fc.coveredBranches;
        }
        rep.coveredFunctions = covFuncs;
        if (rep.executableLines <= 0) {
            rep.executableLines = totStmts;
            rep.coveredLines = covStmts;
        }
        if (rep.totalBranches <= 0) {
            rep.totalBranches = totBr;
            rep.coveredBranches = covBr;
        }

        m_latestCoverageReport = rep;
        emit coverageReportUpdated(m_latestCoverageReport);
        emit progressUpdated(100, QStringLiteral("仿真结束，测试覆盖分析数据已同步就绪。"));
        postLog(LogLevel::Info, QStringLiteral("CoverageEngine"), log);
    }
}

void BackendService::ingestTraceData(const QString& traceContent) {
    m_coverageEngine.ingestTraceContent(traceContent);
    m_latestCoverageReport = m_coverageEngine.generateReport(m_currentSourceContent);
    m_latestCoverageReport.fileReports[QDir::cleanPath(m_currentSourceFile)] = m_latestCoverageReport;

    if (m_latestStaticReport.functions.isEmpty() && !m_currentSourceContent.isEmpty()) {
        m_latestStaticReport = m_staticAnalyzer.analyzeSource(m_currentSourceContent, m_currentSourceFile);
    }
    int idx = 0;
    m_latestCoverageReport.functionCoverages.clear();
    for (const auto& f : m_latestStaticReport.functions) {
        FunctionCoverageInfo fc;
        fc.index = idx++;
        fc.functionName = f.name;
        fc.fileName = !f.filePath.isEmpty() ? f.filePath : m_currentSourceFile;
        fc.startLine = f.startLine;
        fc.endLine = f.endLine;
        fc.statementCount = qMax(1, f.statementCount);
        fc.branchCount = f.branchCount;

        int hitStmts = 0;
        int hitBranches = 0;
        int branchCountInFunc = 0;

        QString cleanFnFile = QDir::cleanPath(fc.fileName);
        const CoverageReport& targetRep = m_latestCoverageReport.fileReports.contains(cleanFnFile)
                                            ? m_latestCoverageReport.fileReports.value(cleanFnFile)
                                            : m_latestCoverageReport;

        for (int l = f.startLine; l <= f.endLine; ++l) {
            if (targetRep.lineDetails.contains(l)) {
                const auto& ld = targetRep.lineDetails.value(l);
                if (ld.hitCount > 0) hitStmts++;
                for (const auto& bp : ld.branches) {
                    branchCountInFunc++;
                    if (bp.isFullyCovered() || bp.isPartiallyCovered()) hitBranches++;
                }
            }
        }
        fc.coveredStatements = qMin(fc.statementCount, hitStmts);
        fc.statementCoveragePercent = (fc.statementCount > 0) ? (100.0 * fc.coveredStatements / fc.statementCount) : 100.0;
        if (fc.branchCount <= 0 && branchCountInFunc > 0) fc.branchCount = branchCountInFunc;
        fc.coveredBranches = qMin(fc.branchCount, hitBranches);
        fc.branchCoveragePercent = (fc.branchCount > 0) ? (100.0 * fc.coveredBranches / fc.branchCount) : 100.0;

        m_latestCoverageReport.functionCoverages.append(fc);
    }

    // 准确更新报告的全局函数总数与覆盖函数数
    m_latestCoverageReport.totalFunctions = m_latestCoverageReport.functionCoverages.size();
    int covFuncs = 0;
    int totStmts = 0;
    int covStmts = 0;
    int totBr = 0;
    int covBr = 0;
    for (const auto& fc : m_latestCoverageReport.functionCoverages) {
        if (fc.coveredStatements > 0) covFuncs++;
        totStmts += fc.statementCount;
        covStmts += fc.coveredStatements;
        totBr += fc.branchCount;
        covBr += fc.coveredBranches;
    }
    m_latestCoverageReport.coveredFunctions = covFuncs;
    if (m_latestCoverageReport.executableLines <= 0) {
        m_latestCoverageReport.executableLines = totStmts;
        m_latestCoverageReport.coveredLines = covStmts;
    }
    if (m_latestCoverageReport.totalBranches <= 0) {
        m_latestCoverageReport.totalBranches = totBr;
        m_latestCoverageReport.coveredBranches = covBr;
    }

    emit coverageReportUpdated(m_latestCoverageReport);
    postLog(LogLevel::Info, QStringLiteral("CoverageEngine"),
                    QStringLiteral("已解析外部运行时覆盖轨迹数据，重新计算度量百分比。"));
}

void BackendService::exportReportToJson(const QString& outputJsonPath) {
    QJsonObject root;
    root[QStringLiteral("staticAnalysis")] = m_latestStaticReport.toJson();
    root[QStringLiteral("coverage")] = m_latestCoverageReport.toJson();
    root[QStringLiteral("stimulus")] = m_latestStimulusPlan.toJson();

    QFile file(outputJsonPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        postLog(LogLevel::Info, QStringLiteral("Backend"),
                        QStringLiteral("综合分析与覆盖报告已成功导出至: ") + outputJsonPath);
    }
}

void BackendService::onStimulusOutput(const QString& text) {
    emit stimulusScriptOutput(text);
}

void BackendService::onStimulusError(const QString& errorText) {
    postLog(LogLevel::Warning, QStringLiteral("Python"), errorText);
}

void BackendService::onStimulusPlanLoaded(const Coverage::StimulusPlan& plan) {
    m_latestStimulusPlan = plan;
    emit stimulusPlanReady(m_latestStimulusPlan);
    postLog(LogLevel::Info, QStringLiteral("StimulusEngine"),
                    QString("成功接收并解析 %1 组测试激励时序步。").arg(plan.steps.size()));

    // 自动触发覆盖仿真更新
    runSimulationWithLatestStimulus();
}

void BackendService::onStimulusFinished(int exitCode, bool success) {
    if (success) {
        postLog(LogLevel::Info, QStringLiteral("StimulusEngine"),
                        QStringLiteral("Python 激励生成进程执行完毕。"));
    } else {
        postLog(LogLevel::Error, QStringLiteral("StimulusEngine"),
                        QString("Python 激励进程异常退出，退出码: %1。").arg(exitCode));
    }
}

} // namespace Coverage
