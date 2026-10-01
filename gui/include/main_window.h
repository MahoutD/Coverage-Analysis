#pragma once

#include "coverage/backend_service.h"
#include "coverage/project.h"
#include "code_editor.h"
#include "static_analysis_view.h"
#include "static_analysis_explorer_view.h"
#include "coverage_view.h"
#include "stimulus_view.h"
#include "graph_view.h"
#include "theme_manager.h"
#include "localization_manager.h"
#include "user_manual_dialog.h"
#include "project_hub_dialog.h"
#include "stack_analysis_view.h"

#include <QMainWindow>
#include <QDockWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QComboBox>
#include <QActionGroup>
#include <QMenu>
#include <QAction>
#include <QMap>

namespace Coverage {

class WelcomeView;

/**
 * @brief 嵌入式 C 覆盖分析与静态分析套件主窗口
 * 负责协调管理项目工程体系 (.covproj)、多文件选项卡代码编辑器、Graphviz 图表、
 * 静态分析探索工作台、覆盖率仪表盘及 Python 激励工作台。
 * 支持 Visual Studio 级可移动、可停靠、可自由浮动布局，支持中英文即时切换与四套精美主题。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

public slots:
    void retranslateUi();

private slots:
    // 工程体系槽函数 (Requirement 6)
    void onActionNewProject();
    void onActionOpenProject();
    void onActionSaveProject();
    void onActionProjectHub();
    void onActionCloseProject();
    void onOpenSampleMotor();
    void onOpenSampleSensor();
    void onOpenRecentProject(const QString& path);

    // 菜单与工具栏槽函数
    void onActionOpenFile();
    void onActionImportDirectory();
    void onActionSaveFile();
    void onActionRunStaticAnalysis();
    void onActionInstrumentSource();
    void onActionStackAnalysis();
    void onActionRunStimulus();
    void onActionRunSimulation();
    void onActionExportReport();
    void onActionUserManual();
    void onActionAbout();
    void onActionViewLogFile();
    void onActionOpenLogDir();
    void onActionResetLayout();
    void onWorkspaceDockVisibilityChanged(bool visible);

    // 多文件选项卡编辑器槽函数 (Requirement 3)
    void onEditorTabCloseRequested(int index);
    void onEditorTabContextMenu(const QPoint& pos);
    void onEditorCurrentChanged(int index);

    // 静态分析工作区右键关闭槽函数 (Requirement 3)
    void onStaticExplorerContextMenu(const QPoint& pos);

    // 图元节点选中跳转
    void onGraphNodeSelected(const QString& name, int startLine, int endLine);
    void onActionCoverageAnalysis();

    // 设置与主题/语言
    void onLanguageActionTriggered();
    void onThemeActionTriggered();
    void onThemeChanged(ThemeType type);

    // 文件切换
    void onFileSwitcherActivated(int index);

    // 后端服务通知响应槽函数
    void onSourceFileLoaded(const QString& filePath, const QString& content);
    void onStaticAnalysisFinished(const StaticAnalysisReport& report);
    void onCoverageReportUpdated(const CoverageReport& report);
    void onStimulusPlanReady(const StimulusPlan& plan);
    void onInstrumentationFinished(bool success, const InstrumentationMetadata& meta, const QString& outputPath);
    void onBackendLog(LogLevel level, const QString& module, const QString& message);

    // 跨组件联动
    void onIssueSelectedInEditor(int line, int col);
    void onCoverageLineSelectedInEditor(int line);
    void onFileTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onFileTreeContextMenu(const QPoint& pos);
    void onBtnAddFileClicked();
    void onBtnAddFolderClicked();
    void onBtnRemoveFileClicked();
    void onDockTabBarContextMenu(QTabBar* bar, const QPoint& pt);

private:
    void setupUi();
    void setupMenusAndToolbars();
    void setupDocks();
    void setupConnections();
    void setupDockTabBarContextMenu();
    void openProjectFile(const QString& covprojPath);
    void updateProjectTree();
    void refreshRecentProjectsMenu();
    void updateProjectDependentUiState(bool hasProject);

    CodeEditor* getOrCreateEditorForFile(const QString& filePath, const QString& content = QString());
    CodeEditor* currentActiveEditor() const;
    void closeEditorTab(int index);
    void loadFileIntoWorkspace(const QString& filePath);
    void scanDirectoryAndPopulate(const QString& dirPath);
    void updateEditorTabsStyle(ThemeType type);
    void revealInExplorer(const QString& filePath);

    // 中央视图与组件
    WelcomeView* m_welcomeView = nullptr;               ///< 启动欢迎与工程引导视图 (Requirement 1)
    QTabWidget* m_editorTabs = nullptr;                 ///< 多文件源码编辑器选项卡
    QMap<QString, CodeEditor*> m_openEditors;           ///< 文件路径 -> 对应的 CodeEditor 实例
    StaticAnalysisExplorerView* m_staticExplorer = nullptr;
    GraphView* m_graphView = nullptr;
    StackAnalysisView* m_stackAnalysisView = nullptr;   ///< 静态堆栈与二进制最大调用栈分析器 (Requirement 15)
    StaticAnalysisView* m_staticView = nullptr;
    CoverageView* m_coverageView = nullptr;
    StimulusView* m_stimulusView = nullptr;
    QTextEdit* m_systemLog = nullptr;
    QTreeWidget* m_fileTree = nullptr;

    // 状态栏组件
    QLabel* m_lblFileStatus = nullptr;
    QLabel* m_lblAnalysisStatus = nullptr;
    QLabel* m_lblCoverageStatus = nullptr;
    QProgressBar* m_progStatusBar = nullptr;

    // 工具栏与文件切换器
    QLabel* m_lblFileSwitcher = nullptr;
    QComboBox* m_comboFileSwitcher = nullptr;

    // 停靠窗口体系 (Visual Studio 式全可移动、可停靠、可浮动布局)
    QDockWidget* m_dockLeft = nullptr;
    QDockWidget* m_dockWelcome = nullptr;               ///< 欢迎向导停靠窗口
    QDockWidget* m_dockCodeEditor = nullptr;
    QDockWidget* m_dockStaticExplorer = nullptr;
    QDockWidget* m_dockGraphView = nullptr;
    QDockWidget* m_dockStackAnalysis = nullptr;         ///< 静态堆栈分析停靠窗口 (Requirement 15)
    QDockWidget* m_dockCoverageView = nullptr;          ///< 测试覆盖分析独立停靠窗口
    QDockWidget* m_dockRight = nullptr;
    QDockWidget* m_dockBottom = nullptr;               ///< 统一下方停靠区：集成缺陷列表与运行日志
    QTabWidget* m_bottomTabs = nullptr;                 ///< 底部选项卡：缺陷列表 + 系统日志
    QTabWidget* m_rightTabs = nullptr;

    bool m_wasRightDockVisible = true;

    // 菜单对象
    QMenu* m_menuProject = nullptr;                    ///< 工程菜单 (Requirement 6)
    QMenu* m_menuRecentProjects = nullptr;              ///< 最近工程子菜单
    QMenu* m_menuFile = nullptr;
    QMenu* m_menuAnalysis = nullptr;
    QMenu* m_menuStimulus = nullptr;
    QMenu* m_menuView = nullptr;
    QMenu* m_menuSettings = nullptr;
    QMenu* m_menuLanguage = nullptr;
    QMenu* m_menuTheme = nullptr;
    QMenu* m_menuHelp = nullptr;
    QAction* m_actResetLayout = nullptr;

    // 工程动作对象 (Requirement 6)
    QAction* m_actNewProject = nullptr;
    QAction* m_actOpenProject = nullptr;
    QAction* m_actSaveProject = nullptr;
    QAction* m_actProjectHub = nullptr;
    QAction* m_actCloseProject = nullptr;
    QAction* m_actSampleMotor = nullptr;
    QAction* m_actSampleSensor = nullptr;

    // 菜单动作对象
    QAction* m_actOpen = nullptr;
    QAction* m_actImportDir = nullptr;
    QAction* m_actSave = nullptr;
    QAction* m_actExport = nullptr;
    QAction* m_actExit = nullptr;

    QAction* m_actStatic = nullptr;
    QAction* m_actInst = nullptr;
    QAction* m_actStackAnalysis = nullptr;             ///< 静态堆栈深度分析动作 (Requirement 15)
    QAction* m_actCoverageAnalysis = nullptr;          ///< 测试覆盖分析独立视窗动作
    QAction* m_actStim = nullptr;
    QAction* m_actSim = nullptr;

    // 受工程存在性控制的互斥动作列表 (Requirements 13 & 14)
    QList<QAction*> m_projectDependentActions;
    QList<QAction*> m_toolbarProjectDependentActions;

    QAction* m_actManual = nullptr;
    QAction* m_actViewLog = nullptr;
    QAction* m_actOpenLogDir = nullptr;
    QAction* m_actAbout = nullptr;

    // 语言单选动作
    QAction* m_actLangZh = nullptr;
    QAction* m_actLangEn = nullptr;
    QActionGroup* m_langGroup = nullptr;

    // 主题单选动作
    QAction* m_actThemeCatppuccin = nullptr;
    QAction* m_actThemeLight = nullptr;
    QAction* m_actThemeSolarized = nullptr;
    QAction* m_actThemeMonokai = nullptr;
    QActionGroup* m_themeGroup = nullptr;

    // 文件管理状态
    QString m_currentDirPath;
    QStringList m_importedFiles;
};

} // namespace Coverage
