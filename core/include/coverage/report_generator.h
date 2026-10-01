#pragma once

#include "coverage/models.h"
#include <QString>

namespace Coverage {

/**
 * @brief 综合质量分析与代码覆盖报告生成器
 * 支持生成标准独立 HTML 网页报告、Word 文档 (.doc) 以及 JSON 格式数据。
 */
class ReportGenerator {
public:
    /**
     * @brief 生成带有离线自包含 CSS 样式的完整 HTML 报告
     */
    static QString generateHtmlReport(const StaticAnalysisReport& staticReport,
                                      const CoverageReport& covReport,
                                      const QString& sourceFile,
                                      const QString& sourceContent,
                                      const StimulusPlan& plan);

    /**
     * @brief 生成原生 OpenXML Word 文档 (*.docx)
     * 遵循 ECMA-376 / ISO 29500 国际规范，包含丰富表格排版、KPI 统计表与缺陷建议表
     * @param targetFilePath 目标 .docx 文件的绝对路径
     * @return 成功返回 true，失败返回 false
     */
    static bool generateDocxReport(const QString& targetFilePath,
                                   const StaticAnalysisReport& staticReport,
                                   const CoverageReport& covReport,
                                   const QString& sourceFile,
                                   const StimulusPlan& plan);

    /**
     * @brief 生成专为 PDF 矢量渲染 (QPdfWriter) 与打印设计的表格排版 HTML
     */
    static QString generatePrintableHtmlReport(const StaticAnalysisReport& staticReport,
                                              const CoverageReport& covReport,
                                              const QString& sourceFile,
                                              const QString& sourceContent,
                                              const StimulusPlan& plan);

    /**
     * @brief 生成兼容 Microsoft Word 与 WPS 的旧版 Word 文本内容
     */
    static QString generateWordDocument(const QString& htmlReport);

    /**
     * @brief 生成结构化 JSON 格式报告
     */
    static QString generateJsonReport(const StaticAnalysisReport& staticReport,
                                      const CoverageReport& covReport);
};

} // namespace Coverage
