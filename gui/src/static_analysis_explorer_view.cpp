#include "static_analysis_explorer_view.h"
#include "localization_manager.h"
#include "theme_manager.h"
#include "coverage/graph_generator.h"
#include "coverage/graph_layout.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QMessageBox>

namespace Coverage {

StaticAnalysisExplorerView::StaticAnalysisExplorerView(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    retranslateUi();
}

void StaticAnalysisExplorerView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // 采用顶级双 Tab 架构
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setDocumentMode(true);
    mainLayout->addWidget(m_tabWidget);

    // =========================================================================
    // Tab 1: 函数度量总览 (Function Metrics Overview)
    // =========================================================================
    m_tabOverview = new QWidget(m_tabWidget);
    auto* overviewLayout = new QVBoxLayout(m_tabOverview);
    overviewLayout->setContentsMargins(6, 6, 6, 6);
    overviewLayout->setSpacing(8);

    // 1.1 顶部 KPI 控制栏与质量度量看板 (双行自适应布局，防止窄屏下水平挤压溢出)
    m_topCard = new QWidget(m_tabOverview);
    auto* topCardLayout = new QVBoxLayout(m_topCard);
    topCardLayout->setContentsMargins(8, 6, 8, 6);
    topCardLayout->setSpacing(6);

    auto* row1Layout = new QHBoxLayout();
    row1Layout->setSpacing(10);
    m_btnAnalyze = new QPushButton(QIcon(QStringLiteral(":/icons/static_analysis.svg")),
                                   QStringLiteral("⚡ 执行选定代码静态分析"), this);
    m_btnAnalyze->setCursor(Qt::PointingHandCursor);
    connect(m_btnAnalyze, &QPushButton::clicked, this, &StaticAnalysisExplorerView::onAnalyzeButtonClicked);
    row1Layout->addWidget(m_btnAnalyze);

    m_lblFile = new QLabel(QStringLiteral("目标文件: (未加载)"), this);
    row1Layout->addWidget(m_lblFile);
    row1Layout->addStretch();
    topCardLayout->addLayout(row1Layout);

    auto* row2Layout = new QHBoxLayout();
    row2Layout->setSpacing(8);
    m_kpiStatements = new QLabel(QStringLiteral("<b>语句数</b>: 0"), this);
    m_kpiBranches = new QLabel(QStringLiteral("<b>分支数</b>: 0"), this);
    m_kpiCommentRatio = new QLabel(QStringLiteral("<b>注释率</b>: 0.0%"), this);
    m_kpiMcdc = new QLabel(QStringLiteral("<b>MC/DC 判定</b>: 0/0"), this);
    m_kpiComplexity = new QLabel(QStringLiteral("<b>圈复杂度</b>: 0"), this);
    m_kpiIssues = new QLabel(QStringLiteral("<b>缺陷违规</b>: 0"), this);

    row2Layout->addWidget(m_kpiStatements);
    row2Layout->addWidget(m_kpiBranches);
    row2Layout->addWidget(m_kpiCommentRatio);
    row2Layout->addWidget(m_kpiMcdc);
    row2Layout->addWidget(m_kpiComplexity);
    row2Layout->addWidget(m_kpiIssues);
    row2Layout->addStretch();
    topCardLayout->addLayout(row2Layout);

    overviewLayout->addWidget(m_topCard);

    // 1.2 当前源文件所有函数度量统计大表
    m_lblTableHint = new QLabel(QStringLiteral("📊 当前源文件所有函数结构度量一览 (双击任意函数行可直接切换至路径探索界面)"), m_tabOverview);
    overviewLayout->addWidget(m_lblTableHint);

    m_overviewTable = new QTableWidget(m_tabOverview);
    m_overviewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_overviewTable->setColumnCount(11);
    m_overviewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_overviewTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_overviewTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_overviewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_overviewTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_overviewTable->setAlternatingRowColors(true);
    m_overviewTable->verticalHeader()->setVisible(false);
    connect(m_overviewTable, &QTableWidget::cellDoubleClicked, this, &StaticAnalysisExplorerView::onOverviewTableDoubleClicked);
    overviewLayout->addWidget(m_overviewTable, 1);

    m_tabWidget->addTab(m_tabOverview, QIcon(QStringLiteral(":/icons/static_analysis.svg")), QStringLiteral("📋 函数度量总览"));

    // =========================================================================
    // Tab 2: 控制流与路径探索 (Control Flow & Path Explorer)
    // =========================================================================
    m_tabPathExplorer = new QWidget(m_tabWidget);
    auto* pathExpLayout = new QVBoxLayout(m_tabPathExplorer);
    pathExpLayout->setContentsMargins(4, 4, 4, 4);
    pathExpLayout->setSpacing(4);

    m_mainSplitter = new QSplitter(Qt::Horizontal, m_tabPathExplorer);

    // ------------------------------------------
    // 2.1 左栏: 函数列表 (上方) + 独立执行路径 (下方)
    // ------------------------------------------
    auto* leftWidget = new QWidget(m_mainSplitter);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto* leftSplitter = new QSplitter(Qt::Vertical, leftWidget);

    // 函数选择列表
    auto* funcBox = new QWidget(leftSplitter);
    auto* funcLayout = new QVBoxLayout(funcBox);
    funcLayout->setContentsMargins(0, 0, 0, 0);
    funcLayout->setSpacing(2);
    m_lblFuncTitle = new QLabel(QStringLiteral("📁 函数列表 (选定以提取独立路径)"), funcBox);
    funcLayout->addWidget(m_lblFuncTitle);

    m_funcTable = new QTableWidget(funcBox);
    m_funcTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_funcTable->setColumnCount(3);
    m_funcTable->setHorizontalHeaderLabels({
        QStringLiteral("函数名称"),
        QStringLiteral("圈复杂度"),
        QStringLiteral("行数")
    });
    m_funcTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_funcTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_funcTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_funcTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_funcTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_funcTable->verticalHeader()->setVisible(false);
    connect(m_funcTable, &QTableWidget::itemSelectionChanged, this, &StaticAnalysisExplorerView::onFunctionTableSelectionChanged);
    funcLayout->addWidget(m_funcTable);
    leftSplitter->addWidget(funcBox);

    // 独立执行路径列表
    auto* pathBox = new QWidget(leftSplitter);
    auto* pathLayout = new QVBoxLayout(pathBox);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->setSpacing(2);
    m_lblPathTitle = new QLabel(QStringLiteral("⚡ 独立执行路径 (Basis Paths)"), pathBox);
    pathLayout->addWidget(m_lblPathTitle);

    m_pathTable = new QTableWidget(pathBox);
    m_pathTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pathTable->setColumnCount(3);
    m_pathTable->setHorizontalHeaderLabels({
        QStringLiteral("序号"),
        QStringLiteral("覆盖代码行"),
        QStringLiteral("判定分支条件")
    });
    m_pathTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pathTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pathTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_pathTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pathTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pathTable->verticalHeader()->setVisible(false);
    connect(m_pathTable, &QTableWidget::itemSelectionChanged, this, &StaticAnalysisExplorerView::onPathTableSelectionChanged);
    pathLayout->addWidget(m_pathTable);
    leftSplitter->addWidget(pathBox);

    leftSplitter->setStretchFactor(0, 4);
    leftSplitter->setStretchFactor(1, 6);
    leftLayout->addWidget(leftSplitter);

    m_mainSplitter->addWidget(leftWidget);

    // ------------------------------------------
    // 2.2 中栏: Graphviz 原生图元路径高亮画布
    // ------------------------------------------
    auto* centerWidget = new QWidget(m_mainSplitter);
    auto* centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(4);

    auto* canvasToolBar = new QHBoxLayout();
    m_lblPathStatus = new QLabel(QStringLiteral("请在左侧选择函数和路径进行 Graphviz 渲染"), centerWidget);
    m_lblPathStatus->setStyleSheet(QStringLiteral("color: #a6adc8; font-weight: bold; font-size: 12px;"));
    canvasToolBar->addWidget(m_lblPathStatus);
    canvasToolBar->addStretch();

    m_btnZoomIn = new QPushButton(QIcon(QStringLiteral(":/icons/zoom_in.svg")), QStringLiteral("放大"), centerWidget);
    m_btnZoomOut = new QPushButton(QIcon(QStringLiteral(":/icons/zoom_out.svg")), QStringLiteral("缩小"), centerWidget);
    m_btnFit = new QPushButton(QIcon(QStringLiteral(":/icons/fit_window.svg")), QStringLiteral("适应"), centerWidget);
    m_btnReset = new QPushButton(QIcon(QStringLiteral(":/icons/reset_zoom.svg")), QStringLiteral("重置"), centerWidget);

    m_btnZoomIn->setIconSize(QSize(14, 14));
    m_btnZoomOut->setIconSize(QSize(14, 14));
    m_btnFit->setIconSize(QSize(14, 14));
    m_btnReset->setIconSize(QSize(14, 14));

    canvasToolBar->addWidget(m_btnZoomIn);
    canvasToolBar->addWidget(m_btnZoomOut);
    canvasToolBar->addWidget(m_btnFit);
    canvasToolBar->addWidget(m_btnReset);
    centerLayout->addLayout(canvasToolBar);

    m_canvasView = new GraphCanvasView(centerWidget);
    connect(m_canvasView, &GraphCanvasView::nodeSelected, this, &StaticAnalysisExplorerView::onGraphNodeClicked);
    connect(m_btnZoomIn, &QPushButton::clicked, m_canvasView, &GraphCanvasView::zoomIn);
    connect(m_btnZoomOut, &QPushButton::clicked, m_canvasView, &GraphCanvasView::zoomOut);
    connect(m_btnFit, &QPushButton::clicked, m_canvasView, &GraphCanvasView::fitToWindow);
    connect(m_btnReset, &QPushButton::clicked, m_canvasView, &GraphCanvasView::resetZoom);
    centerLayout->addWidget(m_canvasView, 1);

    m_mainSplitter->addWidget(centerWidget);

    // ------------------------------------------
    // 2.3 右栏: 源码执行流动态着色视窗
    // ------------------------------------------
    auto* rightWidget = new QWidget(m_mainSplitter);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    m_lblRightHeader = new QLabel(QStringLiteral("📄 源码执行流动态着色 (选定路径覆盖代码行将以亮绿底色高亮)"), rightWidget);
    m_lblRightHeader->setStyleSheet(QStringLiteral("font-weight: bold; color: #89b4fa; padding: 2px;"));
    rightLayout->addWidget(m_lblRightHeader);

    m_rightCodeEditor = new CodeEditor(rightWidget);
    m_rightCodeEditor->setReadOnly(true);
    rightLayout->addWidget(m_rightCodeEditor, 1);

    m_mainSplitter->addWidget(rightWidget);

    // 设置初始分割比例: 左 28%, 中 42%, 右 30%
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 4);
    m_mainSplitter->setStretchFactor(2, 3);

    pathExpLayout->addWidget(m_mainSplitter);
    m_tabWidget->addTab(m_tabPathExplorer, QIcon(QStringLiteral(":/icons/path_explorer.svg")), QStringLiteral("🧭 控制流与路径探索"));

    applyTheme(ThemeManager::currentTheme());
}

void StaticAnalysisExplorerView::retranslateUi() {
    bool isZh = (LocalizationManager::instance().currentLanguage() == Language::Chinese);

    m_tabWidget->setTabText(0, isZh ? QStringLiteral("📋 函数度量总览") : QStringLiteral("📋 Function Metrics Overview"));
    m_tabWidget->setTabText(1, isZh ? QStringLiteral("🧭 控制流与路径探索") : QStringLiteral("🧭 Control Flow & Path Explorer"));

    m_btnAnalyze->setText(isZh ? QStringLiteral("⚡ 执行选定代码静态分析") : QStringLiteral("⚡ Run Static Analysis"));

    m_overviewTable->setHorizontalHeaderLabels({
        isZh ? QStringLiteral("序号") : QStringLiteral("#"),
        isZh ? QStringLiteral("函数名称") : QStringLiteral("Function Name"),
        isZh ? QStringLiteral("函数位置") : QStringLiteral("Location"),
        isZh ? QStringLiteral("语句数") : QStringLiteral("Statements"),
        isZh ? QStringLiteral("分支数") : QStringLiteral("Branches"),
        isZh ? QStringLiteral("圈复杂度") : QStringLiteral("CC"),
        isZh ? QStringLiteral("扇入数") : QStringLiteral("Fan-In"),
        isZh ? QStringLiteral("扇出数") : QStringLiteral("Fan-Out"),
        isZh ? QStringLiteral("函数深度") : QStringLiteral("Func Depth"),
        isZh ? QStringLiteral("调用深度") : QStringLiteral("Call Depth"),
        isZh ? QStringLiteral("MCDC数") : QStringLiteral("MC/DC Count")
    });

    m_funcTable->setHorizontalHeaderLabels({
        isZh ? QStringLiteral("函数名称") : QStringLiteral("Function"),
        isZh ? QStringLiteral("圈复杂度") : QStringLiteral("CC"),
        isZh ? QStringLiteral("行数") : QStringLiteral("LOC")
    });

    m_pathTable->setHorizontalHeaderLabels({
        isZh ? QStringLiteral("序号") : QStringLiteral("#"),
        isZh ? QStringLiteral("覆盖代码行") : QStringLiteral("Lines"),
        isZh ? QStringLiteral("判定分支条件") : QStringLiteral("Decision Conditions")
    });

    m_btnZoomIn->setText(isZh ? QStringLiteral("放大") : QStringLiteral("Zoom In"));
    m_btnZoomOut->setText(isZh ? QStringLiteral("缩小") : QStringLiteral("Zoom Out"));
    m_btnFit->setText(isZh ? QStringLiteral("适应") : QStringLiteral("Fit"));
    m_btnReset->setText(isZh ? QStringLiteral("重置") : QStringLiteral("Reset"));

    m_lblRightHeader->setText(isZh ?
        QStringLiteral("📄 源码执行流动态着色 (选定路径覆盖代码行将以亮绿底色高亮)") :
        QStringLiteral("📄 Source Path Flow Tinting (Executed lines tinted in soft emerald green)"));

    updateKpiBadges();
}

void StaticAnalysisExplorerView::applyTheme(ThemeType type) {
    m_currentTheme = type;
    bool isLight = (type == ThemeType::LightModern);

    // 1. 顶部控制与度量看板卡片
    if (m_topCard) {
        if (isLight) {
            m_topCard->setStyleSheet(QStringLiteral("background: #ffffff; border: 1px solid #dadce0; border-radius: 6px; padding: 4px;"));
        } else {
            m_topCard->setStyleSheet(QStringLiteral("background: #181825; border: 1px solid #313244; border-radius: 6px; padding: 4px;"));
        }
    }

    if (m_lblFile) {
        m_lblFile->setStyleSheet(isLight ?
            QStringLiteral("color: #202124; font-weight: bold; font-size: 13px;") :
            QStringLiteral("color: #cdd6f4; font-weight: bold; font-size: 13px;"));
    }

    if (m_btnAnalyze) {
        if (isLight) {
            m_btnAnalyze->setStyleSheet(QStringLiteral(
                "QPushButton { background-color: #1a73e8; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px; outline: none; }"
                "QPushButton:hover { background-color: #1557b0; }"
            ));
        } else {
            m_btnAnalyze->setStyleSheet(QStringLiteral(
                "QPushButton { background-color: #3b82f6; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px; outline: none; }"
                "QPushButton:hover { background-color: #60a5fa; }"
            ));
        }
    }

    // 2. KPI Badge 样式
    auto setBadgeStyle = [isLight](QLabel* lbl, const QString& lightFg, const QString& darkFg) {
        if (!lbl) return;
        if (isLight) {
            lbl->setStyleSheet(QString("background: #f8f9fa; color: %1; border: 1px solid #dadce0; border-radius: 4px; padding: 4px 8px; font-size: 12px; font-weight: bold;").arg(lightFg));
        } else {
            lbl->setStyleSheet(QString("background: #11111b; color: %1; border: 1px solid #313244; border-radius: 4px; padding: 4px 8px; font-size: 12px; font-weight: bold;").arg(darkFg));
        }
    };
    setBadgeStyle(m_kpiStatements, QStringLiteral("#1a73e8"), QStringLiteral("#89b4fa"));
    setBadgeStyle(m_kpiBranches, QStringLiteral("#b45309"), QStringLiteral("#fab387"));
    setBadgeStyle(m_kpiCommentRatio, QStringLiteral("#16a34a"), QStringLiteral("#a6e3a1"));
    setBadgeStyle(m_kpiMcdc, QStringLiteral("#7c3aed"), QStringLiteral("#f9e2af"));
    setBadgeStyle(m_kpiComplexity, QStringLiteral("#9333ea"), QStringLiteral("#cba6f7"));
    setBadgeStyle(m_kpiIssues, QStringLiteral("#dc2626"), QStringLiteral("#f38ba8"));

    // 3. 表格提示与栏目标题
    if (m_lblTableHint) {
        m_lblTableHint->setStyleSheet(isLight ?
            QStringLiteral("font-weight: bold; color: #1a73e8; padding: 4px 0 2px 2px;") :
            QStringLiteral("font-weight: bold; color: #89b4fa; padding: 4px 0 2px 2px;"));
    }
    if (m_lblFuncTitle) {
        m_lblFuncTitle->setStyleSheet(isLight ?
            QStringLiteral("font-weight: bold; color: #1a73e8; padding: 2px;") :
            QStringLiteral("font-weight: bold; color: #89b4fa; padding: 2px;"));
    }
    if (m_lblPathTitle) {
        m_lblPathTitle->setStyleSheet(isLight ?
            QStringLiteral("font-weight: bold; color: #16a34a; padding: 2px;") :
            QStringLiteral("font-weight: bold; color: #a6e3a1; padding: 2px;"));
    }
    if (m_lblPathStatus) {
        m_lblPathStatus->setStyleSheet(isLight ?
            QStringLiteral("color: #5f6368; font-weight: bold; font-size: 12px;") :
            QStringLiteral("color: #a6adc8; font-weight: bold; font-size: 12px;"));
    }
    if (m_lblRightHeader) {
        m_lblRightHeader->setStyleSheet(isLight ?
            QStringLiteral("font-weight: bold; color: #1a73e8; padding: 2px;") :
            QStringLiteral("font-weight: bold; color: #89b4fa; padding: 2px;"));
    }

    // 4. 双 Tab 选项卡样式适配
    if (m_tabWidget) {
        if (isLight) {
            m_tabWidget->setStyleSheet(QStringLiteral(
                "QTabWidget::pane { border: 1px solid #dadce0; background: #ffffff; border-radius: 4px; }"
                "QTabBar::tab { background: #f1f3f4; color: #5f6368; padding: 6px 14px; border: 1px solid #dadce0; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
                "QTabBar::tab:selected { background: #ffffff; color: #1a73e8; font-weight: bold; border-bottom: 2px solid #1a73e8; }"
                "QTabBar::tab:hover:!selected { background: #e8eaed; }"
            ));
        } else {
            m_tabWidget->setStyleSheet(QStringLiteral(
                "QTabWidget::pane { border: 1px solid #313244; background: #1e1e2e; border-radius: 4px; }"
                "QTabBar::tab { background: #181825; color: #a6adc8; padding: 6px 14px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
                "QTabBar::tab:selected { background: #1e1e2e; color: #89b4fa; font-weight: bold; border-bottom: 2px solid #89b4fa; }"
                "QTabBar::tab:hover:!selected { background: #262738; }"
            ));
        }
    }

    // 5. 三大表格统一样式设置
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

    if (m_overviewTable) m_overviewTable->setStyleSheet(tableStyle);
    if (m_funcTable) m_funcTable->setStyleSheet(tableStyle);
    if (m_pathTable) m_pathTable->setStyleSheet(tableStyle);

    // 6. 子组件主题同步
    if (m_rightCodeEditor) m_rightCodeEditor->applyCurrentTheme();
    if (m_canvasView) m_canvasView->applyTheme();

    // 7. 重新刷新表格条目文字颜色
    populateOverviewTable();
    populateFunctionTable();
    if (!m_currentPaths.isEmpty()) {
        populatePathTable(m_currentPaths);
    }
}

void StaticAnalysisExplorerView::setSourceFile(const QString& filePath, const QString& sourceCode) {
    m_filePath = filePath;
    m_sourceCode = sourceCode;

    QFileInfo fi(filePath);
    bool isZh = (LocalizationManager::instance().currentLanguage() == Language::Chinese);
    m_lblFile->setText(QString("%1: %2").arg(isZh ? QStringLiteral("目标文件") : QStringLiteral("Target"), fi.fileName()));

    m_rightCodeEditor->setPlainText(sourceCode);
}

void StaticAnalysisExplorerView::setAnalysisReport(const StaticAnalysisReport& report, const QString& sourceCode) {
    m_report = report;
    m_functions = report.functions;
    m_filePath = report.filePath;
    m_sourceCode = sourceCode;

    m_rightCodeEditor->setPlainText(sourceCode);

    // 如果未填充全局度量，自动调用计算
    if (m_report.metrics.statementCount == 0 && !sourceCode.isEmpty()) {
        StaticAnalyzer analyzer;
        m_report.metrics = analyzer.computeCodeMetrics(sourceCode, m_filePath, m_functions);
    }

    updateKpiBadges();
    populateOverviewTable();
    populateFunctionTable();
}

void StaticAnalysisExplorerView::updateKpiBadges() {
    bool isZh = (LocalizationManager::instance().currentLanguage() == Language::Chinese);

    m_kpiStatements->setText(QString("<b>%1</b>: %2")
        .arg(isZh ? QStringLiteral("语句数") : QStringLiteral("Statements"))
        .arg(m_report.metrics.statementCount));

    m_kpiBranches->setText(QString("<b>%1</b>: %2")
        .arg(isZh ? QStringLiteral("分支数") : QStringLiteral("Branches"))
        .arg(m_report.metrics.branchCount));

    m_kpiCommentRatio->setText(QString("<b>%1</b>: %2%")
        .arg(isZh ? QStringLiteral("注释率") : QStringLiteral("Comments"))
        .arg(QString::number(m_report.metrics.commentRatio, 'f', 1)));

    m_kpiMcdc->setText(QString("<b>%1</b>: %2/%3")
        .arg(isZh ? QStringLiteral("MC/DC 判定/条件") : QStringLiteral("MC/DC Dec/Cond"))
        .arg(m_report.metrics.mcdcDecisions)
        .arg(m_report.metrics.mcdcConditions));

    m_kpiComplexity->setText(QString("<b>%1</b>: %2")
        .arg(isZh ? QStringLiteral("圈复杂度") : QStringLiteral("Total CC"))
        .arg(m_report.metrics.totalComplexity));

    m_kpiIssues->setText(QString("<b>%1</b>: %2")
        .arg(isZh ? QStringLiteral("缺陷违规") : QStringLiteral("Violations"))
        .arg(m_report.issues.size()));
}

void StaticAnalysisExplorerView::populateOverviewTable() {
    m_overviewTable->setRowCount(0);
    QString baseName = QFileInfo(m_filePath).fileName();
    if (baseName.isEmpty()) baseName = QStringLiteral("source.c");

    for (int i = 0; i < m_functions.size(); ++i) {
        const auto& fn = m_functions[i];
        m_overviewTable->insertRow(i);

        // 1. 序号
        auto* itemIdx = new QTableWidgetItem(QString::number(i + 1));
        itemIdx->setTextAlignment(Qt::AlignCenter);

        // 2. 函数名称
        auto* itemName = new QTableWidgetItem(fn.name);
        itemName->setData(Qt::UserRole, i);

        // 3. 函数路径 / 位置
        QString pathStr = fn.filePath.isEmpty() ? baseName : QFileInfo(fn.filePath).fileName();
        pathStr += QString(" : L%1-L%2").arg(fn.startLine).arg(fn.endLine);
        auto* itemPath = new QTableWidgetItem(pathStr);

        // 4. 语句数
        int stmts = (fn.statementCount > 0) ? fn.statementCount : qMax(1, fn.linesOfCode - 2);
        auto* itemStmts = new QTableWidgetItem(QString::number(stmts));
        itemStmts->setTextAlignment(Qt::AlignCenter);

        // 5. 分支数
        int branches = (fn.branchCount > 0) ? fn.branchCount : qMax(0, fn.cyclomaticComplexity - 1);
        auto* itemBranches = new QTableWidgetItem(QString::number(branches));
        itemBranches->setTextAlignment(Qt::AlignCenter);

        // 6. 圈复杂度
        auto* itemCc = new QTableWidgetItem(QString::number(fn.cyclomaticComplexity));
        itemCc->setTextAlignment(Qt::AlignCenter);

        // 7. 扇入数
        auto* itemFanIn = new QTableWidgetItem(QString::number(fn.fanIn));
        itemFanIn->setTextAlignment(Qt::AlignCenter);

        // 8. 扇出数
        auto* itemFanOut = new QTableWidgetItem(QString::number(fn.fanOut));
        itemFanOut->setTextAlignment(Qt::AlignCenter);

        // 9. 函数深度
        auto* itemFuncDepth = new QTableWidgetItem(QString("%1 层").arg(fn.functionDepth));
        itemFuncDepth->setTextAlignment(Qt::AlignCenter);

        // 10. 调用深度
        auto* itemCallDepth = new QTableWidgetItem(QString("第 %1 层").arg(fn.callDepth));
        itemCallDepth->setTextAlignment(Qt::AlignCenter);

        // 11. MCDC数
        int mcdc = (fn.mcdcCount > 0) ? fn.mcdcCount : (branches > 0 ? branches * 2 : 1);
        auto* itemMcdc = new QTableWidgetItem(QString::number(mcdc));
        itemMcdc->setTextAlignment(Qt::AlignCenter);

        // 复杂度超标警示
        bool isLight = (m_currentTheme == ThemeType::LightModern);
        if (fn.cyclomaticComplexity > 10) {
            QColor danger = isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8"));
            itemName->setForeground(danger);
            itemBranches->setForeground(danger);
            itemCc->setForeground(danger);
        } else if (fn.cyclomaticComplexity > 5) {
            QColor warning = isLight ? QColor(QStringLiteral("#d97706")) : QColor(QStringLiteral("#f9e2af"));
            itemName->setForeground(warning);
            itemCc->setForeground(warning);
        } else {
            QColor ok = isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#a6e3a1"));
            itemName->setForeground(ok);
            itemCc->setForeground(ok);
        }

        m_overviewTable->setItem(i, 0, itemIdx);
        m_overviewTable->setItem(i, 1, itemName);
        m_overviewTable->setItem(i, 2, itemPath);
        m_overviewTable->setItem(i, 3, itemStmts);
        m_overviewTable->setItem(i, 4, itemBranches);
        m_overviewTable->setItem(i, 5, itemCc);
        m_overviewTable->setItem(i, 6, itemFanIn);
        m_overviewTable->setItem(i, 7, itemFanOut);
        m_overviewTable->setItem(i, 8, itemFuncDepth);
        m_overviewTable->setItem(i, 9, itemCallDepth);
        m_overviewTable->setItem(i, 10, itemMcdc);
    }
}

void StaticAnalysisExplorerView::onOverviewTableDoubleClicked(int row, int /* col */) {
    if (row >= 0 && row < m_functions.size()) {
        m_tabWidget->setCurrentIndex(1); // 切换至 Tab 2: 控制流与路径探索
        m_funcTable->selectRow(row);
    }
}

void StaticAnalysisExplorerView::populateFunctionTable() {
    m_funcTable->setRowCount(0);
    m_pathTable->setRowCount(0);
    m_currentPaths.clear();
    m_currentFuncIndex = -1;

    bool isLight = (m_currentTheme == ThemeType::LightModern);
    for (int i = 0; i < m_functions.size(); ++i) {
        const auto& fn = m_functions[i];
        m_funcTable->insertRow(i);

        auto* itemName = new QTableWidgetItem(fn.name);
        auto* itemCc = new QTableWidgetItem(QString::number(fn.cyclomaticComplexity));
        auto* itemLines = new QTableWidgetItem(QString("%1L").arg(fn.linesOfCode));

        itemName->setData(Qt::UserRole, i);
        itemCc->setTextAlignment(Qt::AlignCenter);
        itemLines->setTextAlignment(Qt::AlignCenter);

        if (fn.cyclomaticComplexity > 10) {
            itemCc->setForeground(isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8")));
        } else if (fn.cyclomaticComplexity > 5) {
            itemCc->setForeground(isLight ? QColor(QStringLiteral("#d97706")) : QColor(QStringLiteral("#f9e2af")));
        } else {
            itemCc->setForeground(isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#a6e3a1")));
        }

        m_funcTable->setItem(i, 0, itemName);
        m_funcTable->setItem(i, 1, itemCc);
        m_funcTable->setItem(i, 2, itemLines);
    }

    if (!m_functions.isEmpty()) {
        m_funcTable->selectRow(0);
    }
}

void StaticAnalysisExplorerView::onAnalyzeButtonClicked() {
    if (m_filePath.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先打开一个嵌入式 C 源码文件。"));
        return;
    }
    emit runAnalysisRequested(m_filePath);
}

void StaticAnalysisExplorerView::onFunctionTableSelectionChanged() {
    int row = m_funcTable->currentRow();
    if (row < 0 || row >= m_functions.size()) return;

    m_currentFuncIndex = row;
    const auto& func = m_functions[row];

    QStringList fileLines = m_sourceCode.split(QStringLiteral("\n"));
    m_currentPaths = StaticAnalyzer::extractFunctionPaths(func, fileLines);

    populatePathTable(m_currentPaths);

    // 默认展示该函数的完整 CFG 拓扑
    QString dot = GraphGenerator::generateFunctionCfgDot(func, m_sourceCode);
    LayoutGraph layout = GraphLayoutEngine::computeLayout(dot, &m_report);
    m_canvasView->setGraph(layout);

    m_lblPathStatus->setText(QString("函数: %1() [圈复杂度: %2, 提取独立基路径: %3 条]")
                                 .arg(func.name)
                                 .arg(func.cyclomaticComplexity)
                                 .arg(m_currentPaths.size()));

    // 默认清除右侧源码着色
    m_rightCodeEditor->clearPathHighlights();

    // 选中第一条路径
    if (!m_currentPaths.isEmpty()) {
        m_pathTable->selectRow(0);
    }
}

void StaticAnalysisExplorerView::populatePathTable(const QVector<FunctionPath>& paths) {
    m_pathTable->setRowCount(0);

    bool isLight = (m_currentTheme == ThemeType::LightModern);
    for (int i = 0; i < paths.size(); ++i) {
        const auto& path = paths[i];
        m_pathTable->insertRow(i);

        auto* itemIdx = new QTableWidgetItem(QString("P%1").arg(path.pathId));
        itemIdx->setTextAlignment(Qt::AlignCenter);
        itemIdx->setData(Qt::UserRole, i);
        itemIdx->setForeground(isLight ? QColor(QStringLiteral("#1a73e8")) : QColor(QStringLiteral("#89b4fa")));

        QStringList linesStr;
        for (int l : path.executedLines) {
            linesStr.append(QString::number(l));
        }
        auto* itemLines = new QTableWidgetItem(linesStr.join(QStringLiteral(", ")));

        auto* itemCond = new QTableWidgetItem(path.conditionDescription);
        if (path.isErrorPath) {
            itemCond->setForeground(isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8")));
        } else {
            itemCond->setForeground(isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#a6e3a1")));
        }

        m_pathTable->setItem(i, 0, itemIdx);
        m_pathTable->setItem(i, 1, itemLines);
        m_pathTable->setItem(i, 2, itemCond);
    }
}

void StaticAnalysisExplorerView::onPathTableSelectionChanged() {
    int row = m_pathTable->currentRow();
    if (row < 0 || row >= m_currentPaths.size() || m_currentFuncIndex < 0) return;

    const auto& func = m_functions[m_currentFuncIndex];
    const auto& path = m_currentPaths[row];

    renderSelectedPath(func, path);
}

void StaticAnalysisExplorerView::renderSelectedPath(const FunctionMetrics& func, const FunctionPath& path) {
    // 1. 使用 Graphviz 生成并高亮当前路径的控制流图 (点亮节点与跳转边)
    QString dot = GraphGenerator::generateHighlightedPathCfgDot(func, m_sourceCode, path);
    LayoutGraph layout = GraphLayoutEngine::computeLayout(dot, &m_report);
    m_canvasView->setGraph(layout);

    m_lblPathStatus->setText(QString("▶ 当前高亮路径: P%1 (%2) | 覆盖 %3 行")
                                 .arg(path.pathId)
                                 .arg(path.conditionDescription)
                                 .arg(path.executedLines.size()));

    // 2. 联动在右侧源代码视窗中，对该路径经过的代码行进行柔和翠绿高亮着色
    m_rightCodeEditor->highlightPathLines(path.executedLines);

    // 自动滚动使第一行执行代码居中
    if (!path.executedLines.isEmpty()) {
        m_rightCodeEditor->gotoLine(path.executedLines.first(), 1);
    }
}

void StaticAnalysisExplorerView::onGraphNodeClicked(const QString& name, int startLine, int endLine) {
    (void)name;
    (void)endLine;
    if (startLine > 0) {
        emit jumpToEditorLine(startLine, 1);
    }
}

} // namespace Coverage
