#pragma once

#include "coverage/models.h"
#include <QString>
#include <QRegularExpression>

namespace Coverage {

/**
 * @brief 嵌入式 C 静态代码分析引擎
 * 专注于车载、工控、航天等高可靠嵌入式 C 代码的静态缺陷分析。
 * 覆盖 MISRA C:2012 关键规范、内存越界与栈溢出风险、硬件寄存器易失性访问规范及 McCabe 圈复杂度评估。
 */
class StaticAnalyzer {
public:
    /**
     * @brief 静态分析规则配置选项
     */
    struct Options {
        bool checkMisraRules = true;          ///< 启用 MISRA C:2012 规则检查
        bool checkMemorySafety = true;        ///< 启用嵌入式内存安全与栈空间分析
        bool checkHardwareRegisters = true;   ///< 启用硬件寄存器访问 volatile 校验
        bool checkComplexity = true;          ///< 启用圈复杂度与控制流嵌套深度计算
        int maxComplexityWarning = 10;        ///< 圈复杂度警告阈值 (推荐 <= 10)
        int maxComplexityError = 15;          ///< 圈复杂度错误阈值 (超过 15 强制重构)
        int maxNestingDepth = 4;              ///< 最大允许嵌套层级 (深度 > 4 告警)
        int maxStackArrayBytes = 256;         ///< 局部栈数组最大安全字节数 (超过可能导致 MCU 栈溢出)
    };

    StaticAnalyzer();
    explicit StaticAnalyzer(const Options& options);
    ~StaticAnalyzer() = default;

    void setOptions(const Options& options) { m_options = options; }
    const Options& options() const { return m_options; }

    /**
     * @brief 对指定路径的 C 源文件执行全套静态分析
     * @param filePath 源码文件绝对或相对路径
     * @return 静态分析报告
     */
    StaticAnalysisReport analyzeFile(const QString& filePath);

    /**
     * @brief 对内存中的 C 源码文本执行全套静态分析
     * @param sourceCode 源码字符串
     * @param fileName 虚拟文件名 (用于问题归属标注)
     * @return 静态分析报告
     */
    StaticAnalysisReport analyzeSource(const QString& sourceCode, const QString& fileName = QStringLiteral("buffer.c"));

private:
    Options m_options;

    // 逐行语法扫描与正则表达式规则匹配 (MISRA、动态内存分配、易失性指针、栈空间)
    void analyzeLineByLine(const QStringList& lines, StaticAnalysisReport& report);

    // 函数级语法结构提取 (圈复杂度、递归分析、形参个数、嵌套深度)
    void analyzeFunctions(const QString& sourceCode, const QStringList& lines, StaticAnalysisReport& report);
    
    // McCabe 圈复杂度算法实现：V(G) = 1 + 分支决策点总数 (if/while/for/case/&&/||/?)
    int computeCyclomaticComplexity(const QString& functionBody);

    // 最大括号与控制流嵌套层数计算
    int computeMaxNestingDepth(const QString& functionBody);

public:
    /**
     * @brief 计算源代码全局结构与质量度量指标 (语句数、分支数、注释率、MC/DC 等)
     */
    CodeMetrics computeCodeMetrics(const QString& sourceCode, const QString& fileName, const QVector<FunctionMetrics>& functions);

    /**
     * @brief 提取指定函数的所有独立执行路径 (用于路径探测与 Graphviz/源码着色)
     */
    static QVector<FunctionPath> extractFunctionPaths(const FunctionMetrics& func, const QStringList& lines);
};

} // namespace Coverage
