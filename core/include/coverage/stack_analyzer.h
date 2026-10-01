#ifndef COVERAGE_STACK_ANALYZER_H
#define COVERAGE_STACK_ANALYZER_H

#include "coverage/models.h"
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>

namespace Coverage {

/**
 * @brief 嵌入式可执行程序二进制反汇编、静态堆栈与最坏情况调用深度分析器
 * @note 专为 ISO 26262 ASIL-D、DO-178C 等功能安全标准设计，通过分析可执行文件
 *       和目标文件 (.exe, .elf, .o) 的反汇编指令与 GCC -fstack-usage 导出数据，
 *       获取函数局部栈大小、构建全局调用图并精确推导最大调用栈深度。
 */
struct StackUsageEntry {
    QString sourceFile;
    int line = 0;
    int bytes = 0;
    QString frameType;
};

class StackAnalyzer {
public:
    /**
     * @brief 对指定的可执行文件或目标文件进行全量静态堆栈与反汇编分析
     * @param binaryPath 可执行文件或目标文件路径 (.exe, .elf, .out, .o, .obj)
     * @param suFilePath 可选的 GCC -fstack-usage 生成的 .su 文件路径 (若为空则自动在同目录搜索)
     * @return 静态堆栈分析综合报告
     */
    static StackAnalysisReport analyzeBinary(const QString& binaryPath, const QString& suFilePath = QString());

    /**
     * @brief 解析 GCC -fstack-usage 导出的 .su 堆栈度量记录文件
     * @param suFilePath .su 文件路径
     * @return 函数名 -> 局部栈字节数映射表
     */
    static QMap<QString, int> parseStackUsageFile(const QString& suFilePath);
    static QMap<QString, StackUsageEntry> parseStackUsageEntries(const QString& suFilePath);

    /**
     * @brief 自动定位可用的 objdump 反汇编工具可执行程序路径
     */
    static QString findObjdumpExecutable();

    /**
     * @brief 导出堆栈分析报告为人类可读的格式化报告文本
     */
    static QString exportReportToText(const StackAnalysisReport& report);

private:
    static void computeMaxCallStack(StackAnalysisReport& report);
    static void dfsCallStack(const QString& funcName,
                             const QMap<QString, StackFunctionInfo>& funcMap,
                             QSet<QString>& visitedInPath,
                             int currentDepth,
                             const QStringList& currentPath,
                             int& maxDepth,
                             QStringList& worstPath,
                             bool& detectedRecursion);
};

} // namespace Coverage

#endif // COVERAGE_STACK_ANALYZER_H
