#pragma once

#include "coverage/models.h"
#include <QObject>
#include <QString>
#include <QProcess>
#include <QStringList>

namespace Coverage {

/**
 * @brief Python 激励注入引擎
 * 负责管理 Python 解释器环境、执行外部 .py 激励生成脚本或直接执行在线编写的代码片段，
 * 并提供语法与语义检查功能，确保输入的 Python 代码可用。
 */
class StimulusEngine : public QObject {
    Q_OBJECT
public:
    explicit StimulusEngine(QObject* parent = nullptr);
    ~StimulusEngine() override;

    // Python 解释器管理
    void setPythonExecutable(const QString& pythonPath);
    QString pythonExecutable() const;
    static QString autoDetectPython();

    /**
     * @brief 对 Python 代码进行语法与语义合法性检查 (基于 Python ast 模块)
     * @param pythonCode 待检查的 Python 代码字符串
     * @param outErrorMessage 输出错误详细信息
     * @param outErrorLine 输出错误发生的行号 (1-based, 成功为 0)
     * @param outErrorCol 输出错误发生的列号 (1-based, 成功为 0)
     * @return 语法语义合法返回 true，存在语法错误或解释器不可用返回 false
     */
    bool checkPythonSyntax(const QString& pythonCode,
                           QString* outErrorMessage = nullptr,
                           int* outErrorLine = nullptr,
                           int* outErrorCol = nullptr);

    /**
     * @brief 异步执行磁盘上的 Python 脚本文件
     * @param scriptPath 脚本绝对路径
     * @param args 命令行参数
     * @return 是否成功启动进程
     */
    bool executeScript(const QString& scriptPath, const QStringList& args = QStringList());

    /**
     * @brief 异步执行直接在输入框中编写的 Python 代码内容 (先执行语法检查，自动写入临时文件后执行)
     * @param pythonCode 用户在界面编写的代码
     * @param args 运行参数
     * @param outCheckError 语法检查失败时的错误描述
     * @return 语法检查通过且成功拉起进程返回 true，否则返回 false
     */
    bool executeScriptContent(const QString& pythonCode,
                              const QStringList& args = QStringList(),
                              QString* outCheckError = nullptr);

    void cancelExecution();
    bool isRunning() const;

    // 解析结构化激励计划 (JSON)
    static StimulusPlan parseStimulusJson(const QString& jsonString, QString* errorMsg = nullptr);
    static StimulusPlan loadStimulusFile(const QString& filePath, QString* errorMsg = nullptr);
    static bool saveStimulusFile(const QString& filePath, const StimulusPlan& plan, QString* errorMsg = nullptr);

signals:
    void executionStarted(const QString& scriptPath);
    void executionOutput(const QString& text);
    void executionError(const QString& errorText);
    void executionFinished(int exitCode, bool success);
    void stimulusPlanLoaded(const Coverage::StimulusPlan& plan);

private slots:
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString m_pythonExecutable;
    QProcess* m_process = nullptr;
    QString m_accumulatedOutput;
    QString m_accumulatedError;
    QString m_currentScriptPath;
};

} // namespace Coverage
