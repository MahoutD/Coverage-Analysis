#pragma once

#include "coverage/models.h"
#include <QString>
#include <QVector>
#include <QPair>

namespace Coverage {

/**
 * @brief Graphviz 代码结构分析图生成器
 * 负责分析嵌入式 C 代码的调用拓扑结构与函数内部控制流结构，
 * 生成符合 Graphviz 规范的 DOT 语言脚本，并支持调用 dot 命令渲染为 SVG 矢量图。
 */
class GraphGenerator {
public:
    GraphGenerator() = default;
    ~GraphGenerator() = default;

    /**
     * @brief 自动探测系统中的 Graphviz dot 可执行程序路径
     * @return dot.exe 的绝对路径或可执行命令名
     */
    static QString findDotBinary();

    /**
     * @brief 生成全局函数调用关系图 (Call Graph) 的 DOT 脚本
     * @param report 静态分析报告 (包含所有识别到的函数列表及其圈复杂度)
     * @param sourceCode C 源代码全文 (用于扫描函数体内部的调用语句)
     * @return Graphviz DOT 脚本字符串
     */
    static QString generateCallGraphDot(const StaticAnalysisReport& report, const QString& sourceCode);

    /**
     * @brief 生成单个函数内部控制流图 (Control Flow Graph - CFG) 的 DOT 脚本
     * 展现函数内部基本块 (Basic Block)、决策分支 (if/else/switch/while) 以及节点边关系
     * @param func 目标函数度量模型
     * @param sourceCode C 源代码全文
     * @return Graphviz DOT 脚本字符串
     */
    static QString generateFunctionCfgDot(const FunctionMetrics& func, const QString& sourceCode);

    /**
     * @brief 生成带有高亮执行路径标注的单个函数控制流图 DOT 脚本
     * @param func 目标函数度量模型
     * @param sourceCode C 源代码全文
     * @param path 所选中的特定执行路径
     * @return 路径高亮的 Graphviz DOT 脚本
     */
    static QString generateHighlightedPathCfgDot(const FunctionMetrics& func, const QString& sourceCode, const FunctionPath& path);

    /**
     * @brief 调用外部 Graphviz dot.exe 将 DOT 脚本编译为 SVG 矢量图
     * @param dotContent DOT 脚本内容
     * @param dotBinaryPath dot.exe 路径 (若为空则自动探测)
     * @param errorMsg 输出错误信息
     * @return 成功返回 SVG XML 字符串，失败返回空字符串
     */
    static QString renderDotToSvg(const QString& dotContent,
                                  const QString& dotBinaryPath = QString(),
                                  QString* errorMsg = nullptr);
};

} // namespace Coverage
