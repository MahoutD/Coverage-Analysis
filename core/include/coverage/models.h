#pragma once

#include "coverage/types.h"
#include <QString>
#include <QVector>
#include <QMap>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace Coverage {

/**
 * @brief 静态分析检出的缺陷条目数据模型
 */
struct StaticIssue {
    QString id;                 ///< 缺陷唯一标识 (如 ISSUE-12-5)
    QString file;               ///< 所属源码文件路径
    int line = 0;               ///< 所在源码行号 (1-based)
    int column = 0;             ///< 所在源码列号 (1-based)
    QString ruleId;             ///< 规则标准编号 (如 MISRA-C-2012-Rule-21.3, EMB-VOLATILE-01)
    RuleCategory category = RuleCategory::MisraC; ///< 缺陷所属领域分类
    Severity severity = Severity::Warning;        ///< 严重等级
    QString message;            ///< 缺陷具体描述
    QString suggestion;         ///< 针对嵌入式安全编码的修复建议
    QString messageZh;          ///< 中文缺陷描述
    QString suggestionZh;       ///< 中文修复建议
    QString messageEn;          ///< 英文缺陷描述
    QString suggestionEn;       ///< 英文修复建议
    QString codeSnippet;        ///< 问题所在代码片段

    QString localizedMessage(bool isZh = true) const {
        if (isZh) return !messageZh.isEmpty() ? messageZh : message;
        return !messageEn.isEmpty() ? messageEn : message;
    }

    QString localizedSuggestion(bool isZh = true) const {
        if (isZh) return !suggestionZh.isEmpty() ? suggestionZh : suggestion;
        return !suggestionEn.isEmpty() ? suggestionEn : suggestion;
    }

    QJsonObject toJson() const;
    static StaticIssue fromJson(const QJsonObject& json);
};

/**
 * @brief 源代码结构与质量度量模型 (语句数、分支数、注释率、MC/DC 判定数等)
 */
struct CodeMetrics {
    QString filePath;
    int totalLines = 0;         ///< 物理总行数
    int statementCount = 0;     ///< 语句数 (SLOC / 赋值与控制语句)
    int branchCount = 0;        ///< 分支数 (分支判定点总数)
    int commentLines = 0;       ///< 注释行数
    double commentRatio = 0.0;  ///< 注释率 (%)
    int mcdcDecisions = 0;      ///< MC/DC 复合判定数
    int mcdcConditions = 0;     ///< MC/DC 原子条件总数
    int functionCount = 0;      ///< 函数总数
    int totalComplexity = 0;    ///< 圈复杂度总和
    int maxComplexity = 1;      ///< 单函数最大圈复杂度

    QJsonObject toJson() const;
    static CodeMetrics fromJson(const QJsonObject& json);
};

/**
 * @brief 函数执行路径模型 (用于路径遍历、Graphviz 高亮与源码着色)
 */
struct FunctionPath {
    int pathId = 1;             ///< 路径序号 (1, 2, 3...)
    QString name;               ///< 路径名称 (如: "路径 1: 正常工作主路径")
    QString conditionDescription; ///< 路径决策分支条件 (如: "D1: True -> D2: False")
    QVector<int> executedLines; ///< 该路径执行覆盖的代码行号列表 (用于源代码高亮着色)
    QStringList visitedNodeIds; ///< 该路径经过的 Graphviz CFG 节点 ID
    QVector<QPair<QString, QString>> visitedEdges; ///< 经过的边 (from, to)
    bool isErrorPath = false;   ///< 是否为故障/异常退出路径

    QJsonObject toJson() const;
    static FunctionPath fromJson(const QJsonObject& json);
};

/**
 * @brief 函数级全路径集合分析模型
 */
struct FunctionPathAnalysis {
    QString functionName;
    int startLine = 0;
    int endLine = 0;
    int cyclomaticComplexity = 1;
    QVector<FunctionPath> paths;
};

/**
 * @brief 函数级度量指标数据模型
 */
struct FunctionMetrics {
    QString name;               ///< 函数名称
    QString returnType;         ///< 函数返回值类型
    QString filePath;           ///< 所属源文件路径
    int startLine = 0;          ///< 函数起始定义行
    int endLine = 0;            ///< 函数闭合结束行
    int cyclomaticComplexity = 1; ///< McCabe 圈复杂度 V(G)
    int parameterCount = 0;     ///< 入参形参个数
    int linesOfCode = 0;        ///< 代码物理行数 (LOC)
    int statementCount = 0;     ///< 函数内有效语句数
    int branchCount = 0;        ///< 函数内决策分支数
    int mcdcCount = 0;          ///< 函数内 MC/DC 判定与原子条件数
    int maxNestingDepth = 0;    ///< 最大括号/分支嵌套深度
    int fanIn = 0;              ///< 扇入数 (Fan-In): 调用此函数的上级函数数量
    int fanOut = 0;             ///< 扇出数 (Fan-Out): 此函数调用的下级函数数量
    int functionDepth = 1;      ///< 函数深度 (控制结构最大嵌套层级)
    int callDepth = 1;          ///< 调用深度 (调用图中的调用栈层级)
    bool isRecursive = false;   ///< 是否存在直接/间接递归 (嵌入式安全禁用)

    QJsonObject toJson() const;
    static FunctionMetrics fromJson(const QJsonObject& json);
};

/**
 * @brief 函数级覆盖度量统计信息 (用于覆盖统计大表及柱状图)
 */
struct FunctionCoverageInfo {
    int index = 0;              ///< 序号 (0, 1, 2...)
    QString functionName;       ///< 函数名称
    QString fileName;           ///< 文件名 (相对或绝对路径)
    int statementCount = 0;     ///< 语句数
    int coveredStatements = 0;  ///< 语句覆盖数
    double statementCoveragePercent = 0.0; ///< 语句覆盖率 (%)
    int branchCount = 0;        ///< 分支数
    int coveredBranches = 0;    ///< 分支覆盖数
    double branchCoveragePercent = 0.0;    ///< 分支覆盖率 (%)
    int startLine = 0;          ///< 函数起始行
    int endLine = 0;            ///< 函数结束行

    QJsonObject toJson() const;
    static FunctionCoverageInfo fromJson(const QJsonObject& json);
};

/**
 * @brief 静态分析汇总报告模型
 */
struct StaticAnalysisReport {
    QString filePath;           ///< 目标源文件路径
    int totalLines = 0;         ///< 总行数
    int codeLines = 0;          ///< 有效代码行数 (SLOC)
    int commentLines = 0;       ///< 注释行数
    int blankLines = 0;         ///< 空白行数
    QVector<StaticIssue> issues; ///< 检出的缺陷列表
    QVector<FunctionMetrics> functions; ///< 包含的函数度量列表
    CodeMetrics metrics;        ///< 结构化度量指标 (语句数、分支数、注释率、MC/DC 等)
    QDateTime timestamp;        ///< 分析时间戳
    qint64 elapsedMs = 0;       ///< 分析耗时 (毫秒)

    int errorCount() const;     ///< 统计严重与错误总数
    int warningCount() const;   ///< 统计警告总数
    int styleCount() const;     ///< 统计规范与提示总数

    QJsonObject toJson() const;
    static StaticAnalysisReport fromJson(const QJsonObject& json);
};

/**
 * @brief 分支/判定点覆盖度量模型
 */
struct BranchPoint {
    int branchId = 0;           ///< 判定探针全局编号
    int line = 0;               ///< 所在源码行号
    QString condition;          ///< 分支判定表达式原始内容
    qint64 trueHitCount = 0;    ///< True 条件命中次数
    qint64 falseHitCount = 0;   ///< False 条件命中次数

    bool isFullyCovered() const { return trueHitCount > 0 && falseHitCount > 0; }
    bool isPartiallyCovered() const { return (trueHitCount > 0) ^ (falseHitCount > 0); }
    bool isUncovered() const { return trueHitCount == 0 && falseHitCount == 0; }

    QJsonObject toJson() const;
    static BranchPoint fromJson(const QJsonObject& json);
};

/**
 * @brief 单行覆盖详情模型
 */
struct LineCoverage {
    int lineNumber = 0;         ///< 源码行号
    qint64 hitCount = 0;        ///< 该行执行命中总次数
    LineCoverageStatus status = LineCoverageStatus::NotExecutable; ///< 覆盖状态枚举
    QVector<BranchPoint> branches; ///< 若该行包含分支判定，所附带的分支探针列表

    QJsonObject toJson() const;
    static LineCoverage fromJson(const QJsonObject& json);
};

/**
 * @brief 综合代码覆盖率报告模型
 */
struct CoverageReport {
    QString filePath;           ///< 分析文件路径
    int executableLines = 0;    ///< 可执行代码行总数
    int coveredLines = 0;       ///< 已覆盖行总数
    int totalBranches = 0;      ///< 分支判定总数
    int coveredBranches = 0;    ///< 已双向覆盖分支数
    int totalFunctions = 0;     ///< 函数总数
    int coveredFunctions = 0;   ///< 已执行函数数
    QMap<int, LineCoverage> lineDetails; ///< 行号到覆盖详情映射
    QVector<FunctionCoverageInfo> functionCoverages; ///< 函数级覆盖统计列表
    QDateTime timestamp;        ///< 报告生成时间戳
    QMap<QString, CoverageReport> fileReports; ///< 按文件路径隔离保存的各个源文件详细覆盖报告

    /// 语句覆盖率百分比 (0.0% ~ 100.0%)
    double statementCoveragePercent() const {
        return executableLines > 0 ? (100.0 * coveredLines / executableLines) : 0.0;
    }

    /// 分支/判定覆盖率百分比 (0.0% ~ 100.0%)
    double branchCoveragePercent() const {
        return totalBranches > 0 ? (100.0 * coveredBranches / totalBranches) : 0.0;
    }

    /// 函数调用覆盖率百分比 (0.0% ~ 100.0%)
    double functionCoveragePercent() const {
        return totalFunctions > 0 ? (100.0 * coveredFunctions / totalFunctions) : 0.0;
    }

    QJsonObject toJson() const;
    static CoverageReport fromJson(const QJsonObject& json);
};

/**
 * @brief 单步测试激励向量数据结构
 */
struct StimulusStep {
    double timestampMs = 0.0;   ///< 时隙时间戳 (毫秒)
    QMap<QString, double> signalValues; ///< 注入的命名信号量与其物理数值
    QString rawPayload;         ///< 原始总线报文或字符串载荷描述

    QJsonObject toJson() const;
    static StimulusStep fromJson(const QJsonObject& json);
};

/**
 * @brief 完整激励测试计划数据模型
 */
struct StimulusPlan {
    QString scriptPath;         ///< 来源 Python 脚本路径或在内存中的名称
    QString name;               ///< 激励用例名称
    QString targetModule;       ///< 目标测试嵌入式模块文件名 (如 motor_controller.c)
    QString description;        ///< 激励场景功能说明
    QStringList signalNames;    ///< 涉及的信号名称列表
    QVector<StimulusStep> steps;///< 包含的时序步列表
    double durationMs = 0.0;    ///< 激励总时长 (毫秒)
    QDateTime generatedAt;      ///< 激励生成时间

    QJsonObject toJson() const;
    static StimulusPlan fromJson(const QJsonObject& json);
};

/**
 * @brief 静态堆栈函数调用与局部帧指标
 */
struct StackFunctionInfo {
    QString name;               ///< 函数名称
    QString sourceFile;         ///< 所在源码文件
    int line = 0;               ///< 起始行号
    quint64 address = 0;        ///< 函数入口十六进制地址
    int localStackBytes = 0;    ///< 该函数自身分配的局部栈空间大小 (字节)
    QString frameType;          ///< 栈帧类型 (static, dynamic, leaf)
    int maxCallStackBytes = 0;  ///< 以该函数为根节点时，最坏情况的最大调用栈总和 (字节)
    QStringList worstCasePath;  ///< 最大调用栈路径链条 (e.g. main -> Task1 -> SubFunc)
    QStringList callees;        ///< 该函数直接调用的下游子函数列表
    QStringList callers;        ///< 调用该函数的上游父函数列表
    bool isRecursive = false;   ///< 是否存在自递归或互相递归风险
    QString disassembly;        ///< 该函数的局部反汇编文本指令片段

    QJsonObject toJson() const;
    static StackFunctionInfo fromJson(const QJsonObject& json);
};

/**
 * @brief 静态堆栈与二进制调用深度分析综合报告
 */
struct StackAnalysisReport {
    QString binaryPath;                 ///< 分析的目标可执行程序或目标文件路径
    QString architecture;               ///< 目标处理器架构 (e.g. x86_64, ARM Cortex-M)
    QDateTime analyzedAt;               ///< 分析完成时间戳
    int totalFunctions = 0;             ///< 识别解析出的有效函数总数
    int maxWorstCaseStackBytes = 0;     ///< 全局最深调用栈深度消耗 (字节)
    QString worstCaseRootFunction;      ///< 导致最大栈深度的顶层根函数 (通常为 main 或 ISR 中断)
    QStringList worstCaseCallChain;     ///< 全局最大调用栈调用序列
    QVector<StackFunctionInfo> functions;///< 各函数详细度量
    QString fullDisassembly;            ///< 全量反汇编代码文本
    bool hasRecursion = false;          ///< 全局是否存在递归调用违规

    QJsonObject toJson() const;
    static StackAnalysisReport fromJson(const QJsonObject& json);
};

} // namespace Coverage
