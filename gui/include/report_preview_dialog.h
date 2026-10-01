#pragma once

#include "coverage/models.h"
#include <QDialog>
#include <QTextBrowser>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

namespace Coverage {

/**
 * @brief 综合分析报告预览与多格式导出对话框 (ReportPreviewDialog)
 * 导出前提供所见即所得 (WYSIWYG) 的全貌预览框，
 * 支持在界面中动态切换预览 HTML / PDF / Word (.doc) / JSON 格式，
 * 并支持自由指定导出路径与格式一键落盘。
 */
class ReportPreviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit ReportPreviewDialog(QWidget* parent = nullptr);
    ~ReportPreviewDialog() override = default;

    /**
     * @brief 载入待预览与导出的分析数据
     */
    void setReportData(const StaticAnalysisReport& staticReport,
                       const CoverageReport& covReport,
                       const QString& sourceFile,
                       const QString& sourceContent,
                       const StimulusPlan& plan);

    /**
     * @brief 设置是否显示顶部导出控制栏 (设为 false 时纯只读阅读，无导出控件)
     */
    void setExportVisible(bool visible);

    /**
     * @brief 载入并内部预览已存在的 HTML 报告文件 (兼容历史接口)
     */
    void loadExistingHtmlFile(const QString& filePath);

    /**
     * @brief 载入并内部预览已存在的任意报告文件 (HTML / PDF / DOCX / JSON / TXT)
     */
    void loadExistingReportFile(const QString& filePath);

private slots:
    void onFormatChanged(int index);
    void onBrowsePath();
    void onExportClicked();
    void onZoomIn();
    void onZoomOut();

private:
    void setupUi();
    void refreshPreviewContent();
    QString suggestDefaultFilePath(int formatIndex) const;

    StaticAnalysisReport m_staticReport;
    CoverageReport m_covReport;
    QString m_sourceFile;
    QString m_sourceContent;
    StimulusPlan m_stimulusPlan;

    QString m_htmlContent;
    QString m_printableHtmlContent;
    QString m_jsonContent;

    // 控件
    QWidget* m_topBarWidget = nullptr;
    QComboBox* m_cmbFormat;
    QLineEdit* m_txtPath;
    QPushButton* m_btnBrowse;
    QPushButton* m_btnExport;
    QPushButton* m_btnZoomIn;
    QPushButton* m_btnZoomOut;
    QLabel* m_lblStatus;
    QTextBrowser* m_previewBrowser;
};

} // namespace Coverage
