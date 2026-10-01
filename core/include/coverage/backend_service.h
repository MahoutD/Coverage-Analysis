#pragma once

#include "coverage/types.h"
#include "coverage/models.h"
#include "coverage/static_analyzer.h"
#include "coverage/instrumenter.h"
#include "coverage/coverage_engine.h"
#include "coverage/stimulus_engine.h"
#include "coverage/graph_generator.h"

#include <QObject>
#include <QString>
#include <QSharedPointer>

namespace Coverage {

/**
 * @brief 后端中枢门面服务 (BackendService)
 * 践行“前后端分离”原则的核心控制器单例。
 * 封装静态分析、源码插桩、覆盖度量计算、Python激励执行与 Graphviz 拓扑分析，
 * 向前端 (GUI 或 CLI) 提供统一的异步槽函数与状态变更信号。
 */
class BackendService : public QObject {
    Q_OBJECT
public:
    static BackendService& instance();

    explicit BackendService(QObject* parent = nullptr);
    ~BackendService() override = default;

    // 子引擎访问接口
    StaticAnalyzer& staticAnalyzer() { return m_staticAnalyzer; }
    const StaticAnalyzer& staticAnalyzer() const { return m_staticAnalyzer; }

    StimulusEngine& stimulusEngine() { return m_stimulusEngine; }
    const StimulusEngine& stimulusEngine() const { return m_stimulusEngine; }

    CoverageEngine& coverageEngine() { return m_coverageEngine; }
    const CoverageEngine& coverageEngine() const { return m_coverageEngine; }

    // 当前项目与活跃数据访问接口
    const StaticAnalysisReport& latestStaticReport() const { return m_latestStaticReport; }
    const CoverageReport& latestCoverageReport() const { return m_latestCoverageReport; }
    const StimulusPlan& latestStimulusPlan() const { return m_latestStimulusPlan; }
    const InstrumentationMetadata& latestMetadata() const { return m_latestMetadata; }
    const QString& currentSourceFile() const { return m_currentSourceFile; }
    const QString& currentSourceContent() const { return m_currentSourceContent; }

    // Graphviz 代码图生成接口
    QString generateCallGraphDot() const;
    QString generateFunctionCfgDot(const QString& funcName) const;
    QString renderGraphToSvg(const QString& dotContent, QString* outError = nullptr) const;

    // 统一日志发射并持久化至磁盘日志系统
    void postLog(LogLevel level, const QString& module, const QString& message);

public slots:
    // 核心工作流槽函数 (由前端界面或 CLI 触发)
    void loadSourceFile(const QString& filePath);
    void setSourceContent(const QString& content, const QString& fileName = QStringLiteral("buffer.c"));
    
    void runStaticAnalysis();
    void runInstrumentation(const QString& outputPath = QString());
    
    // Python 激励执行 (模式1: 执行指定文件)
    void runStimulusScript(const QString& scriptPath, const QStringList& args = QStringList());
    
    // Python 激励执行 (模式2: 直接执行在输入框中编写的代码，并具备前置语法校验)
    bool runStimulusCodeContent(const QString& pythonCode, const QStringList& args = QStringList(), QString* outSyntaxError = nullptr);

    void runSimulationWithLatestStimulus();
    void ingestTraceData(const QString& traceContent);

    void exportReportToJson(const QString& outputJsonPath);

signals:
    // 通知前端状态变迁的信号
    void sourceFileLoaded(const QString& filePath, const QString& content);
    void staticAnalysisStarted();
    void staticAnalysisFinished(const Coverage::StaticAnalysisReport& report);
    void instrumentationFinished(bool success, const Coverage::InstrumentationMetadata& meta, const QString& outputPath);
    void stimulusScriptStarted(const QString& scriptPath);
    void stimulusScriptOutput(const QString& output);
    void stimulusPlanReady(const Coverage::StimulusPlan& plan);
    void coverageReportUpdated(const Coverage::CoverageReport& report);
    void progressUpdated(int percent, const QString& statusMessage);
    void logMessage(Coverage::LogLevel level, const QString& module, const QString& message);

private slots:
    void onStimulusOutput(const QString& text);
    void onStimulusError(const QString& errorText);
    void onStimulusPlanLoaded(const Coverage::StimulusPlan& plan);
    void onStimulusFinished(int exitCode, bool success);

private:
    StaticAnalyzer m_staticAnalyzer;
    Instrumenter m_instrumenter;
    CoverageEngine m_coverageEngine;
    StimulusEngine m_stimulusEngine;

    QString m_currentSourceFile;
    QString m_currentSourceContent;
    StaticAnalysisReport m_latestStaticReport;
    CoverageReport m_latestCoverageReport;
    StimulusPlan m_latestStimulusPlan;
    InstrumentationMetadata m_latestMetadata;
};

} // namespace Coverage
