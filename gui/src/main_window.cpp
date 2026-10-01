#include "main_window.h"
#include "welcome_view.h"
#include "report_preview_dialog.h"
#include "user_manual_dialog.h"
#include "project_hub_dialog.h"
#include "coverage/logger.h"
#include "coverage/app_config.h"
#include "coverage/report_generator.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QHeaderView>
#include <QDirIterator>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QTabBar>
#include <QClipboard>
#include <QGuiApplication>

namespace Coverage {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // 主窗口基础属性
    setWindowTitle(QStringLiteral("Embedded C Coverage Studio - 嵌入式 C 覆盖分析与静态分析套件"));
    resize(1500, 960);
    setMinimumSize(960, 600);
    setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.svg")));

    // Visual Studio 风格全停靠架构：隐藏中央占位部件，防止与 Dock 发生几何计算冲突
    auto* dummyCentral = new QWidget(this);
    dummyCentral->setVisible(false);
    dummyCentral->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setCentralWidget(dummyCentral);

    // 1. 多文件选项卡代码编辑器 (Requirement 3)
    m_editorTabs = new QTabWidget(this);
    m_editorTabs->setTabsClosable(true);
    m_editorTabs->setMovable(true);
    m_editorTabs->setContextMenuPolicy(Qt::CustomContextMenu);

    // 2. 静态分析与路径探索工作台、Graphviz 图表
    m_staticExplorer = new StaticAnalysisExplorerView(this);
    m_staticExplorer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_graphView = new GraphView(this);

    // 3. 状态栏
    auto* sBar = statusBar();
    m_lblFileStatus = new QLabel(trText(QStringLiteral("status.no_file")), this);
    m_lblAnalysisStatus = new QLabel(trText(QStringLiteral("status.analysis_idle")), this);
    m_lblCoverageStatus = new QLabel(trText(QStringLiteral("status.coverage_na")), this);
    m_progStatusBar = new QProgressBar(this);
    m_progStatusBar->setMaximumWidth(150);
    m_progStatusBar->setRange(0, 100);
    m_progStatusBar->setValue(0);
    m_progStatusBar->setVisible(false);

    sBar->addWidget(m_lblFileStatus);
    sBar->addPermanentWidget(m_lblAnalysisStatus);
    sBar->addPermanentWidget(m_lblCoverageStatus);
    sBar->addPermanentWidget(m_progStatusBar);

    // 4. 构建菜单、工具栏、Dock 停靠体系与信号联动
    setupDocks();
    setupMenusAndToolbars();
    setupConnections();

    ThemeType curTheme = ThemeManager::currentTheme();
    onThemeChanged(curTheme);

    // 5. 载入活动工程或进入纯净就绪状态 (Requirement 1)
    if (ProjectManager::instance().hasActiveProject()) {
        const auto& proj = ProjectManager::instance().currentProject();
        openProjectFile(proj.filePath);
    } else {
        updateProjectDependentUiState(false);
    }

    retranslateUi();
}

CodeEditor* MainWindow::getOrCreateEditorForFile(const QString& filePath, const QString& content) {
    QString normPath = QDir::toNativeSeparators(filePath);

    if (m_openEditors.contains(normPath)) {
        auto* editor = m_openEditors[normPath];
        int idx = m_editorTabs->indexOf(editor);
        if (idx >= 0) {
            m_editorTabs->setCurrentIndex(idx);
        }
        if (!content.isEmpty() && editor->toPlainText().isEmpty()) {
            editor->setPlainText(content);
        }
        return editor;
    }

    // 创建新的源码编辑器页
    auto* editor = new CodeEditor(m_editorTabs);
    editor->applyCurrentTheme();

    QString loadContent = content;
    if (loadContent.isEmpty() && QFile::exists(normPath)) {
        QFile file(normPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            loadContent = QString::fromUtf8(file.readAll());
            file.close();
        }
    }
    editor->setPlainText(loadContent);

    QFileInfo fi(normPath);
    int tabIdx = m_editorTabs->addTab(editor, QIcon(QStringLiteral(":/icons/open_file.svg")), fi.fileName());
    m_editorTabs->setTabToolTip(tabIdx, normPath);
    m_openEditors[normPath] = editor;
    m_editorTabs->setCurrentIndex(tabIdx);

    return editor;
}

CodeEditor* MainWindow::currentActiveEditor() const {
    if (!m_editorTabs || m_editorTabs->count() == 0) return nullptr;
    return qobject_cast<CodeEditor*>(m_editorTabs->currentWidget());
}

void MainWindow::closeEditorTab(int index) {
    if (index < 0 || index >= m_editorTabs->count()) return;

    QWidget* w = m_editorTabs->widget(index);
    QString pathToRemove;
    for (auto it = m_openEditors.begin(); it != m_openEditors.end(); ++it) {
        if (it.value() == w) {
            pathToRemove = it.key();
            break;
        }
    }

    if (!pathToRemove.isEmpty()) {
        m_openEditors.remove(pathToRemove);
    }

    m_editorTabs->removeTab(index);
    delete w;

    if (m_editorTabs->count() == 0) {
        if (m_dockCodeEditor) {
            m_dockCodeEditor->setWindowTitle(trText(QStringLiteral("dock.code_editor")));
        }
        m_lblFileStatus->setText(trText(QStringLiteral("status.no_file")));
    }
}

void MainWindow::onEditorTabCloseRequested(int index) {
    closeEditorTab(index);
}

void MainWindow::onEditorTabContextMenu(const QPoint& pos) {
    int tabIdx = m_editorTabs->tabBar()->tabAt(pos);
    if (tabIdx < 0) return;

    QString filePath = m_editorTabs->tabToolTip(tabIdx);

    QMenu menu(this);
    auto* actClose = menu.addAction(QIcon(QStringLiteral(":/icons/delete.svg")), QStringLiteral("关闭当前页 (Close Current)"));
    auto* actCloseOthers = menu.addAction(QStringLiteral("关闭除当前外所有页 (Close Others)"));
    auto* actCloseAll = menu.addAction(QStringLiteral("关闭所有页面 (Close All)"));
    menu.addSeparator();
    auto* actCopyPath = menu.addAction(QStringLiteral("复制完整文件路径 (Copy Path)"));

    connect(actClose, &QAction::triggered, this, [this, tabIdx]() {
        closeEditorTab(tabIdx);
    });

    connect(actCloseOthers, &QAction::triggered, this, [this, tabIdx]() {
        QWidget* keep = m_editorTabs->widget(tabIdx);
        for (int i = m_editorTabs->count() - 1; i >= 0; --i) {
            if (m_editorTabs->widget(i) != keep) {
                closeEditorTab(i);
            }
        }
    });

    connect(actCloseAll, &QAction::triggered, this, [this]() {
        while (m_editorTabs->count() > 0) {
            closeEditorTab(0);
        }
    });

    connect(actCopyPath, &QAction::triggered, this, [filePath]() {
        QGuiApplication::clipboard()->setText(filePath);
    });

    menu.exec(m_editorTabs->mapToGlobal(pos));
}

void MainWindow::onEditorCurrentChanged(int index) {
    if (index < 0 || index >= m_editorTabs->count()) return;

    QString filePath = m_editorTabs->tabToolTip(index);
    if (!filePath.isEmpty()) {
        auto* editor = qobject_cast<CodeEditor*>(m_editorTabs->widget(index));
        QString content = editor ? editor->toPlainText() : QString();

        BackendService::instance().setSourceContent(content, filePath);
        QFileInfo fi(filePath);
        if (m_dockCodeEditor) {
            m_dockCodeEditor->setWindowTitle(trText(QStringLiteral("dock.code_editor")) + QStringLiteral(" - ") + fi.fileName());
        }
        m_lblFileStatus->setText(QStringLiteral("文件: ") + fi.fileName());

        if (m_staticExplorer) {
            m_staticExplorer->setSourceFile(filePath, content);
        }

        // 同步工具栏下拉选择器
        int comboIdx = m_comboFileSwitcher->findData(filePath);
        if (comboIdx >= 0 && m_comboFileSwitcher->currentIndex() != comboIdx) {
            m_comboFileSwitcher->setCurrentIndex(comboIdx);
        }
    }
}

void MainWindow::onStaticExplorerContextMenu(const QPoint& pos) {
    QMenu menu(this);
    auto* actClose = menu.addAction(QIcon(QStringLiteral(":/icons/delete.svg")), QStringLiteral("关闭当前页 (静态分析与路径探索)"));
    auto* actCloseOthers = menu.addAction(QStringLiteral("关闭除当前外所有页"));
    auto* actCloseAll = menu.addAction(QStringLiteral("关闭所有页面"));
    menu.addSeparator();
    auto* actRerun = menu.addAction(QIcon(QStringLiteral(":/icons/static_analysis.svg")), QStringLiteral("⚡ 重新执行静态分析 (F5)"));
    auto* actSwitchEditor = menu.addAction(QIcon(QStringLiteral(":/icons/open_file.svg")), QStringLiteral("切换到代码编辑器视窗"));

    connect(actClose, &QAction::triggered, this, [this]() {
        m_dockStaticExplorer->hide();
    });
    connect(actCloseOthers, &QAction::triggered, this, [this]() {
        if (m_dockWelcome) m_dockWelcome->hide();
        if (m_dockCodeEditor) m_dockCodeEditor->hide();
        if (m_dockGraphView) m_dockGraphView->hide();
    });
    connect(actCloseAll, &QAction::triggered, this, [this]() {
        if (m_dockWelcome) m_dockWelcome->hide();
        if (m_dockCodeEditor) m_dockCodeEditor->hide();
        if (m_dockStaticExplorer) m_dockStaticExplorer->hide();
        if (m_dockGraphView) m_dockGraphView->hide();
    });
    connect(actRerun, &QAction::triggered, this, &MainWindow::onActionRunStaticAnalysis);
    connect(actSwitchEditor, &QAction::triggered, this, [this]() {
        m_dockCodeEditor->show();
        m_dockCodeEditor->raise();
    });

    menu.exec(m_staticExplorer->mapToGlobal(pos));
}

void MainWindow::setupMenusAndToolbars() {
    auto* mBar = menuBar();

    // 1. 工程菜单 (Project - Requirement 6)
    m_menuProject = mBar->addMenu(QStringLiteral("工程 (&P)"));
    m_actNewProject = m_menuProject->addAction(QIcon(QStringLiteral(":/icons/add_file.svg")), QStringLiteral("新建工程 (&N)..."), this, &MainWindow::onActionNewProject);
    m_actNewProject->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+N")));

    m_actOpenProject = m_menuProject->addAction(QIcon(QStringLiteral(":/icons/open_file.svg")), QStringLiteral("打开工程 (&O)..."), this, &MainWindow::onActionOpenProject);
    m_actOpenProject->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));

    m_actSaveProject = m_menuProject->addAction(QIcon(QStringLiteral(":/icons/save_file.svg")), QStringLiteral("保存工程 (&S)"), this, &MainWindow::onActionSaveProject);
    m_actSaveProject->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));

    m_actProjectHub = m_menuProject->addAction(QIcon(QStringLiteral(":/icons/app_icon.svg")), QStringLiteral("工程管理中心 (&H)..."), this, &MainWindow::onActionProjectHub);
    m_actProjectHub->setShortcut(QKeySequence(QStringLiteral("Ctrl+P")));

    m_menuProject->addSeparator();

    auto* menuSamples = m_menuProject->addMenu(QIcon(QStringLiteral(":/icons/refresh.svg")), QStringLiteral("快速体验内置示例工程"));
    m_actSampleMotor = menuSamples->addAction(QStringLiteral("🚗 车载无刷电机驱动与故障安全工程 (Motor Controller)"), this, &MainWindow::onOpenSampleMotor);
    m_actSampleSensor = menuSamples->addAction(QStringLiteral("🛰️ 航姿参考多传感器融合滤波工程 (Sensor Fusion)"), this, &MainWindow::onOpenSampleSensor);

    m_menuRecentProjects = m_menuProject->addMenu(QIcon(QStringLiteral(":/icons/open_file.svg")), QStringLiteral("最近打开的工程"));
    refreshRecentProjectsMenu();

    m_menuProject->addSeparator();
    m_actCloseProject = m_menuProject->addAction(QIcon(QStringLiteral(":/icons/delete.svg")), QStringLiteral("关闭工程 (&C)"), this, &MainWindow::onActionCloseProject);

    // 2. 文件菜单 (File)
    m_menuFile = mBar->addMenu(trText(QStringLiteral("menu.file")));
    m_actOpen = m_menuFile->addAction(QIcon(QStringLiteral(":/icons/open_file.svg")), trText(QStringLiteral("act.open_file")), this, &MainWindow::onActionOpenFile);
    m_actOpen->setShortcut(QKeySequence::Open);

    m_actImportDir = m_menuFile->addAction(QIcon(QStringLiteral(":/icons/import_dir.svg")), trText(QStringLiteral("act.import_dir")), this, &MainWindow::onActionImportDirectory);
    m_actImportDir->setShortcut(QKeySequence(QStringLiteral("Ctrl+I")));

    m_actSave = m_menuFile->addAction(QIcon(QStringLiteral(":/icons/save_file.svg")), trText(QStringLiteral("act.save")), this, &MainWindow::onActionSaveFile);
    m_actSave->setShortcut(QKeySequence::Save);

    m_menuFile->addSeparator();
    m_actExport = m_menuFile->addAction(QIcon(QStringLiteral(":/icons/export_report.svg")), trText(QStringLiteral("act.export_report")), this, &MainWindow::onActionExportReport);
    m_actExport->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));

    m_menuFile->addSeparator();
    m_actExit = m_menuFile->addAction(QIcon(QStringLiteral(":/icons/delete.svg")), trText(QStringLiteral("act.exit")), this, &QWidget::close);

    // 3. 分析菜单 (Analysis)
    m_menuAnalysis = mBar->addMenu(trText(QStringLiteral("menu.analysis")));
    m_actStatic = m_menuAnalysis->addAction(QIcon(QStringLiteral(":/icons/static_analysis.svg")), trText(QStringLiteral("act.run_static")), this, &MainWindow::onActionRunStaticAnalysis);
    m_actStatic->setShortcut(QKeySequence(QStringLiteral("F5")));

    m_actInst = m_menuAnalysis->addAction(QIcon(QStringLiteral(":/icons/instrument.svg")), trText(QStringLiteral("act.instrument")), this, &MainWindow::onActionInstrumentSource);
    m_actInst->setShortcut(QKeySequence(QStringLiteral("F6")));

    m_actStackAnalysis = m_menuAnalysis->addAction(QIcon(QStringLiteral(":/icons/stack_analysis.svg")), QStringLiteral("静态堆栈深度分析 (&K)..."), this, &MainWindow::onActionStackAnalysis);
    m_actStackAnalysis->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));

    m_menuAnalysis->addSeparator();

    m_actCoverageAnalysis = m_menuAnalysis->addAction(QIcon(QStringLiteral(":/icons/coverage_analysis.svg")), QStringLiteral("测试覆盖分析 (&V)..."), this, &MainWindow::onActionCoverageAnalysis);
    m_actCoverageAnalysis->setShortcut(QKeySequence(QStringLiteral("F8")));

    m_actSim = m_menuAnalysis->addAction(QIcon(QStringLiteral(":/icons/run_sim.svg")), trText(QStringLiteral("act.run_sim")), this, &MainWindow::onActionRunSimulation);
    m_actSim->setShortcut(QKeySequence(QStringLiteral("Ctrl+F8")));

    // 4. 激励菜单 (Stimulus)
    m_menuStimulus = mBar->addMenu(trText(QStringLiteral("menu.stimulus")));
    m_actStim = m_menuStimulus->addAction(QIcon(QStringLiteral(":/icons/run_stimulus.svg")), trText(QStringLiteral("act.run_stimulus")), this, &MainWindow::onActionRunStimulus);
    m_actStim->setShortcut(QKeySequence(QStringLiteral("F7")));

    // 5. 视图菜单 (View) - 管理所有 Dock 窗口的显示/隐藏 (Requirement 12: 选中效果与互斥，欢迎向导不列在项目中)
    m_menuView = mBar->addMenu(trText(QStringLiteral("menu.view")));

    auto addDockAction = [this](QDockWidget* dock, const QString& iconPath) -> QAction* {
        auto* act = dock->toggleViewAction();
        act->setIcon(QIcon(iconPath));
        act->setCheckable(true);
        act->setChecked(dock->isVisible());
        connect(dock, &QDockWidget::visibilityChanged, act, &QAction::setChecked);
        m_menuView->addAction(act);
        return act;
    };

    addDockAction(m_dockCodeEditor, QStringLiteral(":/icons/open_file.svg"));
    addDockAction(m_dockStaticExplorer, QStringLiteral(":/icons/static_analysis.svg"));
    addDockAction(m_dockGraphView, QStringLiteral(":/icons/path_explorer.svg"));
    addDockAction(m_dockStackAnalysis, QStringLiteral(":/icons/stack_analysis.svg"));
    addDockAction(m_dockCoverageView, QStringLiteral(":/icons/coverage_analysis.svg"));

    m_menuView->addSeparator();

    addDockAction(m_dockLeft, QStringLiteral(":/icons/add_folder.svg"));
    addDockAction(m_dockRight, QStringLiteral(":/icons/run_stimulus.svg"));
    addDockAction(m_dockBottom, QStringLiteral(":/icons/static_analysis.svg"));

    m_menuView->addSeparator();
    m_actResetLayout = m_menuView->addAction(trText(QStringLiteral("act.reset_layout")), this, &MainWindow::onActionResetLayout);

    // 6. 设置菜单 (Settings: Language & Themes)
    m_menuSettings = mBar->addMenu(trText(QStringLiteral("menu.settings")));

    m_menuLanguage = m_menuSettings->addMenu(trText(QStringLiteral("menu.language")));
    m_langGroup = new QActionGroup(this);
    m_langGroup->setExclusive(true);

    m_actLangZh = m_menuLanguage->addAction(trText(QStringLiteral("lang.zh")));
    m_actLangZh->setCheckable(true);
    m_actLangZh->setChecked(LocalizationManager::instance().currentLanguage() == Language::Chinese);
    m_actLangZh->setData(static_cast<int>(Language::Chinese));
    m_langGroup->addAction(m_actLangZh);
    connect(m_actLangZh, &QAction::triggered, this, &MainWindow::onLanguageActionTriggered);

    m_actLangEn = m_menuLanguage->addAction(trText(QStringLiteral("lang.en")));
    m_actLangEn->setCheckable(true);
    m_actLangEn->setChecked(LocalizationManager::instance().currentLanguage() == Language::English);
    m_actLangEn->setData(static_cast<int>(Language::English));
    m_langGroup->addAction(m_actLangEn);
    connect(m_actLangEn, &QAction::triggered, this, &MainWindow::onLanguageActionTriggered);

    m_menuTheme = m_menuSettings->addMenu(trText(QStringLiteral("menu.theme")));
    m_themeGroup = new QActionGroup(this);
    m_themeGroup->setExclusive(true);

    m_actThemeCatppuccin = m_menuTheme->addAction(trText(QStringLiteral("theme.catppuccin")));
    m_actThemeCatppuccin->setCheckable(true);
    m_actThemeCatppuccin->setChecked(ThemeManager::currentTheme() == ThemeType::DarkCatppuccin);
    m_actThemeCatppuccin->setData(static_cast<int>(ThemeType::DarkCatppuccin));
    m_themeGroup->addAction(m_actThemeCatppuccin);
    connect(m_actThemeCatppuccin, &QAction::triggered, this, &MainWindow::onThemeActionTriggered);

    m_actThemeLight = m_menuTheme->addAction(trText(QStringLiteral("theme.light")));
    m_actThemeLight->setCheckable(true);
    m_actThemeLight->setChecked(ThemeManager::currentTheme() == ThemeType::LightModern);
    m_actThemeLight->setData(static_cast<int>(ThemeType::LightModern));
    m_themeGroup->addAction(m_actThemeLight);
    connect(m_actThemeLight, &QAction::triggered, this, &MainWindow::onThemeActionTriggered);

    m_actThemeSolarized = m_menuTheme->addAction(trText(QStringLiteral("theme.solarized")));
    m_actThemeSolarized->setCheckable(true);
    m_actThemeSolarized->setChecked(ThemeManager::currentTheme() == ThemeType::SolarizedDark);
    m_actThemeSolarized->setData(static_cast<int>(ThemeType::SolarizedDark));
    m_themeGroup->addAction(m_actThemeSolarized);
    connect(m_actThemeSolarized, &QAction::triggered, this, &MainWindow::onThemeActionTriggered);

    m_actThemeMonokai = m_menuTheme->addAction(trText(QStringLiteral("theme.monokai")));
    m_actThemeMonokai->setCheckable(true);
    m_actThemeMonokai->setChecked(ThemeManager::currentTheme() == ThemeType::MonokaiPro);
    m_actThemeMonokai->setData(static_cast<int>(ThemeType::MonokaiPro));
    m_themeGroup->addAction(m_actThemeMonokai);
    connect(m_actThemeMonokai, &QAction::triggered, this, &MainWindow::onThemeActionTriggered);

    // 7. 帮助菜单 (Help)
    m_menuHelp = mBar->addMenu(trText(QStringLiteral("menu.help")));
    m_actManual = m_menuHelp->addAction(QIcon(QStringLiteral(":/icons/user_manual.svg")), trText(QStringLiteral("act.user_manual")), this, &MainWindow::onActionUserManual);
    m_actManual->setShortcut(QKeySequence::HelpContents); // F1

    m_menuHelp->addSeparator();
    m_actViewLog = m_menuHelp->addAction(QIcon(QStringLiteral(":/icons/system_log.svg")), trText(QStringLiteral("act.view_log")), this, &MainWindow::onActionViewLogFile);
    m_actViewLog->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));

    m_actOpenLogDir = m_menuHelp->addAction(QIcon(QStringLiteral(":/icons/import_dir.svg")), trText(QStringLiteral("act.open_log_dir")), this, &MainWindow::onActionOpenLogDir);

    m_menuHelp->addSeparator();
    m_actAbout = m_menuHelp->addAction(QIcon(QStringLiteral(":/icons/app_icon.svg")), trText(QStringLiteral("act.about")), this, &MainWindow::onActionAbout);

    // 8. 主控制工具栏 (Requirement 14: 未打开工程时仅显示工程管理中心与打开工程)
    auto* tBar = addToolBar(QStringLiteral("Main Controls"));
    tBar->setMovable(false);
    tBar->setIconSize(QSize(18, 18));
    tBar->setStyleSheet(QStringLiteral(
        "QToolBar { spacing: 3px; padding: 2px 4px; }"
        "QToolButton { margin: 0px 1px; padding: 3px 5px; outline: none; }"
        "QToolBar::separator { width: 1px; margin: 3px 4px; background-color: #313244; }"
    ));

    tBar->addAction(m_actProjectHub);
    tBar->addAction(m_actOpenProject);

    auto addDep = [this, tBar](QAction* act) {
        tBar->addAction(act);
        m_toolbarProjectDependentActions.append(act);
    };

    auto addSep = [this, tBar]() {
        auto* sep = tBar->addSeparator();
        m_toolbarProjectDependentActions.append(sep);
    };

    addSep();
    addDep(m_actOpen);
    addDep(m_actImportDir);
    addDep(m_actSave);
    addSep();
    addDep(m_actStatic);
    addDep(m_actInst);
    addDep(m_actStackAnalysis);
    addSep();
    addDep(m_actStim);
    addDep(m_actCoverageAnalysis);
    addDep(m_actSim);
    addSep();

    m_lblFileSwitcher = new QLabel(QStringLiteral(" ") + trText(QStringLiteral("lbl.active_file")) + QStringLiteral(" "), this);
    m_lblFileSwitcher->setStyleSheet(QStringLiteral("font-weight: bold; color: #89b4fa;"));
    tBar->addWidget(m_lblFileSwitcher);

    m_comboFileSwitcher = new QComboBox(this);
    m_comboFileSwitcher->setMinimumWidth(230);
    m_comboFileSwitcher->setMaximumWidth(320);
    m_comboFileSwitcher->setToolTip(QStringLiteral("在当前工程/目录各文件之间快速切换"));
    connect(m_comboFileSwitcher, QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::onFileSwitcherActivated);
    tBar->addWidget(m_comboFileSwitcher);

    addSep();
    addDep(m_actExport);
    addDep(m_actManual);

    // 受工程存在性控制的互斥动作列表 (Requirements 13 & 14)
    m_projectDependentActions = {
        m_actSaveProject,
        m_actCloseProject,
        m_actOpen,
        m_actImportDir,
        m_actSave,
        m_actExport,
        m_actStatic,
        m_actInst,
        m_actStackAnalysis,
        m_actCoverageAnalysis,
        m_actSim,
        m_actStim
    };
}

void MainWindow::setupDocks() {
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks);
    setDockNestingEnabled(true);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // 1. 左侧停靠区: 结构化工程与源码树
    m_dockLeft = new QDockWidget(QStringLiteral("工程与资源结构 (Project Explorer)"), this);
    m_dockLeft->setObjectName(QStringLiteral("DockProjectFiles"));
    auto* leftContainer = new QWidget(m_dockLeft);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(4, 4, 4, 4);
    leftLayout->setSpacing(4);

    m_fileTree = new QTreeWidget(leftContainer);
    m_fileTree->setHeaderLabels({QStringLiteral("模块 / 文件名称"), QStringLiteral("属性类别")});
    m_fileTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_fileTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTree->setContextMenuPolicy(Qt::CustomContextMenu);
    leftLayout->addWidget(m_fileTree, 1);

    m_dockLeft->setWidget(leftContainer);
    addDockWidget(Qt::LeftDockWidgetArea, m_dockLeft);

    connect(m_fileTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::onFileTreeContextMenu);

    // 2. 中央三大功能面板 (支持自由拖拽浮动与 Tab 化)
    // (0) 欢迎引导与工程管理中心 (Requirement 1: 启动默认进入欢迎向导页)
    m_welcomeView = new WelcomeView(this);
    m_dockWelcome = new QDockWidget(QStringLiteral("🌟 欢迎向导 (Welcome)"), this);
    m_dockWelcome->setObjectName(QStringLiteral("DockWelcome"));
    m_dockWelcome->setWidget(m_welcomeView);
    addDockWidget(Qt::TopDockWidgetArea, m_dockWelcome);

    // (a) 多文件选项卡代码编辑器 (Requirement 3)
    m_dockCodeEditor = new QDockWidget(trText(QStringLiteral("dock.code_editor")), this);
    m_dockCodeEditor->setObjectName(QStringLiteral("DockCodeEditor"));
    m_dockCodeEditor->setWidget(m_editorTabs);
    addDockWidget(Qt::TopDockWidgetArea, m_dockCodeEditor);

    // (b) 静态分析与路径探索工作台
    m_dockStaticExplorer = new QDockWidget(trText(QStringLiteral("dock.static_explorer")), this);
    m_dockStaticExplorer->setObjectName(QStringLiteral("DockStaticExplorer"));
    m_dockStaticExplorer->setWidget(m_staticExplorer);
    addDockWidget(Qt::TopDockWidgetArea, m_dockStaticExplorer);

    // (c) Graphviz 拓扑调用图
    m_dockGraphView = new QDockWidget(trText(QStringLiteral("dock.graph_view")), this);
    m_dockGraphView->setObjectName(QStringLiteral("DockGraphView"));
    m_dockGraphView->setWidget(m_graphView);
    addDockWidget(Qt::TopDockWidgetArea, m_dockGraphView);

    // (d) 静态堆栈与二进制调用深度分析器 (Requirement 15)
    m_stackAnalysisView = new StackAnalysisView(this);
    m_dockStackAnalysis = new QDockWidget(QStringLiteral("静态堆栈与调用深度分析 (Stack Analyzer)"), this);
    m_dockStackAnalysis->setObjectName(QStringLiteral("DockStackAnalysis"));
    m_dockStackAnalysis->setWidget(m_stackAnalysisView);
    addDockWidget(Qt::TopDockWidgetArea, m_dockStackAnalysis);

    // (e) 独立测试覆盖分析工作台 (Requirement 3: 覆盖测试分析作为单独功能点)
    m_coverageView = new CoverageView(this);
    m_dockCoverageView = new QDockWidget(QStringLiteral("测试覆盖分析 (Coverage Analysis)"), this);
    m_dockCoverageView->setObjectName(QStringLiteral("DockCoverageAnalysis"));
    m_dockCoverageView->setWidget(m_coverageView);
    addDockWidget(Qt::TopDockWidgetArea, m_dockCoverageView);

    tabifyDockWidget(m_dockCodeEditor, m_dockWelcome);
    tabifyDockWidget(m_dockWelcome, m_dockStaticExplorer);
    tabifyDockWidget(m_dockStaticExplorer, m_dockGraphView);
    tabifyDockWidget(m_dockGraphView, m_dockStackAnalysis);
    tabifyDockWidget(m_dockStackAnalysis, m_dockCoverageView);
    m_dockWelcome->raise(); // 启动默认进入欢迎向导页 (Requirement 1)

    // 3. 右侧停靠区: Python 激励注入与向量监控工作台
    m_dockRight = new QDockWidget(QStringLiteral("Python 激励生成与注入 (Stimulus Studio)"), this);
    m_dockRight->setObjectName(QStringLiteral("DockStimulusStudio"));
    m_stimulusView = new StimulusView(m_dockRight);
    m_dockRight->setWidget(m_stimulusView);
    addDockWidget(Qt::RightDockWidgetArea, m_dockRight);

    // 4. 底部统一停靠区: 静态缺陷列表 + 运行实时日志合并在同一 DockWidget (Requirement 2)
    m_dockBottom = new QDockWidget(QStringLiteral("静态缺陷列表与系统运行日志"), this);
    m_dockBottom->setObjectName(QStringLiteral("DockBottomIssuesAndLogs"));

    m_bottomTabs = new QTabWidget(m_dockBottom);
    m_staticView = new StaticAnalysisView(m_bottomTabs);
    m_systemLog = new QTextEdit(m_bottomTabs);
    m_systemLog->setReadOnly(true);

    m_bottomTabs->addTab(m_staticView, QIcon(QStringLiteral(":/icons/static_analysis.svg")), trText(QStringLiteral("dock.static_issues")));
    m_bottomTabs->addTab(m_systemLog, QIcon(QStringLiteral(":/icons/system_log.svg")), trText(QStringLiteral("dock.system_log")));
    m_dockBottom->setWidget(m_bottomTabs);

    addDockWidget(Qt::BottomDockWidgetArea, m_dockBottom);

    resizeDocks({m_dockLeft, m_dockCodeEditor, m_dockRight}, {260, 780, 360}, Qt::Horizontal);
    resizeDocks({m_dockCodeEditor, m_dockBottom}, {560, 240}, Qt::Vertical);

    setupDockTabBarContextMenu();
}

void MainWindow::setupDockTabBarContextMenu() {
    const auto tabBars = findChildren<QTabBar*>();
    for (auto* bar : tabBars) {
        if (!bar) continue;
        if (bar->parentWidget() == m_editorTabs ||
            bar->parentWidget() == m_rightTabs ||
            bar->parentWidget() == m_bottomTabs) {
            continue;
        }
        bar->setContextMenuPolicy(Qt::CustomContextMenu);
        disconnect(bar, &QTabBar::customContextMenuRequested, this, nullptr);
        connect(bar, &QTabBar::customContextMenuRequested, this, [this, bar](const QPoint& pt) {
            onDockTabBarContextMenu(bar, pt);
        });
    }
}

void MainWindow::onDockTabBarContextMenu(QTabBar* bar, const QPoint& pt) {
    if (!bar) return;
    int index = bar->tabAt(pt);
    if (index < 0) return;

    QString tabTitle = bar->tabText(index);

    QList<QDockWidget*> centralDocks = {m_dockWelcome, m_dockCodeEditor, m_dockStaticExplorer, m_dockGraphView, m_dockStackAnalysis, m_dockCoverageView};
    QDockWidget* clickedDock = nullptr;
    for (auto* d : centralDocks) {
        if (d && (d->windowTitle() == tabTitle || tabTitle.contains(d->windowTitle()) || d->windowTitle().contains(tabTitle))) {
            clickedDock = d;
            break;
        }
    }

    QMenu menu(this);
    auto* actClose = menu.addAction(QIcon(QStringLiteral(":/icons/delete.svg")),
                                    QStringLiteral("关闭当前页 (%1)").arg(tabTitle));
    auto* actCloseOthers = menu.addAction(QStringLiteral("关闭除当前外所有页"));
    auto* actCloseAll = menu.addAction(QStringLiteral("关闭所有页面"));

    connect(actClose, &QAction::triggered, this, [clickedDock]() {
        if (clickedDock) clickedDock->hide();
    });

    connect(actCloseOthers, &QAction::triggered, this, [clickedDock, centralDocks]() {
        for (auto* d : centralDocks) {
            if (d && d != clickedDock) {
                d->hide();
            }
        }
    });

    connect(actCloseAll, &QAction::triggered, this, [centralDocks]() {
        for (auto* d : centralDocks) {
            if (d) d->hide();
        }
    });

    menu.exec(bar->mapToGlobal(pt));
}

void MainWindow::setupConnections() {
    auto& backend = BackendService::instance();

    // 后端信号 -> 界面联动
    connect(&backend, &BackendService::sourceFileLoaded,
            this, &MainWindow::onSourceFileLoaded);
    connect(&backend, &BackendService::staticAnalysisFinished,
            this, &MainWindow::onStaticAnalysisFinished);
    connect(&backend, &BackendService::coverageReportUpdated,
            this, &MainWindow::onCoverageReportUpdated);
    connect(&backend, &BackendService::stimulusPlanReady,
            this, &MainWindow::onStimulusPlanReady);
    connect(&backend, &BackendService::instrumentationFinished,
            this, &MainWindow::onInstrumentationFinished);
    connect(&backend, &BackendService::logMessage,
            this, &MainWindow::onBackendLog);
    connect(&backend, &BackendService::stimulusScriptOutput,
            m_stimulusView, &StimulusView::appendLog);

    // 激励面板请求 -> 后端
    connect(m_stimulusView, &StimulusView::runScriptRequested,
            &backend, &BackendService::runStimulusScript);
    connect(m_stimulusView, &StimulusView::runCodeContentRequested,
            &backend, [](const QString& code, const QStringList& args) {
                QString err;
                BackendService::instance().runStimulusCodeContent(code, args, &err);
            });
    connect(m_stimulusView, &StimulusView::simulateCoverageRequested,
            &backend, &BackendService::runSimulationWithLatestStimulus);
    connect(m_stimulusView, &StimulusView::pythonPathChanged,
            &backend.stimulusEngine(), &StimulusEngine::setPythonExecutable);

    // 欢迎界面联动 (Requirement 1)
    if (m_welcomeView) {
        connect(m_welcomeView, &WelcomeView::projectSelected,
                this, &MainWindow::openProjectFile);
        connect(m_welcomeView, &WelcomeView::newProjectRequested,
                this, &MainWindow::onActionNewProject);
        connect(m_welcomeView, &WelcomeView::openProjectRequested,
                this, &MainWindow::onActionOpenProject);
        connect(m_welcomeView, &WelcomeView::openFileRequested,
                this, &MainWindow::onActionOpenFile);
        connect(m_welcomeView, &WelcomeView::openSampleMotorRequested,
                this, &MainWindow::onOpenSampleMotor);
        connect(m_welcomeView, &WelcomeView::openSampleSensorRequested,
                this, &MainWindow::onOpenSampleSensor);
        connect(m_welcomeView, &WelcomeView::openManualRequested,
                this, &MainWindow::onActionUserManual);
    }

    // 跨组件联动
    connect(m_staticView, &StaticAnalysisView::issueSelected,
            this, &MainWindow::onIssueSelectedInEditor);
    connect(m_coverageView, &CoverageView::lineSelected,
            this, &MainWindow::onCoverageLineSelectedInEditor);
    connect(m_fileTree, &QTreeWidget::itemDoubleClicked,
            this, &MainWindow::onFileTreeItemDoubleClicked);
    connect(m_graphView, &GraphView::nodeSelected,
            this, &MainWindow::onGraphNodeSelected);

    // 静态分析探索工作台联动
    connect(m_staticExplorer, &StaticAnalysisExplorerView::runAnalysisRequested,
            this, [this](const QString& path) {
                if (!path.isEmpty() && path != BackendService::instance().currentSourceFile()) {
                    loadFileIntoWorkspace(path);
                } else if (auto* ed = currentActiveEditor()) {
                    BackendService::instance().setSourceContent(ed->toPlainText(), path);
                }
                BackendService::instance().runStaticAnalysis();
            });
    connect(m_staticExplorer, &StaticAnalysisExplorerView::jumpToEditorLine,
            this, [this](int line, int col) {
                if (auto* ed = currentActiveEditor()) {
                    ed->gotoLine(line, col);
                }
            });

    // 静态分析面板右键关闭 (Requirement 3)
    connect(m_staticExplorer, &QWidget::customContextMenuRequested,
            this, &MainWindow::onStaticExplorerContextMenu);

    // 多文件选项卡事件 (Requirement 3)
    connect(m_editorTabs, &QTabWidget::tabCloseRequested,
            this, &MainWindow::onEditorTabCloseRequested);
    connect(m_editorTabs, &QTabWidget::customContextMenuRequested,
            this, &MainWindow::onEditorTabContextMenu);
    connect(m_editorTabs, &QTabWidget::currentChanged,
            this, &MainWindow::onEditorCurrentChanged);

    // 监听静态分析工作区显示状态，实现右侧面板联动隐藏与恢复
    connect(m_dockStaticExplorer, &QDockWidget::visibilityChanged,
            this, &MainWindow::onWorkspaceDockVisibilityChanged);

    // 本地化与主题动态变化监听
    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, [this](Language /*lang*/) {
                retranslateUi();
            });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
}

// -----------------------------------------------------------------------------
// 工程管理逻辑 (Requirement 6)
// -----------------------------------------------------------------------------
void MainWindow::onActionNewProject() {
    ProjectHubDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (!dlg.selectedProjectPath().isEmpty()) {
            openProjectFile(dlg.selectedProjectPath());
        }
    }
}

void MainWindow::onActionOpenProject() {
    QString p = QFileDialog::getOpenFileName(this, QStringLiteral("打开嵌入式工程"),
                                            QDir::currentPath(),
                                            QStringLiteral("Coverage Project (*.covproj);;All Files (*.*)"));
    if (!p.isEmpty()) {
        openProjectFile(p);
    }
}

void MainWindow::onActionSaveProject() {
    if (ProjectManager::instance().saveCurrentProject()) {
        statusBar()->showMessage(QStringLiteral("工程已成功保存: ") + ProjectManager::instance().currentProject().filePath, 3000);
    }
}

void MainWindow::onActionProjectHub() {
    ProjectHubDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (!dlg.selectedProjectPath().isEmpty()) {
            openProjectFile(dlg.selectedProjectPath());
        }
    }
}

void MainWindow::onActionCloseProject() {
    ProjectManager::instance().closeCurrentProject();
    while (m_editorTabs->count() > 0) {
        closeEditorTab(0);
    }
    m_fileTree->clear();
    m_comboFileSwitcher->clear();
    m_importedFiles.clear();
    setWindowTitle(QStringLiteral("Embedded C Coverage Studio (未打开工程)"));
    statusBar()->showMessage(QStringLiteral("工程已关闭。"), 3000);

    if (m_stimulusView) {
        m_stimulusView->reloadProjectScripts();
    }

    // Requirement 13 & 14: 恢复到项目未打开时的状态
    updateProjectDependentUiState(false);
}

void MainWindow::onOpenSampleMotor() {
    if (ProjectManager::instance().openBuiltinSample(QStringLiteral("motor_controller"))) {
        openProjectFile(ProjectManager::instance().currentProject().filePath);
    }
}

void MainWindow::onOpenSampleSensor() {
    if (ProjectManager::instance().openBuiltinSample(QStringLiteral("sensor_fusion"))) {
        openProjectFile(ProjectManager::instance().currentProject().filePath);
    }
}

void MainWindow::onOpenRecentProject(const QString& path) {
    if (QFile::exists(path)) {
        openProjectFile(path);
    } else {
        QMessageBox::warning(this, QStringLiteral("文件不存在"), QStringLiteral("指定的工程文件已不存在。"));
        ProjectManager::instance().removeRecentProject(path);
        refreshRecentProjectsMenu();
    }
}

void MainWindow::openProjectFile(const QString& covprojPath) {
    bool ok = ProjectManager::instance().openProject(covprojPath);
    if (!ok && !ProjectManager::instance().hasActiveProject()) {
        return;
    }

    const auto& proj = ProjectManager::instance().currentProject();
    setWindowTitle(QString("Embedded C Coverage Studio - [%1.covproj - %2]")
                       .arg(proj.name, proj.targetArch));

    // Requirement 11: 当项目打开后，关闭欢迎向导页
    // Requirements 13 & 14: 恢复菜单与工具栏所有功能按钮并显示主工作区
    updateProjectDependentUiState(true);

    updateProjectTree();
    if (m_stimulusView) {
        m_stimulusView->reloadProjectScripts();
    }
    refreshRecentProjectsMenu();
    if (m_welcomeView) {
        m_welcomeView->refreshRecentProjects();
    }
    setupDockTabBarContextMenu();

    // 自动检索工程目录下的可执行程序并设置到静态堆栈分析器中 (Requirement 15)
    QFileInfo projFi(covprojPath);
    QString projDir = projFi.absolutePath();
    QString base = projFi.baseName();
    QStringList candBins = {
        projDir + QStringLiteral("/") + base + QStringLiteral(".exe"),
        projDir + QStringLiteral("/bin/") + base + QStringLiteral(".exe"),
        projDir + QStringLiteral("/build/") + base + QStringLiteral(".exe"),
        projDir + QStringLiteral("/") + base + QStringLiteral(".elf"),
        projDir + QStringLiteral("/bin/") + base + QStringLiteral(".elf"),
        projDir + QStringLiteral("/build/") + base + QStringLiteral(".elf"),
        projDir + QStringLiteral("/Source/") + base + QStringLiteral(".o")
    };
    QString foundBin;
    for (const auto& b : candBins) {
        if (QFile::exists(b)) {
            foundBin = b;
            break;
        }
    }
    if (foundBin.isEmpty()) {
        QDirIterator it(projDir, QStringList() << QStringLiteral("*.exe") << QStringLiteral("*.elf") << QStringLiteral("*.o"), QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext()) {
            foundBin = it.next();
        }
    }
    if (!foundBin.isEmpty() && m_stackAnalysisView) {
        m_stackAnalysisView->setBinaryPath(foundBin);
    }

    // 载入活动文件或第一个源文件
    QString fileToLoad = proj.activeFile;
    if (fileToLoad.isEmpty() && !proj.sourceFiles.isEmpty()) {
        fileToLoad = proj.sourceFiles.first();
    }

    if (!fileToLoad.isEmpty() && QFile::exists(fileToLoad)) {
        loadFileIntoWorkspace(fileToLoad);
    }
}

void MainWindow::updateProjectTree() {
    m_fileTree->clear();
    m_comboFileSwitcher->clear();
    m_importedFiles.clear();

    if (!ProjectManager::instance().hasActiveProject()) {
        auto* item = new QTreeWidgetItem(m_fileTree, {QStringLiteral("暂无打开的工程 (请点击工程菜单新建或打开)"), QStringLiteral("None")});
        item->setForeground(0, QColor(QStringLiteral("#a6adc8")));
        return;
    }

    const auto& proj = ProjectManager::instance().currentProject();

    // 1. 工程根节点
    auto* rootProj = new QTreeWidgetItem(m_fileTree, {
        QString("[%1.covproj] %2").arg(proj.name, proj.targetArch),
        QStringLiteral("Project")
    });
    rootProj->setIcon(0, QIcon(QStringLiteral(":/icons/project.svg")));
    rootProj->setData(0, Qt::UserRole, proj.filePath);
    rootProj->setData(1, Qt::DisplayRole, QStringLiteral("Project"));
    rootProj->setExpanded(true);
    QFont fRoot = rootProj->font(0);
    fRoot.setBold(true);
    rootProj->setFont(0, fRoot);

    // 2. 源文件组 (*.c, *.cpp) - Requirement 8: 名称应该为 "源文件"
    auto* groupSrc = new QTreeWidgetItem(rootProj, {QStringLiteral("源文件"), QStringLiteral("Category")});
    groupSrc->setIcon(0, QIcon(QStringLiteral(":/icons/folder.svg")));
    groupSrc->setData(1, Qt::DisplayRole, QStringLiteral("Category"));
    groupSrc->setExpanded(true);

    // 3. 头文件组 (*.h) - Requirement 8: 名称应该为 "头文件"
    auto* groupHdr = new QTreeWidgetItem(rootProj, {QStringLiteral("头文件"), QStringLiteral("Category")});
    groupHdr->setIcon(0, QIcon(QStringLiteral(":/icons/folder.svg")));
    groupHdr->setData(1, Qt::DisplayRole, QStringLiteral("Category"));
    groupHdr->setExpanded(true);

    for (const QString& sf : proj.sourceFiles) {
        if (!QFile::exists(sf)) continue;
        m_importedFiles.append(sf);
        QFileInfo fi(sf);

        if (sf.endsWith(QStringLiteral(".h"), Qt::CaseInsensitive)) {
            auto* item = new QTreeWidgetItem(groupHdr, {fi.fileName(), QStringLiteral("C Header")});
            item->setIcon(0, QIcon(QStringLiteral(":/icons/h_file.svg"))); // Requirement 3 & 9
            item->setData(0, Qt::UserRole, sf);
            item->setData(1, Qt::DisplayRole, QStringLiteral("C Header"));
            item->setToolTip(0, sf);
        } else {
            auto* item = new QTreeWidgetItem(groupSrc, {fi.fileName(), QStringLiteral("C Source")});
            item->setIcon(0, QIcon(QStringLiteral(":/icons/c_file.svg"))); // Requirement 3 & 9
            item->setData(0, Qt::UserRole, sf);
            item->setData(1, Qt::DisplayRole, QStringLiteral("C Source"));
            item->setToolTip(0, sf);
        }
        m_comboFileSwitcher->addItem(fi.fileName(), sf);
    }

    // 4. 脚本文件组 (*.py) - Requirement 8: 名称应该为 "脚本文件"
    auto* groupPy = new QTreeWidgetItem(rootProj, {QStringLiteral("脚本文件"), QStringLiteral("Category")});
    groupPy->setIcon(0, QIcon(QStringLiteral(":/icons/folder.svg")));
    groupPy->setData(1, Qt::DisplayRole, QStringLiteral("Category"));
    groupPy->setExpanded(true);

    for (const QString& sc : proj.scriptFiles) {
        if (!QFile::exists(sc)) continue;
        QFileInfo fi(sc);
        auto* item = new QTreeWidgetItem(groupPy, {fi.fileName(), QStringLiteral("Python Script")});
        item->setIcon(0, QIcon(QStringLiteral(":/icons/py_file.svg"))); // Requirement 3 & 9
        item->setData(0, Qt::UserRole, sc);
        item->setData(1, Qt::DisplayRole, QStringLiteral("Python Script"));
        item->setToolTip(0, sc);
    }

    // 5. 报告文件组 (*.html) - Requirement 1: 默认加载 html 格式源文件
    auto* groupRep = new QTreeWidgetItem(rootProj, {QStringLiteral("报告文件"), QStringLiteral("Category")});
    groupRep->setIcon(0, QIcon(QStringLiteral(":/icons/folder.svg")));
    groupRep->setData(1, Qt::DisplayRole, QStringLiteral("Category"));
    groupRep->setExpanded(true);

    QFileInfo projFi(proj.filePath);
    QString projDir = projFi.absolutePath();
    QStringList repDirs = {
        projDir + QStringLiteral("/Report"),
        projDir + QStringLiteral("/reports"),
        QDir::currentPath() + QStringLiteral("/reports")
    };

    QStringList addedReports;
    QStringList repFilters = {
        QStringLiteral("*.html"), QStringLiteral("*.htm"),
        QStringLiteral("*.pdf"),
        QStringLiteral("*.docx"), QStringLiteral("*.doc"),
        QStringLiteral("*.json"),
        QStringLiteral("*.txt")
    };
    for (const auto& d : repDirs) {
        if (!QDir(d).exists()) continue;
        QDirIterator repIt(d, repFilters, QDir::Files);
        while (repIt.hasNext()) {
            QString repPath = repIt.next();
            if (addedReports.contains(repPath)) continue;
            addedReports.append(repPath);
            QFileInfo rfi(repPath);
            QString ext = rfi.suffix().toLower();
            QString typeDesc = QStringLiteral("HTML 报告");
            QString iconPath = QStringLiteral(":/icons/html_file.svg");
            if (ext == QStringLiteral("pdf")) {
                typeDesc = QStringLiteral("PDF 报告");
                iconPath = QStringLiteral(":/icons/export_report.svg");
            } else if (ext == QStringLiteral("docx") || ext == QStringLiteral("doc")) {
                typeDesc = QStringLiteral("Word 报告");
                iconPath = QStringLiteral(":/icons/export_report.svg");
            } else if (ext == QStringLiteral("json")) {
                typeDesc = QStringLiteral("JSON 报告");
                iconPath = QStringLiteral(":/icons/path_explorer.svg");
            }
            auto* item = new QTreeWidgetItem(groupRep, {rfi.fileName(), typeDesc});
            item->setIcon(0, QIcon(iconPath));
            item->setData(0, Qt::UserRole, repPath);
            item->setData(1, Qt::DisplayRole, QStringLiteral("Report"));
            item->setToolTip(0, repPath);
        }
    }

    if (addedReports.isEmpty()) {
        QString reportFolder = projDir + QStringLiteral("/Report");
        QDir().mkpath(reportFolder);
        QString defRepPath = reportFolder + QStringLiteral("/") + proj.name + QStringLiteral("_report.html");

        QString htmlContent = ReportGenerator::generateHtmlReport(
            BackendService::instance().latestStaticReport(),
            BackendService::instance().latestCoverageReport(),
            proj.sourceFiles.isEmpty() ? QString() : proj.sourceFiles.first(),
            QString(),
            BackendService::instance().latestStimulusPlan()
        );
        QFile rf(defRepPath);
        if (rf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&rf);
            out << htmlContent;
            rf.close();

            QFileInfo rfi(defRepPath);
            auto* item = new QTreeWidgetItem(groupRep, {rfi.fileName(), QStringLiteral("HTML 报告")});
            item->setIcon(0, QIcon(QStringLiteral(":/icons/html_file.svg")));
            item->setData(0, Qt::UserRole, defRepPath);
            item->setData(1, Qt::DisplayRole, QStringLiteral("Report"));
            item->setToolTip(0, defRepPath);
        }
    }
}

void MainWindow::refreshRecentProjectsMenu() {
    if (!m_menuRecentProjects) return;
    m_menuRecentProjects->clear();

    const QStringList recents = ProjectManager::instance().recentProjects();
    if (recents.isEmpty()) {
        auto* actNone = m_menuRecentProjects->addAction(QStringLiteral("暂无历史工程记录"));
        actNone->setEnabled(false);
        return;
    }

    for (const QString& p : recents) {
        QFileInfo fi(p);
        auto* act = m_menuRecentProjects->addAction(QString("%1 (%2)").arg(fi.baseName(), fi.path()));
        connect(act, &QAction::triggered, this, [this, p]() {
            onOpenRecentProject(p);
        });
    }

    m_menuRecentProjects->addSeparator();
    auto* actClear = m_menuRecentProjects->addAction(QStringLiteral("清空历史记录"));
    connect(actClear, &QAction::triggered, this, [this]() {
        ProjectManager::instance().clearRecentProjects();
        refreshRecentProjectsMenu();
    });
}

void MainWindow::retranslateUi() {
    if (ProjectManager::instance().hasActiveProject()) {
        const auto& proj = ProjectManager::instance().currentProject();
        setWindowTitle(QString("Embedded C Coverage Studio - [%1.covproj - %2]").arg(proj.name, proj.targetArch));
    } else {
        setWindowTitle(trText(QStringLiteral("app.title")));
    }

    if (m_menuProject) m_menuProject->setTitle(QStringLiteral("工程 (&P)"));
    if (m_menuFile) m_menuFile->setTitle(trText(QStringLiteral("menu.file")));
    if (m_menuAnalysis) m_menuAnalysis->setTitle(trText(QStringLiteral("menu.analysis")));
    if (m_menuStimulus) m_menuStimulus->setTitle(trText(QStringLiteral("menu.stimulus")));
    if (m_menuView) m_menuView->setTitle(trText(QStringLiteral("menu.view")));
    if (m_menuSettings) m_menuSettings->setTitle(trText(QStringLiteral("menu.settings")));
    if (m_menuLanguage) m_menuLanguage->setTitle(trText(QStringLiteral("menu.language")));
    if (m_menuTheme) m_menuTheme->setTitle(trText(QStringLiteral("menu.theme")));
    if (m_menuHelp) m_menuHelp->setTitle(trText(QStringLiteral("menu.help")));

    if (m_actOpen) m_actOpen->setText(trText(QStringLiteral("act.open_file")));
    if (m_actImportDir) m_actImportDir->setText(trText(QStringLiteral("act.import_dir")));
    if (m_actSave) m_actSave->setText(trText(QStringLiteral("act.save")));
    if (m_actExport) m_actExport->setText(trText(QStringLiteral("act.export_report")));
    if (m_actExit) m_actExit->setText(trText(QStringLiteral("act.exit")));
    if (m_actStatic) m_actStatic->setText(trText(QStringLiteral("act.run_static")));
    if (m_actInst) m_actInst->setText(trText(QStringLiteral("act.instrument")));
    if (m_actStim) m_actStim->setText(trText(QStringLiteral("act.run_stimulus")));
    if (m_actSim) m_actSim->setText(trText(QStringLiteral("act.run_sim")));
    if (m_actManual) m_actManual->setText(trText(QStringLiteral("act.user_manual")));
    if (m_actViewLog) m_actViewLog->setText(trText(QStringLiteral("act.view_log")));
    if (m_actOpenLogDir) m_actOpenLogDir->setText(trText(QStringLiteral("act.open_log_dir")));
    if (m_actAbout) m_actAbout->setText(trText(QStringLiteral("act.about")));
    if (m_actResetLayout) m_actResetLayout->setText(trText(QStringLiteral("act.reset_layout")));

    if (m_dockLeft) m_dockLeft->setWindowTitle(QStringLiteral("工程与资源结构 (Project Explorer)"));
    if (m_dockWelcome) m_dockWelcome->setWindowTitle(QStringLiteral("🌟 欢迎向导 (Welcome)"));
    if (m_dockCodeEditor) {
        QString currentFile = BackendService::instance().currentSourceFile();
        if (currentFile.isEmpty()) {
            m_dockCodeEditor->setWindowTitle(trText(QStringLiteral("dock.code_editor")));
        } else {
            m_dockCodeEditor->setWindowTitle(trText(QStringLiteral("dock.code_editor")) + QStringLiteral(" - ") + QFileInfo(currentFile).fileName());
        }
    }
    if (m_dockStaticExplorer) m_dockStaticExplorer->setWindowTitle(trText(QStringLiteral("dock.static_explorer")));
    if (m_dockGraphView) m_dockGraphView->setWindowTitle(trText(QStringLiteral("dock.graph_view")));
    if (m_dockStackAnalysis) m_dockStackAnalysis->setWindowTitle(QStringLiteral("静态堆栈与调用深度分析 (Stack Analyzer)"));
    if (m_dockCoverageView) m_dockCoverageView->setWindowTitle(QStringLiteral("测试覆盖分析 (Coverage Analysis)"));
    if (m_dockRight) m_dockRight->setWindowTitle(QStringLiteral("Python 激励生成与注入 (Stimulus Studio)"));
    if (m_dockBottom) m_dockBottom->setWindowTitle(QStringLiteral("静态缺陷列表与系统运行日志"));

    if (m_bottomTabs) {
        m_bottomTabs->setTabText(0, trText(QStringLiteral("dock.static_issues")));
        m_bottomTabs->setTabText(1, trText(QStringLiteral("dock.system_log")));
    }

    if (m_staticExplorer) {
        m_staticExplorer->retranslateUi();
    }

    if (m_coverageView) {
        m_coverageView->retranslateUi();
    }

    if (m_lblFileSwitcher) {
        m_lblFileSwitcher->setText(QStringLiteral(" ") + trText(QStringLiteral("lbl.active_file")) + QStringLiteral(" "));
    }
}

void MainWindow::onActionResetLayout() {
    addDockWidget(Qt::LeftDockWidgetArea, m_dockLeft);
    addDockWidget(Qt::TopDockWidgetArea, m_dockWelcome);
    addDockWidget(Qt::TopDockWidgetArea, m_dockCodeEditor);
    addDockWidget(Qt::TopDockWidgetArea, m_dockStaticExplorer);
    addDockWidget(Qt::TopDockWidgetArea, m_dockGraphView);
    addDockWidget(Qt::TopDockWidgetArea, m_dockStackAnalysis);
    addDockWidget(Qt::TopDockWidgetArea, m_dockCoverageView);
    tabifyDockWidget(m_dockCodeEditor, m_dockWelcome);
    tabifyDockWidget(m_dockWelcome, m_dockStaticExplorer);
    tabifyDockWidget(m_dockStaticExplorer, m_dockGraphView);
    tabifyDockWidget(m_dockGraphView, m_dockStackAnalysis);
    tabifyDockWidget(m_dockStackAnalysis, m_dockCoverageView);
    m_dockWelcome->raise();

    addDockWidget(Qt::RightDockWidgetArea, m_dockRight);
    addDockWidget(Qt::BottomDockWidgetArea, m_dockBottom);
    m_dockBottom->raise();

    setupDockTabBarContextMenu();

    resizeDocks({m_dockLeft, m_dockCodeEditor, m_dockRight}, {260, 780, 360}, Qt::Horizontal);
    resizeDocks({m_dockCodeEditor, m_dockBottom}, {560, 240}, Qt::Vertical);
    statusBar()->showMessage(QStringLiteral("已重置为主机默认 Visual Studio 风格窗口布局"), 3000);
}

void MainWindow::onWorkspaceDockVisibilityChanged(bool visible) {
    if (visible && m_dockStaticExplorer && m_dockStaticExplorer->isVisible()) {
        if (m_dockRight && m_dockRight->isVisible()) {
            m_wasRightDockVisible = true;
            m_dockRight->hide();
        }
    } else {
        if (m_wasRightDockVisible && m_dockRight && !m_dockRight->isVisible()) {
            m_dockRight->show();
        }
    }
}

void MainWindow::onLanguageActionTriggered() {
    auto* act = qobject_cast<QAction*>(sender());
    if (!act) return;

    Language lang = static_cast<Language>(act->data().toInt());
    LocalizationManager::instance().setLanguage(lang);
}

void MainWindow::onThemeActionTriggered() {
    auto* act = qobject_cast<QAction*>(sender());
    if (!act) return;

    ThemeType theme = static_cast<ThemeType>(act->data().toInt());
    ThemeManager::applyTheme(theme);
    if (m_coverageView) {
        m_coverageView->applyTheme(theme);
    }
}

void MainWindow::updateEditorTabsStyle(ThemeType type) {
    bool isLight = (type == ThemeType::LightModern);
    QString qss = isLight ? QStringLiteral(
        "QTabWidget::pane { border: 1px solid #dadce0; background: #ffffff; border-radius: 4px; }"
        "QTabBar::tab { background: #f1f3f4; color: #5f6368; padding: 6px 14px; border: 1px solid #dadce0; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
        "QTabBar::tab:selected { background: #ffffff; color: #1a73e8; font-weight: bold; border-bottom: 2px solid #1a73e8; }"
        "QTabBar::tab:hover:!selected { background: #e8eaed; }"
    ) : QStringLiteral(
        "QTabWidget::pane { border: 1px solid #313244; background: #1e1e2e; border-radius: 4px; }"
        "QTabBar::tab { background: #181825; color: #a6adc8; padding: 6px 14px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
        "QTabBar::tab:selected { background: #1e1e2e; color: #89b4fa; font-weight: bold; border-bottom: 2px solid #89b4fa; }"
        "QTabBar::tab:hover:!selected { background: #262738; }"
    );
    if (m_editorTabs) m_editorTabs->setStyleSheet(qss);
    if (m_bottomTabs) m_bottomTabs->setStyleSheet(qss);
    if (m_rightTabs) m_rightTabs->setStyleSheet(qss);
}

void MainWindow::onThemeChanged(ThemeType type) {
    updateEditorTabsStyle(type);
    for (auto* ed : m_openEditors) {
        if (ed) ed->applyCurrentTheme();
    }
    if (m_staticExplorer) {
        m_staticExplorer->applyTheme(type);
    }
    if (m_staticView) {
        m_staticView->applyTheme(type);
    }
    if (m_welcomeView) {
        m_welcomeView->applyTheme(type);
    }
    if (m_stackAnalysisView) {
        m_stackAnalysisView->applyTheme(type);
    }
    if (m_graphView) {
        m_graphView->applyTheme(type);
    }
    if (m_coverageView) {
        m_coverageView->applyTheme(type);
    }
    if (m_stimulusView) {
        m_stimulusView->applyTheme(type);
    }

    if (m_systemLog) {
        if (type == ThemeType::LightModern) {
            m_systemLog->setStyleSheet(QStringLiteral("background: #ffffff; color: #202124; font-family: 'Consolas', monospace; font-size: 12px; border: 1px solid #dadce0;"));
        } else if (type == ThemeType::SolarizedDark) {
            m_systemLog->setStyleSheet(QStringLiteral("background: #00212b; color: #93a1a1; font-family: 'Consolas', monospace; font-size: 12px; border: 1px solid #073642;"));
        } else if (type == ThemeType::MonokaiPro) {
            m_systemLog->setStyleSheet(QStringLiteral("background: #19181a; color: #fcfcfa; font-family: 'Consolas', monospace; font-size: 12px; border: 1px solid #403e41;"));
        } else {
            m_systemLog->setStyleSheet(QStringLiteral("background: #11111b; color: #a6adc8; font-family: 'Consolas', monospace; font-size: 12px; border: 1px solid #313244;"));
        }
    }

    bool isLight = (type == ThemeType::LightModern);
    if (m_lblFileSwitcher) {
        m_lblFileSwitcher->setStyleSheet(isLight ?
            QStringLiteral("font-weight: bold; color: #1a73e8;") :
            QStringLiteral("font-weight: bold; color: #89b4fa;"));
    }
}

void MainWindow::onActionOpenFile() {
    QString path = QFileDialog::getOpenFileName(this, trText(QStringLiteral("act.open_file")),
                                                QString(), QStringLiteral("C Source Files (*.c *.h);;All Files (*.*)"));
    if (!path.isEmpty()) {
        loadFileIntoWorkspace(path);

        if (ProjectManager::instance().hasActiveProject()) {
            auto& proj = ProjectManager::instance().currentProject();
            if (!proj.sourceFiles.contains(path)) {
                proj.sourceFiles.append(path);
                proj.saveToFile();
                updateProjectTree();
            }
        }
    }
}

void MainWindow::onActionImportDirectory() {
    QString dirPath = QFileDialog::getExistingDirectory(this, trText(QStringLiteral("act.import_dir")),
                                                        m_currentDirPath.isEmpty() ? QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis") : m_currentDirPath,
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dirPath.isEmpty()) return;

    scanDirectoryAndPopulate(dirPath);
}

void MainWindow::scanDirectoryAndPopulate(const QString& dirPath) {
    m_currentDirPath = dirPath;

    // 递归检索所有源文件与头文件
    QDirIterator it(dirPath, QStringList() << QStringLiteral("*.c") << QStringLiteral("*.h") << QStringLiteral("*.cpp"),
                    QDir::Files, QDirIterator::Subdirectories);

    QString firstCFile;
    QStringList newSources;

    while (it.hasNext()) {
        QString filePath = it.next();
        newSources.append(filePath);
        if (firstCFile.isEmpty() && (filePath.endsWith(QStringLiteral(".c")) || filePath.endsWith(QStringLiteral(".cpp")))) {
            firstCFile = filePath;
        }
    }

    // 检索 Python 脚本
    QStringList newScripts;
    QDirIterator pyIt(dirPath, QStringList() << QStringLiteral("*.py"), QDir::Files, QDirIterator::Subdirectories);
    while (pyIt.hasNext()) {
        newScripts.append(pyIt.next());
    }

    if (ProjectManager::instance().hasActiveProject()) {
        auto& proj = ProjectManager::instance().currentProject();
        for (const QString& s : newSources) {
            if (!proj.sourceFiles.contains(s)) proj.sourceFiles.append(s);
        }
        for (const QString& py : newScripts) {
            if (!proj.scriptFiles.contains(py)) proj.scriptFiles.append(py);
        }
        proj.saveToFile();
        updateProjectTree();
    }

    BackendService::instance().logMessage(LogLevel::Info, QStringLiteral("Project"),
        QStringLiteral("成功扫描目录：%1，检索到 %2 个源文件，%3 个脚本。")
            .arg(dirPath).arg(newSources.size()).arg(newScripts.size()));

    if (!firstCFile.isEmpty()) {
        loadFileIntoWorkspace(firstCFile);
    } else if (!newSources.isEmpty()) {
        loadFileIntoWorkspace(newSources.first());
    }
}

void MainWindow::onFileSwitcherActivated(int index) {
    if (index < 0 || index >= m_comboFileSwitcher->count()) return;
    QString filePath = m_comboFileSwitcher->itemData(index).toString();
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        loadFileIntoWorkspace(filePath);
    }
}

void MainWindow::loadFileIntoWorkspace(const QString& filePath) {
    BackendService::instance().loadSourceFile(filePath);
}

void MainWindow::onActionSaveFile() {
    auto* editor = currentActiveEditor();
    if (!editor) return;

    QString current = BackendService::instance().currentSourceFile();
    if (current.isEmpty()) {
        current = QFileDialog::getSaveFileName(this, trText(QStringLiteral("act.save")),
                                               QStringLiteral("source.c"), QStringLiteral("C Source Files (*.c *.h)"));
        if (current.isEmpty()) return;
    }

    QFile file(current);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << editor->toPlainText();
        file.close();
        BackendService::instance().setSourceContent(editor->toPlainText(), current);
        statusBar()->showMessage(QStringLiteral("已保存文件: ") + current, 3000);
    }
}

void MainWindow::onActionRunStaticAnalysis() {
    m_dockStaticExplorer->show();
    m_dockStaticExplorer->raise();
    if (m_dockRight && m_dockRight->isVisible()) {
        m_wasRightDockVisible = true;
        m_dockRight->hide();
    }

    auto* ed = currentActiveEditor();
    QString code = ed ? ed->toPlainText() : QString();
    BackendService::instance().setSourceContent(code, BackendService::instance().currentSourceFile());
    BackendService::instance().runStaticAnalysis();
}

void MainWindow::onActionInstrumentSource() {
    auto* ed = currentActiveEditor();
    QString code = ed ? ed->toPlainText() : QString();
    BackendService::instance().setSourceContent(code, BackendService::instance().currentSourceFile());
    BackendService::instance().runInstrumentation();
}

void MainWindow::onActionRunStimulus() {
    m_dockRight->show();
    m_dockRight->raise();
    m_stimulusView->clearLog();
    BackendService::instance().logMessage(LogLevel::Info, QStringLiteral("Stimulus"),
        QStringLiteral("触发激励工作台生成..."));
}

void MainWindow::onActionRunSimulation() {
    onActionCoverageAnalysis();
    BackendService::instance().runSimulationWithLatestStimulus();
}

void MainWindow::onActionCoverageAnalysis() {
    if (m_dockCoverageView) {
        m_dockCoverageView->show();
        m_dockCoverageView->raise();
    }
    auto* ed = currentActiveEditor();
    QString code = ed ? ed->toPlainText() : QString();
    QString curFile = BackendService::instance().currentSourceFile();
    m_coverageView->setCoverageReport(BackendService::instance().latestCoverageReport(),
                                      BackendService::instance().latestStaticReport().functions,
                                      code, curFile);
}

void MainWindow::onActionExportReport() {
    auto* ed = currentActiveEditor();
    QString code = ed ? ed->toPlainText() : QString();

    ReportPreviewDialog dlg(this);
    dlg.setReportData(BackendService::instance().latestStaticReport(),
                      BackendService::instance().latestCoverageReport(),
                      BackendService::instance().currentSourceFile(),
                      code,
                      BackendService::instance().latestStimulusPlan());
    dlg.exec();
}

void MainWindow::onActionViewLogFile() {
    QString logPath = Logger::instance().currentLogFilePath();
    if (QFile::exists(logPath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("暂无生成的日志文件。"));
    }
}

void MainWindow::onActionOpenLogDir() {
    QString logDir = Logger::instance().logDirectory();
    if (QDir(logDir).exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
    }
}

void MainWindow::onGraphNodeSelected(const QString& name, int startLine, int /* endLine */) {
    if (startLine > 0) {
        m_dockCodeEditor->show();
        m_dockCodeEditor->raise();
        if (m_wasRightDockVisible && m_dockRight && !m_dockRight->isVisible()) {
            m_dockRight->show();
        }
        if (auto* ed = currentActiveEditor()) {
            ed->gotoLine(startLine, 1);
        }
        statusBar()->showMessage(QStringLiteral("已通过图元交互定位至函数/代码块: ") + name +
                                 QString(" (第 %1 行)").arg(startLine), 4000);
        Logger::instance().log(LogLevel::Info, QStringLiteral("Navigation"),
            QString("图元点击跳转至节点 %1 (行 %2)").arg(name).arg(startLine));
    }
}

void MainWindow::onActionUserManual() {
    UserManualDialog dlg(this);
    dlg.exec();
}

void MainWindow::onActionAbout() {
    QMessageBox::about(this, trText(QStringLiteral("act.about")),
        QStringLiteral("<h3>嵌入式 C 覆盖分析与静态分析套件 (Embedded C Coverage & Static Studio)</h3>"
                       "<p>版本: <b>v1.5.0 专业工程版</b></p>"
                       "<p>专为高安全等级嵌入式控制系统打造的现代代码分析、拓扑可视化与覆盖率仿真度量平台。</p>"
                       "<b>核心架构亮点：</b>"
                       "<ul>"
                       "<li><b>以工程 (.covproj) 为核心：</b> 全生命周期管理目标架构、源码清单与激励测试方案。</li>"
                       "<li><b>多文件选项卡代码编辑器：</b> 自由打开切换源文件，支持右键关闭与全关闭。</li>"
                       "<li><b>免配置独立 Graphviz：</b> 优先绑定本地 Graphviz 二进制环境，图元大小完全自适应，无失真。</li>"
                       "<li><b>统一停靠底板：</b> 静态缺陷列表与实时运行控制台日志紧密集成。</li>"
                       "<li><b>多主题与多语言即时响应：</b> 默认为简体中文，内置 4 种现代科技主题。</li>"
                       "</ul>"));
}

void MainWindow::onSourceFileLoaded(const QString& filePath, const QString& content) {
    auto* editor = getOrCreateEditorForFile(filePath, content);
    QFileInfo fi(filePath);
    if (m_dockCodeEditor) {
        m_dockCodeEditor->setWindowTitle(trText(QStringLiteral("dock.code_editor")) + QStringLiteral(" - ") + fi.fileName());
    }
    m_lblFileStatus->setText(QStringLiteral("文件: ") + fi.fileName());

    // 同步更新覆盖分析视窗的源码
    if (m_coverageView) {
        m_coverageView->setSourceFile(filePath, content);
    }

    // 同步更新静态分析探索视窗
    if (m_staticExplorer) {
        m_staticExplorer->setSourceFile(filePath, content);
    }

    // 同步更新切换下拉框选中的文件
    int idx = m_comboFileSwitcher->findData(filePath);
    if (idx >= 0 && m_comboFileSwitcher->currentIndex() != idx) {
        m_comboFileSwitcher->setCurrentIndex(idx);
    }

    // 载入源文件后自动触发静态分析与图表生成
    BackendService::instance().runStaticAnalysis();
}

void MainWindow::onStaticAnalysisFinished(const StaticAnalysisReport& report) {
    m_staticView->setReport(report);

    auto* ed = currentActiveEditor();
    if (ed) {
        ed->setStaticIssues(report.issues);
    }
    
    QString code = ed ? ed->toPlainText() : QString();
    m_coverageView->setCoverageReport(BackendService::instance().latestCoverageReport(),
                                      report.functions,
                                      code,
                                      BackendService::instance().currentSourceFile());

    // 联动刷新静态分析与执行路径探索工作台
    if (m_staticExplorer) {
        m_staticExplorer->setAnalysisReport(report, code);
    }

    // 联动刷新 Graphviz 代码拓扑调用图与函数内部控制流图
    m_graphView->updateData(report, code);

    m_lblAnalysisStatus->setText(QString("问题: %1 (错误: %2, 警告: %3)")
                                     .arg(report.issues.size())
                                     .arg(report.errorCount())
                                     .arg(report.warningCount()));
}

void MainWindow::onCoverageReportUpdated(const CoverageReport& report) {
    auto* ed = currentActiveEditor();
    QString code = ed ? ed->toPlainText() : QString();
    QString curFile = BackendService::instance().currentSourceFile();

    m_coverageView->setCoverageReport(report,
                                      BackendService::instance().latestStaticReport().functions,
                                      code, curFile);
    if (ed) {
        ed->setCoverageReport(report);
    }

    m_lblCoverageStatus->setText(QString("覆盖率: %1% (语句) | %2% (分支)")
                                     .arg(QString::number(report.statementCoveragePercent(), 'f', 1))
                                     .arg(QString::number(report.branchCoveragePercent(), 'f', 1)));

    // 运行脚本注入激励/仿真后，自动切出测试覆盖分析视图 (Requirement 3)
    if (m_dockCoverageView) {
        m_dockCoverageView->show();
        m_dockCoverageView->raise();
    }
}

void MainWindow::onStimulusPlanReady(const StimulusPlan& plan) {
    m_stimulusView->setStimulusPlan(plan);
}

void MainWindow::onInstrumentationFinished(bool success, const InstrumentationMetadata& meta, const QString& outputPath) {
    if (success) {
        QMessageBox::information(this, QStringLiteral("源码插桩完成"),
            QString("嵌入式源文件插桩探针注入成功！\n\n"
                    "目标路径：%1\n"
                    "已注入探针统计：\n"
                    "• 语句覆盖率探针: %2 个\n"
                    "• 分支判定探针:   %3 个\n"
                    "• 函数入口探针:   %4 个\n\n"
                    "已在输出路径生成嵌入式轻量级运行时头文件 'coverage_runtime.h'。")
                .arg(outputPath)
                .arg(meta.totalProbeLines)
                .arg(meta.totalProbeBranches)
                .arg(meta.totalProbeFunctions));
    }
}

void MainWindow::onBackendLog(LogLevel level, const QString& module, const QString& message) {
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    QString levelStr = (level == LogLevel::Error) ? (isLight ? QStringLiteral("<font color='#dc2626'>[错误]</font>") : QStringLiteral("<font color='#f38ba8'>[错误]</font>")) :
                       (level == LogLevel::Warning) ? (isLight ? QStringLiteral("<font color='#d97706'>[警告]</font>") : QStringLiteral("<font color='#f9e2af'>[警告]</font>")) :
                       (level == LogLevel::Debug) ? (isLight ? QStringLiteral("<font color='#1a73e8'>[调试]</font>") : QStringLiteral("<font color='#89b4fa'>[调试]</font>")) :
                       (isLight ? QStringLiteral("<font color='#16a34a'>[信息]</font>") : QStringLiteral("<font color='#a6e3a1'>[信息]</font>"));

    QString timeColor = isLight ? QStringLiteral("#5f6368") : QStringLiteral("#6c7086");
    QString html = QString("<font color='%1'>[%2]</font> %3 <b>[%4]</b> %5")
                       .arg(timeColor, timeStr, levelStr, module, message.toHtmlEscaped());
    m_systemLog->append(html);
}

void MainWindow::onIssueSelectedInEditor(int line, int col) {
    if (auto* ed = currentActiveEditor()) {
        ed->gotoLine(line, col);
    }
}

void MainWindow::onCoverageLineSelectedInEditor(int line) {
    if (auto* ed = currentActiveEditor()) {
        ed->gotoLine(line, 1);
    }
}

void MainWindow::onFileTreeItemDoubleClicked(QTreeWidgetItem* item, int /* col */) {
    if (!item) return;
    QString filePath = item->data(0, Qt::UserRole).toString();
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        if (filePath.endsWith(QStringLiteral(".c")) || filePath.endsWith(QStringLiteral(".h")) || filePath.endsWith(QStringLiteral(".cpp"))) {
            loadFileIntoWorkspace(filePath);
            m_dockCodeEditor->show();
            m_dockCodeEditor->raise();
            if (m_wasRightDockVisible && m_dockRight && !m_dockRight->isVisible()) {
                m_dockRight->show();
            }
        } else if (filePath.endsWith(QStringLiteral(".py"))) {
            m_dockRight->show();
            m_dockRight->raise();
            m_stimulusView->setPythonExecutable(StimulusEngine::autoDetectPython());
            m_stimulusView->loadScriptFile(filePath);
        } else if (filePath.endsWith(QStringLiteral(".covproj"))) {
            openProjectFile(filePath);
        } else if (filePath.endsWith(QStringLiteral(".html"), Qt::CaseInsensitive) ||
                   filePath.endsWith(QStringLiteral(".htm"), Qt::CaseInsensitive) ||
                   filePath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive) ||
                   filePath.endsWith(QStringLiteral(".docx"), Qt::CaseInsensitive) ||
                   filePath.endsWith(QStringLiteral(".doc"), Qt::CaseInsensitive) ||
                   filePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive) ||
                   filePath.endsWith(QStringLiteral(".txt"), Qt::CaseInsensitive)) {
            // Requirement 11: 默认内置打开 HTML/PDF/DOCX/JSON 报告，纯只读预览，严禁唤起外部三方软件
            auto* dlg = new ReportPreviewDialog(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->loadExistingReportFile(filePath);
            dlg->show();
        }
    }
}

void MainWindow::onFileTreeContextMenu(const QPoint& pos) {
    auto* item = m_fileTree->itemAt(pos);

    // 空白处右键
    if (!item) {
        QMenu menu(this);
        auto* actAddFile = menu.addAction(QIcon(QStringLiteral(":/icons/add_file.svg")), QStringLiteral("➕ 添加文件到工程..."));
        connect(actAddFile, &QAction::triggered, this, &MainWindow::onBtnAddFileClicked);
        auto* actAddFolder = menu.addAction(QIcon(QStringLiteral(":/icons/add_folder.svg")), QStringLiteral("📁 导入源码目录到工程..."));
        connect(actAddFolder, &QAction::triggered, this, &MainWindow::onBtnAddFolderClicked);
        menu.exec(m_fileTree->viewport()->mapToGlobal(pos));
        return;
    }

    QString role = item->data(1, Qt::DisplayRole).toString();
    QString filePath = item->data(0, Qt::UserRole).toString();

    // Requirement 8: 屏蔽分类节点 (源文件、头文件、脚本文件、报告文件) 的右键事件
    if (role == QStringLiteral("Category") || role == QStringLiteral("Folder") || filePath.isEmpty()) {
        return;
    }

    QMenu menu(this);

    // 1. 工程根节点
    if (role == QStringLiteral("Project") || filePath.endsWith(QStringLiteral(".covproj"), Qt::CaseInsensitive)) {
        auto* actAddFile = menu.addAction(QIcon(QStringLiteral(":/icons/add_file.svg")), QStringLiteral("➕ 添加源文件/脚本到工程..."));
        connect(actAddFile, &QAction::triggered, this, &MainWindow::onBtnAddFileClicked);

        auto* actAddFolder = menu.addAction(QIcon(QStringLiteral(":/icons/add_folder.svg")), QStringLiteral("📁 导入源码目录到工程..."));
        connect(actAddFolder, &QAction::triggered, this, &MainWindow::onBtnAddFolderClicked);

        menu.addSeparator();
        auto* actProps = menu.addAction(QIcon(QStringLiteral(":/icons/app_icon.svg")), QStringLiteral("⚙️ 工程属性与配置 (Hub)..."));
        connect(actProps, &QAction::triggered, this, &MainWindow::onActionProjectHub);

        auto* actSave = menu.addAction(QIcon(QStringLiteral(":/icons/save_file.svg")), QStringLiteral("💾 保存当前工程"));
        connect(actSave, &QAction::triggered, this, &MainWindow::onActionSaveProject);

        menu.addSeparator();
        auto* actReveal = menu.addAction(QIcon(QStringLiteral(":/icons/folder.svg")), QStringLiteral("📂 打开文件所在路径"));
        connect(actReveal, &QAction::triggered, this, [this, filePath]() {
            revealInExplorer(filePath);
        });

        menu.exec(m_fileTree->viewport()->mapToGlobal(pos));
        return;
    }

    // 2. Python 激励脚本
    if (filePath.endsWith(QStringLiteral(".py"), Qt::CaseInsensitive)) {
        auto* actRunPy = menu.addAction(QIcon(QStringLiteral(":/icons/run_stimulus.svg")),
                                        trText(QStringLiteral("act.run_py_script")));
        connect(actRunPy, &QAction::triggered, this, [this, filePath]() {
            m_dockRight->show();
            m_dockRight->raise();
            m_stimulusView->loadScriptFile(filePath);
            if (m_rightTabs) m_rightTabs->setCurrentWidget(m_stimulusView);
            BackendService::instance().runStimulusScript(filePath);
        });

        auto* actEditPy = menu.addAction(QIcon(QStringLiteral(":/icons/open_file.svg")),
                                         QStringLiteral("在 Python 激励工作台中打开编辑"));
        connect(actEditPy, &QAction::triggered, this, [this, filePath]() {
            m_dockRight->show();
            m_dockRight->raise();
            m_stimulusView->loadScriptFile(filePath);
            if (m_rightTabs) m_rightTabs->setCurrentWidget(m_stimulusView);
        });

        menu.addSeparator();
        auto* actReveal = menu.addAction(QIcon(QStringLiteral(":/icons/folder.svg")), QStringLiteral("📂 打开文件所在路径"));
        connect(actReveal, &QAction::triggered, this, [this, filePath]() {
            revealInExplorer(filePath);
        });

        menu.addSeparator();
        auto* actAddFile = menu.addAction(QIcon(QStringLiteral(":/icons/add_file.svg")), QStringLiteral("➕ 添加文件到工程..."));
        connect(actAddFile, &QAction::triggered, this, &MainWindow::onBtnAddFileClicked);
        auto* actAddFolder = menu.addAction(QIcon(QStringLiteral(":/icons/add_folder.svg")), QStringLiteral("📁 导入源码目录到工程..."));
        connect(actAddFolder, &QAction::triggered, this, &MainWindow::onBtnAddFolderClicked);

        menu.addSeparator();
        auto* actRemove = menu.addAction(QIcon(QStringLiteral(":/icons/delete.svg")),
                                         QStringLiteral("🗑️ 从当前工程移除选中文件"));
        connect(actRemove, &QAction::triggered, this, &MainWindow::onBtnRemoveFileClicked);

        menu.exec(m_fileTree->viewport()->mapToGlobal(pos));
        return;
    }

    // 3. C/C++ 源文件与头文件
    if (filePath.endsWith(QStringLiteral(".c"), Qt::CaseInsensitive) ||
        filePath.endsWith(QStringLiteral(".cpp"), Qt::CaseInsensitive) ||
        filePath.endsWith(QStringLiteral(".h"), Qt::CaseInsensitive)) {

        auto* actOpen = menu.addAction(QIcon(QStringLiteral(":/icons/open_file.svg")), QStringLiteral("在多标签编辑器中打开"));
        connect(actOpen, &QAction::triggered, this, [this, filePath]() {
            loadFileIntoWorkspace(filePath);
            m_dockCodeEditor->show();
            m_dockCodeEditor->raise();
            if (m_wasRightDockVisible && m_dockRight && !m_dockRight->isVisible()) {
                m_dockRight->show();
            }
        });

        auto* actStatic = menu.addAction(QIcon(QStringLiteral(":/icons/static_analysis.svg")), QStringLiteral("⚡ 执行静态分析与路径探索"));
        connect(actStatic, &QAction::triggered, this, [this, filePath]() {
            loadFileIntoWorkspace(filePath);
            m_dockStaticExplorer->show();
            m_dockStaticExplorer->raise();
            if (m_dockRight && m_dockRight->isVisible()) {
                m_wasRightDockVisible = true;
                m_dockRight->hide();
            }
            BackendService::instance().runStaticAnalysis();
        });

        auto* actInst = menu.addAction(QIcon(QStringLiteral(":/icons/instrument.svg")), QStringLiteral("执行源码探针插桩 (F6)"));
        connect(actInst, &QAction::triggered, this, [this, filePath]() {
            loadFileIntoWorkspace(filePath);
            onActionInstrumentSource();
        });

        menu.addSeparator();
        auto* actReveal = menu.addAction(QIcon(QStringLiteral(":/icons/folder.svg")), QStringLiteral("📂 打开文件所在路径"));
        connect(actReveal, &QAction::triggered, this, [this, filePath]() {
            revealInExplorer(filePath);
        });

        menu.addSeparator();
        auto* actAddFile = menu.addAction(QIcon(QStringLiteral(":/icons/add_file.svg")), QStringLiteral("➕ 添加文件到工程..."));
        connect(actAddFile, &QAction::triggered, this, &MainWindow::onBtnAddFileClicked);
        auto* actAddFolder = menu.addAction(QIcon(QStringLiteral(":/icons/add_folder.svg")), QStringLiteral("📁 导入源码目录到工程..."));
        connect(actAddFolder, &QAction::triggered, this, &MainWindow::onBtnAddFolderClicked);

        menu.addSeparator();
        auto* actRemove = menu.addAction(QIcon(QStringLiteral(":/icons/delete.svg")),
                                         QStringLiteral("🗑️ 从当前工程移除选中文件"));
        connect(actRemove, &QAction::triggered, this, &MainWindow::onBtnRemoveFileClicked);

        menu.exec(m_fileTree->viewport()->mapToGlobal(pos));
        return;
    }

    // 4. 报告文件
    if (filePath.endsWith(QStringLiteral(".html"), Qt::CaseInsensitive) ||
        filePath.endsWith(QStringLiteral(".htm"), Qt::CaseInsensitive) ||
        filePath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive) ||
        filePath.endsWith(QStringLiteral(".docx"), Qt::CaseInsensitive) ||
        filePath.endsWith(QStringLiteral(".doc"), Qt::CaseInsensitive) ||
        filePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive) ||
        filePath.endsWith(QStringLiteral(".txt"), Qt::CaseInsensitive)) {

        auto* actViewRep = menu.addAction(QIcon(QStringLiteral(":/icons/html_file.svg")), QStringLiteral("在内置全貌预览框中查看"));
        connect(actViewRep, &QAction::triggered, this, [this, filePath]() {
            auto* dlg = new ReportPreviewDialog(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->loadExistingReportFile(filePath);
            dlg->show();
        });

        menu.addSeparator();
        auto* actReveal = menu.addAction(QIcon(QStringLiteral(":/icons/folder.svg")), QStringLiteral("📂 打开文件所在路径"));
        connect(actReveal, &QAction::triggered, this, [this, filePath]() {
            revealInExplorer(filePath);
        });

        menu.addSeparator();
        auto* actRemove = menu.addAction(QIcon(QStringLiteral(":/icons/delete.svg")),
                                         QStringLiteral("🗑️ 从列表中移除"));
        connect(actRemove, &QAction::triggered, this, &MainWindow::onBtnRemoveFileClicked);

        menu.exec(m_fileTree->viewport()->mapToGlobal(pos));
        return;
    }
}

void MainWindow::revealInExplorer(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    if (!fi.exists()) return;

#if defined(Q_OS_WIN)
    QString winPath = QDir::toNativeSeparators(fi.absoluteFilePath());
    QProcess::startDetached(QStringLiteral("explorer.exe"), {QStringLiteral("/select,"), winPath});
#elif defined(Q_OS_MAC)
    QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-R"), fi.absoluteFilePath()});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
#endif
}

void MainWindow::onBtnAddFileClicked() {
    QStringList paths = QFileDialog::getOpenFileNames(this, QStringLiteral("添加源文件到工程"),
                                                      QString(),
                                                      QStringLiteral("Source / Script Files (*.c *.h *.cpp *.py);;All Files (*.*)"));
    if (paths.isEmpty()) return;

    if (ProjectManager::instance().hasActiveProject()) {
        auto& proj = ProjectManager::instance().currentProject();
        for (const QString& path : paths) {
            if (path.endsWith(QStringLiteral(".py"))) {
                if (!proj.scriptFiles.contains(path)) proj.scriptFiles.append(path);
            } else {
                if (!proj.sourceFiles.contains(path)) proj.sourceFiles.append(path);
            }
        }
        proj.saveToFile();
        updateProjectTree();
    }

    loadFileIntoWorkspace(paths.first());
}

void MainWindow::onBtnAddFolderClicked() {
    onActionImportDirectory();
}

void MainWindow::onBtnRemoveFileClicked() {
    auto* item = m_fileTree->currentItem();
    if (!item) return;

    QString path = item->data(0, Qt::UserRole).toString();
    if (!path.isEmpty()) {
        m_importedFiles.removeAll(path);
        int idx = m_comboFileSwitcher->findData(path);
        if (idx >= 0) {
            m_comboFileSwitcher->removeItem(idx);
        }

        if (ProjectManager::instance().hasActiveProject()) {
            auto& proj = ProjectManager::instance().currentProject();
            proj.sourceFiles.removeAll(path);
            proj.scriptFiles.removeAll(path);
            proj.saveToFile();
        }

        // 如果在多文件编辑器中打开了，也关闭它
        if (m_openEditors.contains(path)) {
            int tabIdx = m_editorTabs->indexOf(m_openEditors[path]);
            if (tabIdx >= 0) {
                closeEditorTab(tabIdx);
            }
        }
    }

    delete item;
}

void MainWindow::onActionStackAnalysis() {
    if (m_dockStackAnalysis) {
        m_dockStackAnalysis->show();
        m_dockStackAnalysis->raise();
    }

    if (m_stackAnalysisView) {
        if (ProjectManager::instance().hasActiveProject()) {
            const auto& proj = ProjectManager::instance().currentProject();
            QFileInfo projFi(proj.filePath);
            QString projDir = projFi.absolutePath();
            QString base = projFi.baseName();
            QString candBin = projDir + QStringLiteral("/") + base + QStringLiteral(".exe");
            if (!QFile::exists(candBin)) candBin = projDir + QStringLiteral("/bin/") + base + QStringLiteral(".exe");
            if (!QFile::exists(candBin)) candBin = projDir + QStringLiteral("/build/") + base + QStringLiteral(".exe");
            if (!QFile::exists(candBin)) candBin = projDir + QStringLiteral("/") + base + QStringLiteral(".elf");
            if (!QFile::exists(candBin)) candBin = projDir + QStringLiteral("/Source/") + base + QStringLiteral(".o");
            if (QFile::exists(candBin)) {
                m_stackAnalysisView->setBinaryPath(candBin);
            }
        }
    }
}

void MainWindow::updateProjectDependentUiState(bool hasProject) {
    if (!hasProject) {
        // 1. 关闭/隐藏项目工作区停靠窗口 (Requirement 1)
        if (m_dockCodeEditor) m_dockCodeEditor->hide();
        if (m_dockStaticExplorer) m_dockStaticExplorer->hide();
        if (m_dockGraphView) m_dockGraphView->hide();
        if (m_dockStackAnalysis) m_dockStackAnalysis->hide();
        if (m_dockCoverageView) m_dockCoverageView->hide();
        if (m_dockRight) m_dockRight->hide();

        // 2. 仅展示欢迎向导、空项目树与底部运行实时日志 (Requirement 1)
        if (m_dockWelcome) {
            m_dockWelcome->show();
            m_dockWelcome->raise();
        }
        if (m_dockLeft) {
            m_dockLeft->show();
        }
        if (m_dockBottom) {
            m_dockBottom->show();
            if (m_bottomTabs && m_systemLog) {
                m_bottomTabs->setCurrentWidget(m_systemLog);
            }
        }

        // 3. 禁用菜单中所有业务功能按钮，实现互斥 (Requirement 13)
        for (auto* act : m_projectDependentActions) {
            if (act) act->setEnabled(false);
        }

        // 4. 工具栏仅显示工程管理中心和打开工程，其余功能按钮和切换器全部隐藏 (Requirement 14)
        for (auto* act : m_toolbarProjectDependentActions) {
            if (act) act->setVisible(false);
        }
        if (m_lblFileSwitcher) m_lblFileSwitcher->setVisible(false);
        if (m_comboFileSwitcher) m_comboFileSwitcher->setVisible(false);

    } else {
        // 1. 关闭欢迎向导页 (Requirement 11)
        if (m_dockWelcome) {
            m_dockWelcome->hide();
        }

        // 2. 显示并激活主要工作视窗
        if (m_dockLeft) m_dockLeft->show();
        if (m_dockCodeEditor) {
            m_dockCodeEditor->show();
            m_dockCodeEditor->raise();
        }
        if (m_dockRight) m_dockRight->show();
        if (m_dockBottom) m_dockBottom->show();

        // 3. 恢复启用菜单栏中业务功能按钮 (Requirement 13)
        for (auto* act : m_projectDependentActions) {
            if (act) act->setEnabled(true);
        }

        // 4. 恢复工具栏所有功能按钮与切换器 (Requirement 14)
        for (auto* act : m_toolbarProjectDependentActions) {
            if (act) act->setVisible(true);
        }
        if (m_lblFileSwitcher) m_lblFileSwitcher->setVisible(true);
        if (m_comboFileSwitcher) m_comboFileSwitcher->setVisible(true);
    }
}

} // namespace Coverage
