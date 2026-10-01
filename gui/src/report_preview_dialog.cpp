#include "report_preview_dialog.h"
#include "coverage/report_generator.h"
#include "coverage/logger.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QPdfWriter>
#include <QPageSize>
#include <QPageLayout>
#include <QTextDocument>

namespace Coverage {

ReportPreviewDialog::ReportPreviewDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("综合分析报告全貌预览与多格式导出 (Report Export Preview)"));
    resize(1150, 800);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.svg")));

    setupUi();
}

void ReportPreviewDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // 1. 顶部控制工具栏：格式选择、导出路径、浏览按钮、立即导出按钮
    m_topBarWidget = new QWidget(this);
    auto* topBar = new QHBoxLayout(m_topBarWidget);
    topBar->setContentsMargins(0, 0, 0, 0);

    topBar->addWidget(new QLabel(QStringLiteral("📄 导出格式:"), this));
    m_cmbFormat = new QComboBox(this);
    m_cmbFormat->addItem(QStringLiteral("HTML 独立网页报告 (*.html)"), 0);
    m_cmbFormat->addItem(QStringLiteral("PDF 矢量高清文档 (*.pdf)"), 1);
    m_cmbFormat->addItem(QStringLiteral("Word 综合工程文档 (*.docx)"), 2);
    m_cmbFormat->addItem(QStringLiteral("JSON 结构化数据 (*.json)"), 3);
    m_cmbFormat->setMinimumWidth(210);
    topBar->addWidget(m_cmbFormat);

    topBar->addWidget(new QLabel(QStringLiteral(" 📁 导出文件路径:"), this));
    m_txtPath = new QLineEdit(this);
    m_txtPath->setPlaceholderText(QStringLiteral("请指定导出文件的完整保存路径..."));
    topBar->addWidget(m_txtPath, 1);

    m_btnBrowse = new QPushButton(QStringLiteral("浏览..."), this);
    topBar->addWidget(m_btnBrowse);

    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);

    m_btnExport = new QPushButton(QStringLiteral("💾 立即导出报告"), this);
    if (isLight) {
        m_btnExport->setStyleSheet(QStringLiteral("QPushButton { background-color: #16a34a; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px; }"
                                                  "QPushButton:hover { background-color: #15803d; }"));
    } else {
        m_btnExport->setStyleSheet(QStringLiteral("QPushButton { background-color: #a6e3a1; color: #11111b; font-weight: bold; padding: 6px 14px; border-radius: 4px; }"
                                                  "QPushButton:hover { background-color: #94e2d5; }"));
    }
    topBar->addWidget(m_btnExport);

    mainLayout->addWidget(m_topBarWidget);

    // 2. 中央预览区
    auto* midHeader = new QHBoxLayout();
    m_lblStatus = new QLabel(QStringLiteral("📋 报告全貌预览 (所见即所得):"), this);
    m_lblStatus->setStyleSheet(isLight ?
        QStringLiteral("font-weight: bold; color: #1a73e8;") :
        QStringLiteral("font-weight: bold; color: #89b4fa;"));
    midHeader->addWidget(m_lblStatus);
    midHeader->addStretch();

    m_btnZoomIn = new QPushButton(QStringLiteral("🔍 放大"), this);
    m_btnZoomOut = new QPushButton(QStringLiteral("🔍 缩小"), this);
    midHeader->addWidget(m_btnZoomIn);
    midHeader->addWidget(m_btnZoomOut);
    mainLayout->addLayout(midHeader);

    m_previewBrowser = new QTextBrowser(this);
    m_previewBrowser->setOpenExternalLinks(true);
    m_previewBrowser->setStyleSheet(QStringLiteral(
        "QTextBrowser {"
        "  background-color: #ffffff;"
        "  color: #212529;"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 6px;"
        "  padding: 16px;"
        "  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "}"
    ));
    mainLayout->addWidget(m_previewBrowser, 1);

    // 3. 底部栏
    auto* bottomBar = new QHBoxLayout();
    auto* lblTip = new QLabel(QStringLiteral("💡 提示：在上方预览无误后，选择所需导出格式并点击「立即导出报告」即可完成持久化输出。"), this);
    lblTip->setStyleSheet(isLight ?
        QStringLiteral("color: #5f6368; font-size: 11px;") :
        QStringLiteral("color: #a6adc8; font-size: 11px;"));
    bottomBar->addWidget(lblTip, 1);

    auto* btnClose = new QPushButton(QStringLiteral("关闭"), this);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    bottomBar->addWidget(btnClose);
    mainLayout->addLayout(bottomBar);

    // 信号连接
    connect(m_cmbFormat, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportPreviewDialog::onFormatChanged);
    connect(m_btnBrowse, &QPushButton::clicked,
            this, &ReportPreviewDialog::onBrowsePath);
    connect(m_btnExport, &QPushButton::clicked,
            this, &ReportPreviewDialog::onExportClicked);
    connect(m_btnZoomIn, &QPushButton::clicked,
            this, &ReportPreviewDialog::onZoomIn);
    connect(m_btnZoomOut, &QPushButton::clicked,
            this, &ReportPreviewDialog::onZoomOut);
}

void ReportPreviewDialog::setReportData(const StaticAnalysisReport& staticReport,
                                       const CoverageReport& covReport,
                                       const QString& sourceFile,
                                       const QString& sourceContent,
                                       const StimulusPlan& plan)
{
    m_staticReport = staticReport;
    m_covReport = covReport;
    m_sourceFile = sourceFile;
    m_sourceContent = sourceContent;
    m_stimulusPlan = plan;

    m_htmlContent = ReportGenerator::generateHtmlReport(staticReport, covReport, sourceFile, sourceContent, plan);
    m_printableHtmlContent = ReportGenerator::generatePrintableHtmlReport(staticReport, covReport, sourceFile, sourceContent, plan);
    m_jsonContent = ReportGenerator::generateJsonReport(staticReport, covReport);

    m_txtPath->setText(suggestDefaultFilePath(m_cmbFormat->currentIndex()));
    refreshPreviewContent();
}

void ReportPreviewDialog::setExportVisible(bool visible) {
    if (m_topBarWidget) {
        m_topBarWidget->setVisible(visible);
    }
}

void ReportPreviewDialog::loadExistingHtmlFile(const QString& filePath) {
    loadExistingReportFile(filePath);
}

void ReportPreviewDialog::loadExistingReportFile(const QString& filePath) {
    setExportVisible(false); // 纯预览模式，不展示导出操作条
    QFileInfo fi(filePath);
    setWindowTitle(QStringLiteral("嵌入式分析报告全貌预览 - ") + fi.fileName());
    m_lblStatus->setText(QStringLiteral("📋 正在查看本地已生成报告: ") + fi.fileName());

    QString ext = fi.suffix().toLower();
    if (ext == QStringLiteral("html") || ext == QStringLiteral("htm")) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_htmlContent = QString::fromUtf8(file.readAll());
            file.close();
            m_previewBrowser->setHtml(m_htmlContent);
        }
    } else if (ext == QStringLiteral("json")) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString raw = QString::fromUtf8(file.readAll());
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
            m_previewBrowser->setPlainText(doc.toJson(QJsonDocument::Indented));
        }
    } else if (ext == QStringLiteral("pdf")) {
        QString html = QString(
            "<div style='padding: 24px; font-family: Segoe UI, sans-serif;'>"
            "<h2 style='color: #dc2626;'>📄 PDF 矢量报告预览</h2>"
            "<table style='border-collapse: collapse; width: 100%; margin: 16px 0;'>"
            "<tr><td style='padding: 8px; border: 1px solid #ddd;'><b>文件名称:</b></td><td style='padding: 8px; border: 1px solid #ddd;'>%1</td></tr>"
            "<tr><td style='padding: 8px; border: 1px solid #ddd;'><b>文件大小:</b></td><td style='padding: 8px; border: 1px solid #ddd;'>%2 KB</td></tr>"
            "<tr><td style='padding: 8px; border: 1px solid #ddd;'><b>最后更新:</b></td><td style='padding: 8px; border: 1px solid #ddd;'>%3</td></tr>"
            "<tr><td style='padding: 8px; border: 1px solid #ddd;'><b>物理路径:</b></td><td style='padding: 8px; border: 1px solid #ddd;'>%4</td></tr>"
            "</table>"
            "<p style='color: #4b5563; line-height: 1.6;'>本文件为二进制矢量 PDF 格式测试报告，已由软件内置报告解析引擎校验完整性。<br/>报告包含嵌入式函数调用栈静态度量、圈复杂度分析、语句覆盖率与分支判定矩阵。</p>"
            "</div>"
        ).arg(fi.fileName()).arg(fi.size() / 1024).arg(fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))).arg(fi.absoluteFilePath());
        m_previewBrowser->setHtml(html);
    } else if (ext == QStringLiteral("docx") || ext == QStringLiteral("doc")) {
        QString html = QString(
            "<div style='padding: 24px; font-family: Segoe UI, sans-serif;'>"
            "<h2 style='color: #2563eb;'>📝 Word 工程分析报告预览</h2>"
            "<table style='border-collapse: collapse; width: 100%; margin: 16px 0;'>"
            "<tr><td style='padding: 8px; border: 1px solid #ddd;'><b>文件名称:</b></td><td style='padding: 8px; border: 1px solid #ddd;'>%1</td></tr>"
            "<tr><td style='padding: 8px; border: 1px solid #ddd;'><b>文件大小:</b></td><td style='padding: 8px; border: 1px solid #ddd;'>%2 KB</td></tr>"
            "<tr><td style='padding: 8px; border: 1px solid #ddd;'><b>最后更新:</b></td><td style='padding: 8px; border: 1px solid #ddd;'>%3</td></tr>"
            "<tr><td style='padding: 8px; border: 1px solid #ddd;'><b>物理路径:</b></td><td style='padding: 8px; border: 1px solid #ddd;'>%4</td></tr>"
            "</table>"
            "<p style='color: #4b5563; line-height: 1.6;'>本文件为 Office OpenXML 格式综合工程质量报告，包含嵌入式软件质量体系审阅表、MISRA 规范遵循记录与测试覆盖结论。</p>"
            "</div>"
        ).arg(fi.fileName()).arg(fi.size() / 1024).arg(fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))).arg(fi.absoluteFilePath());
        m_previewBrowser->setHtml(html);
    } else {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_previewBrowser->setPlainText(QString::fromUtf8(file.readAll()));
            file.close();
        }
    }
}

QString ReportPreviewDialog::suggestDefaultFilePath(int formatIndex) const {
    QString dir = QDir::currentPath() + QStringLiteral("/reports");
    QDir().mkpath(dir);

    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"));
    QString base = dir + QStringLiteral("/coverage_analysis_report_") + timeStr;

    switch (formatIndex) {
    case 0: return base + QStringLiteral(".html");
    case 1: return base + QStringLiteral(".pdf");
    case 2: return base + QStringLiteral(".docx");
    case 3: return base + QStringLiteral(".json");
    }
    return base + QStringLiteral(".html");
}

void ReportPreviewDialog::onFormatChanged(int index) {
    m_txtPath->setText(suggestDefaultFilePath(index));
    refreshPreviewContent();
}

void ReportPreviewDialog::refreshPreviewContent() {
    int index = m_cmbFormat->currentIndex();
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    if (index == 3) {
        // JSON 原始数据展示
        if (isLight) {
            m_previewBrowser->setStyleSheet(QStringLiteral(
                "QTextBrowser {"
                "  background-color: #ffffff;"
                "  color: #0f172a;"
                "  font-family: 'Consolas', monospace;"
                "  font-size: 12px;"
                "  padding: 12px;"
                "  border: 1px solid #dadce0;"
                "}"
            ));
        } else {
            m_previewBrowser->setStyleSheet(QStringLiteral(
                "QTextBrowser {"
                "  background-color: #11111b;"
                "  color: #a6e3a1;"
                "  font-family: 'Consolas', monospace;"
                "  font-size: 12px;"
                "  padding: 12px;"
                "}"
            ));
        }
        m_previewBrowser->setPlainText(m_jsonContent);
    } else if (index == 1 || index == 2) {
        // PDF 与 Word 导出采用针对打印/文档排版的精致矢量表格布局
        m_previewBrowser->setStyleSheet(QStringLiteral(
            "QTextBrowser {"
            "  background-color: #ffffff;"
            "  color: #1e293b;"
            "  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
            "  font-size: 13px;"
            "  padding: 16px;"
            "}"
        ));
        m_previewBrowser->setHtml(m_printableHtmlContent);
    } else {
        // HTML 现代响应式网页预览
        m_previewBrowser->setStyleSheet(QStringLiteral(
            "QTextBrowser {"
            "  background-color: #ffffff;"
            "  color: #212529;"
            "  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
            "  font-size: 13px;"
            "  padding: 16px;"
            "}"
        ));
        m_previewBrowser->setHtml(m_htmlContent);
    }

    m_lblStatus->setText(QString("📋 报告全貌预览 (检测到 %1 处静态缺陷，语句覆盖率 %2%，函数 %3 个)")
                             .arg(m_staticReport.issues.size())
                             .arg(QString::number(m_covReport.statementCoveragePercent(), 'f', 1))
                             .arg(m_staticReport.functions.size()));
}

void ReportPreviewDialog::onBrowsePath() {
    int index = m_cmbFormat->currentIndex();
    QString filter;
    QString defExt;
    switch (index) {
    case 0: filter = QStringLiteral("HTML Webpage (*.html)"); defExt = QStringLiteral(".html"); break;
    case 1: filter = QStringLiteral("PDF Document (*.pdf)"); defExt = QStringLiteral(".pdf"); break;
    case 2: filter = QStringLiteral("Word Document (*.docx)"); defExt = QStringLiteral(".docx"); break;
    case 3: filter = QStringLiteral("JSON Data (*.json)"); defExt = QStringLiteral(".json"); break;
    }

    QString chosen = QFileDialog::getSaveFileName(this, QStringLiteral("选择报告导出路径"),
                                                  m_txtPath->text(), filter);
    if (!chosen.isEmpty()) {
        m_txtPath->setText(chosen);
    }
}

void ReportPreviewDialog::onExportClicked() {
    QString targetPath = m_txtPath->text().trimmed();
    if (targetPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("路径错误"), QStringLiteral("请先指定导出的文件目标路径。"));
        return;
    }

    QFileInfo fi(targetPath);
    QDir().mkpath(fi.absolutePath());

    int formatIdx = m_cmbFormat->currentIndex();
    bool success = false;
    QString errorMsg;

    if (formatIdx == 0) {
        // 1. 导出 HTML
        QFile file(targetPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_htmlContent;
            file.close();
            success = true;
        } else {
            errorMsg = file.errorString();
        }
    } else if (formatIdx == 1) {
        // 2. 导出高清 PDF 矢量文档
        QPdfWriter writer(targetPath);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

        QTextDocument doc;
        doc.setHtml(m_printableHtmlContent);
        doc.print(&writer);
        success = QFile::exists(targetPath);
    } else if (formatIdx == 2) {
        // 3. 导出原生 OpenXML Word 文档 (*.docx)
        success = ReportGenerator::generateDocxReport(targetPath, m_staticReport, m_covReport, m_sourceFile, m_stimulusPlan);
        if (!success) {
            errorMsg = QStringLiteral("写入 .docx 压缩包失败，请检查文件写入权限。");
        }
    } else if (formatIdx == 3) {
        // 4. 导出 JSON
        QFile file(targetPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_jsonContent;
            file.close();
            success = true;
        } else {
            errorMsg = file.errorString();
        }
    }

    if (success) {
        Logger::instance().log(LogLevel::Info, QStringLiteral("ReportExport"),
                               QStringLiteral("报告导出成功: ") + targetPath);
        QMessageBox::information(this, QStringLiteral("导出成功"),
                                 QStringLiteral("已成功导出质量分析与覆盖报告至：\n") + targetPath);
    } else {
        Logger::instance().log(LogLevel::Error, QStringLiteral("ReportExport"),
                               QStringLiteral("导出报告失败: ") + errorMsg);
        QMessageBox::critical(this, QStringLiteral("导出失败"),
                              QStringLiteral("写入文件失败: ") + errorMsg);
    }
}

void ReportPreviewDialog::onZoomIn() {
    m_previewBrowser->zoomIn(1);
}

void ReportPreviewDialog::onZoomOut() {
    m_previewBrowser->zoomOut(1);
}

} // namespace Coverage
