#pragma once

#include "coverage/models.h"
#include "theme_manager.h"
#include <QWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QRadioButton>
#include <QTabWidget>
#include <QSplitter>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QMap>
#include <QVector>

namespace Coverage {

class CSyntaxHighlighter;

enum class CoverageChartType {
    BarChart,   ///< 柱状图
    LineChart,  ///< 折线图
    PieChart    ///< 饼图
};

/**
 * @brief 覆盖率矢量图表组件 (支持右键自由切换柱状图、折线图与饼图)
 */
class CoverageBarChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit CoverageBarChartWidget(QWidget* parent = nullptr);

    void setData(const QVector<FunctionCoverageInfo>& data);
    void setSelectedFunction(const QString& funcName);
    void setChartType(CoverageChartType type);
    CoverageChartType chartType() const { return m_chartType; }
    void applyTheme(ThemeType theme);

signals:
    void functionClicked(const QString& funcName);
    void chartTypeChanged(CoverageChartType type);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void drawBarChart(QPainter& painter);
    void drawLineChart(QPainter& painter);
    void drawPieChart(QPainter& painter);

    QVector<FunctionCoverageInfo> m_data;
    QString m_selectedFunction;
    int m_hoveredIndex = -1;
    CoverageChartType m_chartType = CoverageChartType::BarChart;
    ThemeType m_theme = ThemeType::DarkCatppuccin;

    QVector<QRectF> m_barRects;
    QVector<QPointF> m_linePoints;
    struct PieSlice {
        int index = -1;
        double startAngle = 0.0;
        double sweepAngle = 0.0;
        QColor color;
        QString label;
    };
    QVector<PieSlice> m_pieSlices;
    QPointF m_pieCenter;
    double m_pieRadius = 0.0;
};

/**
 * @brief 代码覆盖带色阅读器旁侧行号与覆盖状态标记槽 (Gutter Area)
 */
class CoverageGutterArea : public QWidget {
public:
    explicit CoverageGutterArea(class CoverageCodeViewer* viewer);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    class CoverageCodeViewer* m_viewer;
};

/**
 * @brief 源代码实际覆盖着色视图组件 (按语句实际覆盖着绿色，未覆盖着浅红/警示色)
 */
class CoverageCodeViewer : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CoverageCodeViewer(QWidget* parent = nullptr);

    void setSourceAndCoverage(const QString& sourceCode, const CoverageReport& report);
    void gotoLine(int line);
    void findNextUncovered();
    void findPrevUncovered();
    void applyTheme(ThemeType theme);

    void gutterPaintEvent(QPaintEvent* event);
    int gutterWidth() const;

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateGutterWidth(int newBlockCount);
    void updateGutterArea(const QRect& rect, int dy);

private:
    CoverageGutterArea* m_gutter;
    CSyntaxHighlighter* m_highlighter = nullptr;
    CoverageReport m_coverageReport;
    ThemeType m_theme = ThemeType::DarkCatppuccin;
    int m_navigatedLine = -1;

    void updateCoverageHighlights();
};

/**
 * @brief 测试覆盖分析独立视窗 (包含左侧文件/函数过滤面板、右侧覆盖统计大表+柱状图、代码覆盖着色两大 Tab 页)
 */
class CoverageView : public QWidget {
    Q_OBJECT
public:
    explicit CoverageView(QWidget* parent = nullptr);

    void setCoverageReport(const CoverageReport& report,
                           const QVector<FunctionMetrics>& functions = QVector<FunctionMetrics>(),
                           const QString& sourceCode = QString(),
                           const QString& sourceFile = QString());
    void setSourceFile(const QString& filePath, const QString& content);
    void clear();
    void applyTheme(ThemeType type = ThemeType::DarkCatppuccin);
    void retranslateUi();

signals:
    void lineSelected(int line);
    void exportRequested();

private slots:
    void onSearchTextChanged(const QString& text);
    void onModeRadioToggled();
    void onListItemClicked(QListWidgetItem* item);
    void onListItemDoubleClicked(QListWidgetItem* item);
    void onTableSelectionChanged();
    void onTableDoubleClicked(int row, int col);
    void onChartFunctionClicked(const QString& funcName);
    void onFileComboIndexChanged(int index);

private:
    void setupUi();
    void populateList();
    void populateTable();
    void updateKpiBadges();

    CoverageReport m_report;
    QVector<FunctionMetrics> m_functions;
    QMap<QString, QString> m_fileContents; // filePath -> content
    QString m_activeFile;
    ThemeType m_currentTheme = ThemeType::DarkCatppuccin;

    // 左侧面板
    QRadioButton* m_radioFileMode = nullptr;
    QRadioButton* m_radioFuncMode = nullptr;
    QLineEdit* m_editSearch = nullptr;
    QListWidget* m_listItems = nullptr;

    // 右侧 Tab 区域 (置于下方)
    QTabWidget* m_tabWidget = nullptr;

    // Tab 1: 覆盖统计
    QWidget* m_tabStats = nullptr;
    QTableWidget* m_tableCoverage = nullptr;
    CoverageBarChartWidget* m_chartWidget = nullptr;

    // Tab 2: 代码覆盖
    QWidget* m_tabCode = nullptr;
    QComboBox* m_cmbFiles = nullptr;
    QLabel* m_lblStmtBadge = nullptr;
    QLabel* m_lblBranchBadge = nullptr;
    QLabel* m_lblFuncBadge = nullptr;
    QPushButton* m_btnPrevUncovered = nullptr;
    QPushButton* m_btnNextUncovered = nullptr;
    CoverageCodeViewer* m_codeViewer = nullptr;
};

} // namespace Coverage
