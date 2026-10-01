#pragma once

#include "coverage/models.h"
#include "coverage/static_analyzer.h"
#include "graph_canvas_view.h"
#include "code_editor.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QVector>

#include "theme_manager.h"

namespace Coverage {

/**
 * @brief 静态分析与路径探索综合工作台 (StaticAnalysisExplorerView)
 * 采用双 Tab 架构：
 * 1. Tab 1:「函数度量总览」：展示当前文件所有函数列表，包含序号、函数名称、函数路径、语句数、分支数、MCDC数及整文件 KPI。
 * 2. Tab 2:「函数控制流与路径探索」：左侧函数与独立路径列表，中间 Graphviz 拓扑高亮渲染，右侧源码编辑器动态绿色着色。
 */
class StaticAnalysisExplorerView : public QWidget {
    Q_OBJECT
public:
    explicit StaticAnalysisExplorerView(QWidget* parent = nullptr);
    ~StaticAnalysisExplorerView() override = default;

    void setSourceFile(const QString& filePath, const QString& sourceCode);
    void setAnalysisReport(const StaticAnalysisReport& report, const QString& sourceCode);
    void retranslateUi();
    void applyTheme(ThemeType type = ThemeType::DarkCatppuccin);

signals:
    void runAnalysisRequested(const QString& filePath);
    void jumpToEditorLine(int line, int col);

private slots:
    void onAnalyzeButtonClicked();
    void onOverviewTableDoubleClicked(int row, int col);
    void onFunctionTableSelectionChanged();
    void onPathTableSelectionChanged();
    void onGraphNodeClicked(const QString& name, int startLine, int endLine);

private:
    void setupUi();
    void updateKpiBadges();
    void populateOverviewTable();
    void populateFunctionTable();
    void populatePathTable(const QVector<FunctionPath>& paths);
    void renderSelectedPath(const FunctionMetrics& func, const FunctionPath& path);

    // 数据模型
    QString m_filePath;
    QString m_sourceCode;
    StaticAnalysisReport m_report;
    QVector<FunctionMetrics> m_functions;
    QVector<FunctionPath> m_currentPaths;
    int m_currentFuncIndex = -1;

    // 顶级 TabWidget (函数度量总览 vs 控制流与路径探索)
    QTabWidget* m_tabWidget;

    // --- Tab 1: 函数度量总览 ---
    QWidget* m_tabOverview;
    QPushButton* m_btnAnalyze;
    QLabel* m_lblFile;
    QLabel* m_kpiStatements;
    QLabel* m_kpiBranches;
    QLabel* m_kpiCommentRatio;
    QLabel* m_kpiMcdc;
    QLabel* m_kpiComplexity;
    QLabel* m_kpiIssues;
    QTableWidget* m_overviewTable;

    // --- Tab 2: 控制流与路径探索 ---
    QWidget* m_tabPathExplorer;
    QSplitter* m_mainSplitter;
    QTableWidget* m_funcTable;
    QTableWidget* m_pathTable;

    // 中间 Graphviz 原生画布
    GraphCanvasView* m_canvasView;
    QLabel* m_lblPathStatus;
    QPushButton* m_btnZoomIn;
    QPushButton* m_btnZoomOut;
    QPushButton* m_btnFit;
    QPushButton* m_btnReset;

    // 右侧源码着色视窗
    CodeEditor* m_rightCodeEditor;
    QLabel* m_lblRightHeader;

    // 主题样式管理与组件引用
    ThemeType m_currentTheme = ThemeType::DarkCatppuccin;
    QWidget* m_topCard = nullptr;
    QLabel* m_lblTableHint = nullptr;
    QLabel* m_lblFuncTitle = nullptr;
    QLabel* m_lblPathTitle = nullptr;
};

} // namespace Coverage
