#pragma once

#include "coverage/models.h"
#include "theme_manager.h"
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>

namespace Coverage {

class StaticAnalysisView : public QWidget {
    Q_OBJECT
public:
    explicit StaticAnalysisView(QWidget* parent = nullptr);

    void setReport(const StaticAnalysisReport& report);
    void clear();
    void applyTheme(ThemeType type = ThemeType::DarkCatppuccin);

signals:
    void issueSelected(int line, int column);

private slots:
    void onFilterChanged();
    void onTableItemDoubleClicked(int row, int column);
    void onTableSelectionChanged();

private:
    StaticAnalysisReport m_report;
    ThemeType m_currentTheme = ThemeType::DarkCatppuccin;

    QLabel* m_lblTotalIssues;
    QLabel* m_lblErrors;
    QLabel* m_lblWarnings;
    QLabel* m_lblMetrics;

    QComboBox* m_cmbSeverityFilter;
    QLineEdit* m_txtSearch;
    QTableWidget* m_table;
    QTextEdit* m_txtDetails;

    void updateTable();
};

} // namespace Coverage
