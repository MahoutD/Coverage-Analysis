#ifndef COVERAGE_STACK_ANALYSIS_VIEW_H
#define COVERAGE_STACK_ANALYSIS_VIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QTabWidget>
#include "coverage/models.h"
#include "coverage/stack_analyzer.h"
#include "coverage/graph_layout.h"
#include "graph_canvas_view.h"
#include "theme_manager.h"

namespace Coverage {

/**
 * @brief 静态堆栈与二进制最大调用栈分析工作台视窗
 * 具备二进制反汇编、GCC .su 栈度量融合、DFS 全局调用图分析、
 * 最大调用深度路径图元矢量渲染与 MISRA 17.2 规约检测能力。
 */
class StackAnalysisView : public QWidget {
    Q_OBJECT

public:
    explicit StackAnalysisView(QWidget* parent = nullptr);
    ~StackAnalysisView() override = default;

    void setReport(const StackAnalysisReport& report);
    void setBinaryPath(const QString& path);
    void applyTheme(ThemeType type);
    void retranslateUi();

signals:
    void runAnalysisRequested(const QString& binaryPath, const QString& suPath);

public slots:
    void onAnalyzeClicked();
    void onExportClicked();
    void onBrowseBinary();
    void onBrowseSu();
    void onInstructionsClicked();
    void onRenderGraphClicked();
    void onFunctionTableSelectionChanged();
    void onWorstChainItemClicked(QTreeWidgetItem* item, int column);

private:
    void setupUi();
    void updateKpiCards();
    void populateFunctionTable();
    void displayFunctionDetails(const StackFunctionInfo& info);
    void displayWorstCaseChain();
    void renderWorstCaseCallGraph();

    // 控件成员
    QLineEdit* m_editBinaryPath = nullptr;
    QLineEdit* m_editSuPath = nullptr;
    QPushButton* m_btnBrowseBinary = nullptr;
    QPushButton* m_btnBrowseSu = nullptr;
    QPushButton* m_btnAnalyze = nullptr;
    QPushButton* m_btnExport = nullptr;
    QPushButton* m_btnInstructions = nullptr;
    QPushButton* m_btnRenderGraph = nullptr;

    // KPI 标签
    QLabel* m_lblKpiMaxStack = nullptr;
    QLabel* m_lblKpiRootFunc = nullptr;
    QLabel* m_lblKpiTotalFuncs = nullptr;
    QLabel* m_lblKpiRecursion = nullptr;

    QWidget* m_topCardWidget = nullptr;
    QWidget* m_cardMaxStack = nullptr;
    QWidget* m_cardRootFunc = nullptr;
    QWidget* m_cardTotalFuncs = nullptr;
    QWidget* m_cardRecursion = nullptr;

    // 主展示区
    QLabel* m_lblTableTitle = nullptr;
    QTableWidget* m_tableFunctions = nullptr;
    QTabWidget* m_chainTabs = nullptr;
    QTreeWidget* m_treeWorstChain = nullptr;
    GraphCanvasView* m_chainCanvasView = nullptr;
    QTextEdit* m_txtDisasm = nullptr;
    QLabel* m_lblDisasmHeader = nullptr;

    // 数据与主题
    StackAnalysisReport m_report;
    ThemeType m_currentTheme = ThemeType::DarkCatppuccin;
};

} // namespace Coverage

#endif // COVERAGE_STACK_ANALYSIS_VIEW_H
