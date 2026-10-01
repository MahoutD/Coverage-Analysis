#pragma once

#include "coverage/models.h"
#include "coverage/instrumenter.h"
#include <QString>
#include <QObject>
#include <QSet>
#include <QMap>
#include <QSharedPointer>

namespace Coverage {

/**
 * @brief 覆盖率度量分析与仿真执行引擎
 * 负责接收并解析运行期轨迹打桩数据 (Trace Data)，如 UART/JTAG/控制台日志，
 * 计算语句覆盖率、判定/分支覆盖率及函数覆盖率，并支持基于 Python 激励向量进行虚拟驱动仿真。
 */
class CoverageEngine : public QObject {
    Q_OBJECT
public:
    explicit CoverageEngine(QObject* parent = nullptr);
    ~CoverageEngine() override = default;

    /**
     * @brief 设置当前分析文件插桩所建立的探针元数据
     */
    void setMetadata(const InstrumentationMetadata& metadata);
    const InstrumentationMetadata& metadata() const { return m_metadata; }

    /**
     * @brief 解析单行覆盖轨迹日志 (如 "[COV] L:1:42", "[COV] B:3:T", "[COV] F:2")
     */
    void ingestTraceLine(const QString& line);

    /**
     * @brief 批量解析完整的覆盖率轨迹日志文本
     */
    void ingestTraceContent(const QString& fullTrace);

    /**
     * @brief 清空当前所有命中计数器
     */
    void resetHits();

    /**
     * @brief 根据探针命中情况与原始 C 源码比对，生成最终覆盖率综合报告
     */
    CoverageReport generateReport(const QString& originalSourceCode) const;

    /**
     * @brief 虚拟嵌入式执行仿真：将时序激励向量喂入仿真驱动内核，动态驱动分支判定并实时统计覆盖
     * @param sourceCode 原始 C 源码
     * @param stimulus 注入的激励计划
     * @param runLog 仿真日志输出
     * @param outReport 生成的覆盖率报告
     * @return 仿真是否正常执行完毕
     */
    bool runSimulationWithStimulus(const QString& sourceCode,
                                   const StimulusPlan& stimulus,
                                   QString& runLog,
                                   CoverageReport& outReport);

signals:
    void hitRecorded(int line, LineCoverageStatus status);
    void simulationProgress(int step, int totalSteps);
    void simulationFinished(bool success, const QString& message);

private:
    InstrumentationMetadata m_metadata;    ///< 插桩元数据
    QMap<int, qint64> m_lineHits;          ///< 源码行号 -> 命中执行次数
    QMap<int, qint64> m_branchTrueHits;    ///< 分支探针 ID -> True 触发次数
    QMap<int, qint64> m_branchFalseHits;   ///< 分支探针 ID -> False 触发次数
    QMap<int, qint64> m_functionHits;      ///< 函数探针 ID -> 命中调用次数
};

} // namespace Coverage
