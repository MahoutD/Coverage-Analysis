#include "static_analysis_view.h"
#include "localization_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>

namespace Coverage {

StaticAnalysisView::StaticAnalysisView(QWidget* parent)
    : QWidget(parent)
{
    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, [this]() {
                updateTable();
                onTableSelectionChanged();
            });
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // 1. Top Statistics Bar
    auto* statsLayout = new QHBoxLayout();
    m_lblTotalIssues = new QLabel(QStringLiteral("Total Issues: 0"), this);
    m_lblErrors = new QLabel(QStringLiteral("Errors: 0"), this);
    m_lblErrors->setStyleSheet(QStringLiteral("color: #f38ba8; font-weight: bold;"));
    m_lblWarnings = new QLabel(QStringLiteral("Warnings: 0"), this);
    m_lblWarnings->setStyleSheet(QStringLiteral("color: #f9e2af; font-weight: bold;"));
    m_lblMetrics = new QLabel(QStringLiteral("Lines: 0 | Functions: 0"), this);
    m_lblMetrics->setStyleSheet(QStringLiteral("color: #89b4fa;"));

    statsLayout->addWidget(m_lblTotalIssues);
    statsLayout->addWidget(m_lblErrors);
    statsLayout->addWidget(m_lblWarnings);
    statsLayout->addWidget(m_lblMetrics);
    statsLayout->addStretch();

    // Filters
    m_cmbSeverityFilter = new QComboBox(this);
    m_cmbSeverityFilter->addItems({
        QStringLiteral("All Severities"),
        QStringLiteral("Errors & Critical"),
        QStringLiteral("Warnings"),
        QStringLiteral("MISRA C Violations")
    });

    m_txtSearch = new QLineEdit(this);
    m_txtSearch->setPlaceholderText(QStringLiteral("Search issues / rules..."));
    m_txtSearch->setClearButtonEnabled(true);

    statsLayout->addWidget(new QLabel(QStringLiteral("Filter:"), this));
    statsLayout->addWidget(m_cmbSeverityFilter);
    statsLayout->addWidget(m_txtSearch);

    mainLayout->addLayout(statsLayout);

    // 2. Splitter with Table and Details
    auto* splitter = new QSplitter(Qt::Vertical, this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("#"),
        QStringLiteral("Severity"),
        QStringLiteral("Rule ID"),
        QStringLiteral("Location"),
        QStringLiteral("Description"),
        QStringLiteral("Code Snippet")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    splitter->addWidget(m_table);

    // Details Text
    m_txtDetails = new QTextEdit(this);
    m_txtDetails->setReadOnly(true);
    m_txtDetails->setPlaceholderText(QStringLiteral("Select an issue to inspect embedded safety recommendation and MISRA guidelines..."));
    m_txtDetails->setMaximumHeight(140);
    splitter->addWidget(m_txtDetails);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    connect(m_cmbSeverityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StaticAnalysisView::onFilterChanged);
    connect(m_txtSearch, &QLineEdit::textChanged,
            this, &StaticAnalysisView::onFilterChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &StaticAnalysisView::onTableItemDoubleClicked);
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &StaticAnalysisView::onTableSelectionChanged);

    applyTheme(ThemeManager::currentTheme());
}

void StaticAnalysisView::applyTheme(ThemeType type) {
    m_currentTheme = type;
    bool isLight = (type == ThemeType::LightModern);

    QString tableStyle = isLight ?
        QStringLiteral(
            "QTableWidget { background-color: #ffffff; alternate-background-color: #f8f9fa; border: 1px solid #dadce0; color: #202124; gridline-color: #e8eaed; outline: none; }"
            "QHeaderView::section { background-color: #f1f3f4; color: #1a73e8; font-weight: bold; border: 1px solid #dadce0; padding: 6px; }"
            "QTableWidget::item:selected { background-color: #e8f0fe; color: #1a73e8; font-weight: 500; }"
            "QTableCornerButton::section { background-color: #f1f3f4; border: 1px solid #dadce0; }"
        ) :
        QStringLiteral(
            "QTableWidget { background-color: #1e1e2e; alternate-background-color: #181825; border: 1px solid #313244; color: #cdd6f4; gridline-color: #313244; outline: none; }"
            "QHeaderView::section { background-color: #181825; color: #89b4fa; font-weight: bold; border: 1px solid #313244; padding: 6px; }"
            "QTableWidget::item:selected { background-color: #45475a; color: #a6e3a1; font-weight: 500; }"
            "QTableCornerButton::section { background-color: #181825; border: 1px solid #313244; }"
        );
    if (m_table) m_table->setStyleSheet(tableStyle);

    if (m_txtDetails) {
        if (isLight) {
            m_txtDetails->setStyleSheet(QStringLiteral("QTextEdit { background-color: #ffffff; color: #202124; border: 1px solid #dadce0; border-radius: 4px; padding: 6px; }"));
        } else {
            m_txtDetails->setStyleSheet(QStringLiteral("QTextEdit { background-color: #11111b; color: #cdd6f4; border: 1px solid #313244; border-radius: 4px; padding: 6px; }"));
        }
    }

    if (m_lblTotalIssues) {
        m_lblTotalIssues->setStyleSheet(isLight ? QStringLiteral("color: #202124; font-weight: bold;") : QStringLiteral("color: #cdd6f4; font-weight: bold;"));
    }
    if (m_lblErrors) {
        m_lblErrors->setStyleSheet(isLight ? QStringLiteral("color: #dc2626; font-weight: bold;") : QStringLiteral("color: #f38ba8; font-weight: bold;"));
    }
    if (m_lblWarnings) {
        m_lblWarnings->setStyleSheet(isLight ? QStringLiteral("color: #d97706; font-weight: bold;") : QStringLiteral("color: #f9e2af; font-weight: bold;"));
    }
    if (m_lblMetrics) {
        m_lblMetrics->setStyleSheet(isLight ? QStringLiteral("color: #1a73e8;") : QStringLiteral("color: #89b4fa;"));
    }

    updateTable();
    onTableSelectionChanged();
}

void StaticAnalysisView::setReport(const StaticAnalysisReport& report) {
    m_report = report;

    m_lblTotalIssues->setText(QString("Total Issues: %1").arg(report.issues.size()));
    m_lblErrors->setText(QString("Errors: %1").arg(report.errorCount()));
    m_lblWarnings->setText(QString("Warnings: %1").arg(report.warningCount()));
    m_lblMetrics->setText(QString("Lines: %1 (Code: %2, Blank: %3) | Functions: %4")
                              .arg(report.totalLines)
                              .arg(report.codeLines)
                              .arg(report.blankLines)
                              .arg(report.functions.size()));

    updateTable();
}

void StaticAnalysisView::clear() {
    m_report = StaticAnalysisReport();
    m_table->setRowCount(0);
    m_txtDetails->clear();
    m_lblTotalIssues->setText(QStringLiteral("Total Issues: 0"));
    m_lblErrors->setText(QStringLiteral("Errors: 0"));
    m_lblWarnings->setText(QStringLiteral("Warnings: 0"));
    m_lblMetrics->setText(QStringLiteral("Lines: 0 | Functions: 0"));
}

void StaticAnalysisView::onFilterChanged() {
    updateTable();
}

void StaticAnalysisView::updateTable() {
    m_table->setRowCount(0);

    QString filterType = m_cmbSeverityFilter->currentText();
    QString search = m_txtSearch->text().trimmed().toLower();

    bool isZh = (LocalizationManager::instance().currentLanguage() == Language::Chinese);
    bool isLight = (m_currentTheme == ThemeType::LightModern);
    int rowIdx = 0;
    for (int i = 0; i < m_report.issues.size(); ++i) {
        const auto& issue = m_report.issues[i];

        // Filter condition
        if (filterType == QStringLiteral("Errors & Critical")) {
            if (issue.severity != Severity::Error && issue.severity != Severity::Critical) continue;
        } else if (filterType == QStringLiteral("Warnings")) {
            if (issue.severity != Severity::Warning) continue;
        } else if (filterType == QStringLiteral("MISRA C Violations")) {
            if (issue.category != RuleCategory::MisraC) continue;
        }

        if (!search.isEmpty()) {
            bool matches = issue.ruleId.toLower().contains(search) ||
                           issue.localizedMessage(isZh).toLower().contains(search) ||
                           issue.codeSnippet.toLower().contains(search);
            if (!matches) continue;
        }

        m_table->insertRow(rowIdx);

        auto* itemIdx = new QTableWidgetItem(QString::number(rowIdx + 1));
        auto* itemSev = new QTableWidgetItem(severityToString(issue.severity));
        auto* itemRule = new QTableWidgetItem(issue.ruleId);
        auto* itemLoc = new QTableWidgetItem(QString("%1:%2").arg(issue.line).arg(issue.column));
        auto* itemMsg = new QTableWidgetItem(issue.localizedMessage(isZh));
        auto* itemCode = new QTableWidgetItem(issue.codeSnippet);

        // Store original issue index in UserRole
        itemIdx->setData(Qt::UserRole, i);

        // Styling based on severity
        if (issue.severity == Severity::Critical || issue.severity == Severity::Error) {
            itemSev->setForeground(isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8")));
        } else if (issue.severity == Severity::Warning) {
            itemSev->setForeground(isLight ? QColor(QStringLiteral("#d97706")) : QColor(QStringLiteral("#f9e2af")));
        } else {
            itemSev->setForeground(isLight ? QColor(QStringLiteral("#1a73e8")) : QColor(QStringLiteral("#89b4fa")));
        }

        m_table->setItem(rowIdx, 0, itemIdx);
        m_table->setItem(rowIdx, 1, itemSev);
        m_table->setItem(rowIdx, 2, itemRule);
        m_table->setItem(rowIdx, 3, itemLoc);
        m_table->setItem(rowIdx, 4, itemMsg);
        m_table->setItem(rowIdx, 5, itemCode);

        rowIdx++;
    }
}

void StaticAnalysisView::onTableItemDoubleClicked(int row, int /* col */) {
    auto* item = m_table->item(row, 0);
    if (!item) return;

    int origIndex = item->data(Qt::UserRole).toInt();
    if (origIndex >= 0 && origIndex < m_report.issues.size()) {
        const auto& issue = m_report.issues[origIndex];
        emit issueSelected(issue.line, issue.column);
    }
}

void StaticAnalysisView::onTableSelectionChanged() {
    int row = m_table->currentRow();
    if (row < 0) return;

    auto* item = m_table->item(row, 0);
    if (!item) return;

    int origIndex = item->data(Qt::UserRole).toInt();
    if (origIndex >= 0 && origIndex < m_report.issues.size()) {
        const auto& issue = m_report.issues[origIndex];
        bool isZh = (LocalizationManager::instance().currentLanguage() == Language::Chinese);
        bool isLight = (m_currentTheme == ThemeType::LightModern);

        QString ruleColor = isLight ? QStringLiteral("#1a73e8") : QStringLiteral("#89b4fa");
        QString locColor = isLight ? QStringLiteral("#374151") : QStringLiteral("#cdd6f4");
        QString issueColor = isLight ? QStringLiteral("#dc2626") : QStringLiteral("#f38ba8");
        QString recColor = isLight ? QStringLiteral("#16a34a") : QStringLiteral("#a6e3a1");
        QString preBg = isLight ? QStringLiteral("#f1f3f4") : QStringLiteral("#181825");
        QString preBorder = isLight ? QStringLiteral("#dadce0") : QStringLiteral("#313244");
        QString preColor = isLight ? QStringLiteral("#b45309") : QStringLiteral("#fab387");

        QString html = QString(
            "<b style='color: %1;'>%2:</b> %3 (%4)<br/>"
            "<b style='color: %5;'>%6:</b> %7 %8, %9 %10<br/>"
            "<b style='color: %11;'>%12:</b> %13<br/>"
            "<b style='color: %14;'>%15:</b> %16<br/>"
            "<pre style='background: %17; border: 1px solid %18; padding: 6px; border-radius: 4px; color: %19; font-family: Consolas, monospace;'>%20</pre>"
        ).arg(ruleColor)
         .arg(isZh ? QStringLiteral("规则编号") : QStringLiteral("Rule"))
         .arg(issue.ruleId)
         .arg(categoryToString(issue.category))
         .arg(locColor)
         .arg(isZh ? QStringLiteral("代码位置") : QStringLiteral("Location"))
         .arg(isZh ? QStringLiteral("行") : QStringLiteral("Line"))
         .arg(issue.line)
         .arg(isZh ? QStringLiteral("列") : QStringLiteral("Col"))
         .arg(issue.column)
         .arg(issueColor)
         .arg(isZh ? QStringLiteral("缺陷描述") : QStringLiteral("Issue"))
         .arg(issue.localizedMessage(isZh))
         .arg(recColor)
         .arg(isZh ? QStringLiteral("安全修复建议") : QStringLiteral("Recommendation"))
         .arg(issue.localizedSuggestion(isZh))
         .arg(preBg)
         .arg(preBorder)
         .arg(preColor)
         .arg(issue.codeSnippet.toHtmlEscaped());

        m_txtDetails->setHtml(html);
    }
}

} // namespace Coverage
