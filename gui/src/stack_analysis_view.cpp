#include "stack_analysis_view.h"
#include "localization_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QTextStream>
#include <QFile>
#include <QDialog>
#include <QRegularExpression>
#include <cmath>

namespace Coverage {

StackAnalysisView::StackAnalysisView(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    applyTheme(ThemeManager::currentTheme());
}

void StackAnalysisView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // 1. 顶部控制栏 (二进制文件选择、.su 文件选择与操作按钮)
    auto* barWidget = new QWidget(this);
    auto* barMainLayout = new QVBoxLayout(barWidget);
    barMainLayout->setContentsMargins(4, 4, 4, 4);
    barMainLayout->setSpacing(4);

    auto* row1Widget = new QWidget(barWidget);
    auto* row1Layout = new QHBoxLayout(row1Widget);
    row1Layout->setContentsMargins(0, 0, 0, 0);
    row1Layout->setSpacing(6);

    auto* lblBin = new QLabel(QStringLiteral("目标程序:"), row1Widget);
    m_editBinaryPath = new QLineEdit(row1Widget);
    m_editBinaryPath->setPlaceholderText(QStringLiteral("选择编译出的可执行文件或目标文件 (.exe, .elf, .out, .o)"));
    m_btnBrowseBinary = new QPushButton(QStringLiteral("浏览..."), row1Widget);
    m_btnBrowseBinary->setFocusPolicy(Qt::NoFocus);

    m_btnAnalyze = new QPushButton(QIcon(QStringLiteral(":/icons/stack_analysis.svg")), QStringLiteral("执行静态堆栈分析"), row1Widget);
    m_btnAnalyze->setFocusPolicy(Qt::NoFocus);
    m_btnAnalyze->setStyleSheet(QStringLiteral("font-weight: bold; padding: 4px 12px;"));

    row1Layout->addWidget(lblBin);
    row1Layout->addWidget(m_editBinaryPath, 1);
    row1Layout->addWidget(m_btnBrowseBinary);
    row1Layout->addWidget(m_btnAnalyze);

    auto* row2Widget = new QWidget(barWidget);
    auto* row2Layout = new QHBoxLayout(row2Widget);
    row2Layout->setContentsMargins(0, 0, 0, 0);
    row2Layout->setSpacing(6);

    auto* lblSu = new QLabel(QStringLiteral("堆栈文件 (.su):"), row2Widget);
    m_editSuPath = new QLineEdit(row2Widget);
    m_editSuPath->setPlaceholderText(QStringLiteral("可选 GCC -fstack-usage 导出的 .su 文件 (留空自动检索)"));
    m_btnBrowseSu = new QPushButton(QStringLiteral("浏览..."), row2Widget);
    m_btnBrowseSu->setFocusPolicy(Qt::NoFocus);

    m_btnInstructions = new QPushButton(QIcon(QStringLiteral(":/icons/user_manual.svg")), QStringLiteral("📖 操作说明"), row2Widget);
    m_btnInstructions->setFocusPolicy(Qt::NoFocus);

    m_btnExport = new QPushButton(QIcon(QStringLiteral(":/icons/export_report.svg")), QStringLiteral("导出报告"), row2Widget);
    m_btnExport->setFocusPolicy(Qt::NoFocus);

    row2Layout->addWidget(lblSu);
    row2Layout->addWidget(m_editSuPath, 1);
    row2Layout->addWidget(m_btnBrowseSu);
    row2Layout->addWidget(m_btnInstructions);
    row2Layout->addWidget(m_btnExport);

    barMainLayout->addWidget(row1Widget);
    barMainLayout->addWidget(row2Widget);

    mainLayout->addWidget(barWidget);

    connect(m_btnBrowseBinary, &QPushButton::clicked, this, &StackAnalysisView::onBrowseBinary);
    connect(m_btnBrowseSu, &QPushButton::clicked, this, &StackAnalysisView::onBrowseSu);
    connect(m_btnAnalyze, &QPushButton::clicked, this, &StackAnalysisView::onAnalyzeClicked);
    connect(m_btnInstructions, &QPushButton::clicked, this, &StackAnalysisView::onInstructionsClicked);
    connect(m_btnExport, &QPushButton::clicked, this, &StackAnalysisView::onExportClicked);

    // 2. KPI 看板卡片行
    m_topCardWidget = new QWidget(this);
    auto* kpiLayout = new QHBoxLayout(m_topCardWidget);
    kpiLayout->setContentsMargins(0, 0, 0, 0);
    kpiLayout->setSpacing(8);

    auto createKpiCard = [this](QWidget*& cardWidget, QLabel*& lblVal, const QString& titleText) {
        cardWidget = new QWidget(m_topCardWidget);
        auto* lay = new QVBoxLayout(cardWidget);
        lay->setContentsMargins(10, 6, 10, 6);
        lay->setSpacing(2);

        auto* lblT = new QLabel(titleText, cardWidget);
        lblT->setObjectName(QStringLiteral("kpiTitle"));
        lblVal = new QLabel(QStringLiteral("--"), cardWidget);
        lblVal->setObjectName(QStringLiteral("kpiValue"));
        lblVal->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold;"));

        lay->addWidget(lblT);
        lay->addWidget(lblVal);
        return cardWidget;
    };

    kpiLayout->addWidget(createKpiCard(m_cardMaxStack, m_lblKpiMaxStack, QStringLiteral("⚡ 全局最大调用栈 (Worst-Case)")));
    kpiLayout->addWidget(createKpiCard(m_cardRootFunc, m_lblKpiRootFunc, QStringLiteral("📍 最深调用栈根入口 (Root)")));
    kpiLayout->addWidget(createKpiCard(m_cardTotalFuncs, m_lblKpiTotalFuncs, QStringLiteral("📦 解析函数总数 (Functions)")));
    kpiLayout->addWidget(createKpiCard(m_cardRecursion, m_lblKpiRecursion, QStringLiteral("🛡️ 递归违规合规检查 (MISRA 17.2)")));

    mainLayout->addWidget(m_topCardWidget);

    // 3. 主展示区分割器 (左侧：函数堆栈大表；右侧：调用链树/Graphviz画布与反汇编)
    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 3.1 左侧函数表格 (包含行号、函数名称、所在源码文件、自身堆栈大小、最大调用堆栈深度、调用类型、递归风险)
    auto* leftWidget = new QWidget(mainSplitter);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    m_lblTableTitle = new QLabel(QStringLiteral("📊 函数局部栈大小与最大调用栈深度清单:"), leftWidget);
    m_lblTableTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    leftLayout->addWidget(m_lblTableTitle);

    m_tableFunctions = new QTableWidget(leftWidget);
    m_tableFunctions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableFunctions->setColumnCount(8);
    m_tableFunctions->setHorizontalHeaderLabels({
        QStringLiteral("序号"),
        QStringLiteral("函数名称"),
        QStringLiteral("所在源码文件"),
        QStringLiteral("行号"),
        QStringLiteral("自身栈大小 (B)"),
        QStringLiteral("最大调用栈 (B)"),
        QStringLiteral("调用类型"),
        QStringLiteral("递归风险")
    });
    m_tableFunctions->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableFunctions->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableFunctions->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableFunctions->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableFunctions->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tableFunctions->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_tableFunctions->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_tableFunctions->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_tableFunctions->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableFunctions->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableFunctions->verticalHeader()->setVisible(false);
    leftLayout->addWidget(m_tableFunctions);

    connect(m_tableFunctions, &QTableWidget::itemSelectionChanged, this, &StackAnalysisView::onFunctionTableSelectionChanged);
    mainSplitter->addWidget(leftWidget);

    // 3.2 右侧垂直分割器 (上：调用链树与 Graphviz 拓扑图双选项卡；下：反汇编指令检视)
    auto* rightSplitter = new QSplitter(Qt::Vertical, mainSplitter);

    m_chainTabs = new QTabWidget(rightSplitter);

    // Tab 1: 最深调用链树形清单
    auto* treeTabWidget = new QWidget(m_chainTabs);
    auto* treeTabLayout = new QVBoxLayout(treeTabWidget);
    treeTabLayout->setContentsMargins(2, 2, 2, 2);
    treeTabLayout->setSpacing(4);

    auto* treeTopBar = new QHBoxLayout();
    auto* lblChainTitle = new QLabel(QStringLiteral("🌲 最大调用栈路径调用序列 (点击任意节点自动渲染矢量拓扑图):"), treeTabWidget);
    lblChainTitle->setStyleSheet(QStringLiteral("font-weight: bold; color: #89b4fa;"));
    m_btnRenderGraph = new QPushButton(QIcon(QStringLiteral(":/icons/path_explorer.svg")), QStringLiteral("🎨 渲染拓扑调用图"), treeTabWidget);
    m_btnRenderGraph->setFocusPolicy(Qt::NoFocus);
    connect(m_btnRenderGraph, &QPushButton::clicked, this, &StackAnalysisView::onRenderGraphClicked);

    treeTopBar->addWidget(lblChainTitle);
    treeTopBar->addStretch();
    treeTopBar->addWidget(m_btnRenderGraph);
    treeTabLayout->addLayout(treeTopBar);

    m_treeWorstChain = new QTreeWidget(treeTabWidget);
    m_treeWorstChain->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeWorstChain->setHeaderLabels({QStringLiteral("调用节点 / 函数"), QStringLiteral("局部栈"), QStringLiteral("累计栈深度")});
    m_treeWorstChain->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWorstChain->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWorstChain->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(m_treeWorstChain, &QTreeWidget::itemClicked, this, &StackAnalysisView::onWorstChainItemClicked);
    treeTabLayout->addWidget(m_treeWorstChain);

    m_chainTabs->addTab(treeTabWidget, QIcon(QStringLiteral(":/icons/static_analysis.svg")), QStringLiteral("🌲 调用链清单 (Tree)"));

    // Tab 2: Graphviz 矢量调用拓扑图
    auto* graphTabWidget = new QWidget(m_chainTabs);
    auto* graphTabLayout = new QVBoxLayout(graphTabWidget);
    graphTabLayout->setContentsMargins(2, 2, 2, 2);
    graphTabLayout->setSpacing(4);

    auto* graphBar = new QHBoxLayout();
    auto* lblGraphHint = new QLabel(QStringLiteral("🎨 Graphviz 最深调用栈边节点连线拓扑 (矢量无失真渲染):"), graphTabWidget);
    lblGraphHint->setStyleSheet(QStringLiteral("font-weight: bold; color: #a6e3a1;"));
    auto* btnZoomIn = new QPushButton(QIcon(QStringLiteral(":/icons/zoom_in.svg")), QStringLiteral("放大"), graphTabWidget);
    auto* btnZoomOut = new QPushButton(QIcon(QStringLiteral(":/icons/zoom_out.svg")), QStringLiteral("缩小"), graphTabWidget);
    auto* btnFit = new QPushButton(QIcon(QStringLiteral(":/icons/fit_window.svg")), QStringLiteral("适应"), graphTabWidget);

    graphBar->addWidget(lblGraphHint);
    graphBar->addStretch();
    graphBar->addWidget(btnZoomIn);
    graphBar->addWidget(btnZoomOut);
    graphBar->addWidget(btnFit);
    graphTabLayout->addLayout(graphBar);

    m_chainCanvasView = new GraphCanvasView(graphTabWidget);
    connect(btnZoomIn, &QPushButton::clicked, m_chainCanvasView, &GraphCanvasView::zoomIn);
    connect(btnZoomOut, &QPushButton::clicked, m_chainCanvasView, &GraphCanvasView::zoomOut);
    connect(btnFit, &QPushButton::clicked, m_chainCanvasView, &GraphCanvasView::fitToWindow);
    graphTabLayout->addWidget(m_chainCanvasView);

    m_chainTabs->addTab(graphTabWidget, QIcon(QStringLiteral(":/icons/path_explorer.svg")), QStringLiteral("🎨 拓扑调用图 (Graphviz)"));

    rightSplitter->addWidget(m_chainTabs);

    // 下半部分: 反汇编指令检视
    auto* rightBottomWidget = new QWidget(rightSplitter);
    auto* rightBottomLayout = new QVBoxLayout(rightBottomWidget);
    rightBottomLayout->setContentsMargins(0, 0, 0, 0);
    rightBottomLayout->setSpacing(4);

    m_lblDisasmHeader = new QLabel(QStringLiteral("🔍 函数反汇编与指令级堆栈分配检视:"), rightBottomWidget);
    m_lblDisasmHeader->setStyleSheet(QStringLiteral("font-weight: bold;"));
    rightBottomLayout->addWidget(m_lblDisasmHeader);

    m_txtDisasm = new QTextEdit(rightBottomWidget);
    m_txtDisasm->setReadOnly(true);
    m_txtDisasm->setFont(QFont(QStringLiteral("Consolas, monospace"), 10));
    rightBottomLayout->addWidget(m_txtDisasm);
    rightSplitter->addWidget(rightBottomWidget);

    rightSplitter->setStretchFactor(0, 5);
    rightSplitter->setStretchFactor(1, 5);

    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setStretchFactor(0, 5);
    mainSplitter->setStretchFactor(1, 5);

    mainLayout->addWidget(mainSplitter, 1);
}

void StackAnalysisView::setBinaryPath(const QString& path) {
    m_editBinaryPath->setText(path);
    QFileInfo fi(path);
    QString candSu = fi.absolutePath() + QStringLiteral("/") + fi.baseName() + QStringLiteral(".su");
    if (QFile::exists(candSu)) {
        m_editSuPath->setText(candSu);
    }
}

void StackAnalysisView::onBrowseBinary() {
    QString p = QFileDialog::getOpenFileName(this, QStringLiteral("选择待分析的二进制可执行文件"),
        m_editBinaryPath->text().isEmpty() ? QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis/bin") : m_editBinaryPath->text(),
        QStringLiteral("可执行程序与目标文件 (*.exe *.elf *.out *.axf *.o *.obj);;所有文件 (*.*)"));
    if (!p.isEmpty()) {
        setBinaryPath(p);
    }
}

void StackAnalysisView::onBrowseSu() {
    QString p = QFileDialog::getOpenFileName(this, QStringLiteral("选择 GCC 栈用量记录文件 (.su)"),
        m_editBinaryPath->text().isEmpty() ? QString() : QFileInfo(m_editBinaryPath->text()).absolutePath(),
        QStringLiteral("GCC Stack Usage (*.su);;所有文件 (*.*)"));
    if (!p.isEmpty()) {
        m_editSuPath->setText(p);
    }
}

void StackAnalysisView::onInstructionsClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("静态堆栈与二进制调用深度分析器 · 操作说明"));
    dlg.resize(760, 520);

    auto* lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(12);

    auto* txt = new QTextEdit(&dlg);
    txt->setReadOnly(true);
    txt->setFont(QFont(QStringLiteral("Segoe UI, Microsoft YaHei"), 10));

    QString html = QStringLiteral(
        "<h2>🛠️ 静态堆栈与二进制调用深度分析器操作指南</h2>"
        "<p>本工具面向 <b>ISO 26262 ASIL-D</b> 与 <b>DO-178C Level A</b> 高安全性嵌入式系统研发场景，提供完全静态、无插装运行开销的二进制最大栈消耗求解与调用路径可视化。</p>"
        "<hr/>"
        "<h3>1. 编译前置配置说明 (推荐使用 GCC 编译器)</h3>"
        "<ul>"
        "  <li>在编译嵌入式源码时，推荐开启 <code>-fstack-usage</code> 编译选项：<br/>"
        "      <code>gcc -O2 -fstack-usage -g -c main.c -o main.o</code></li>"
        "  <li>编译器将为每个源文件生成同名 <code>.su</code> 文件，记录每个函数的精确局部帧字节大小及类型 (<code>static</code> / <code>dynamic</code>)。</li>"
        "  <li><b>若未生成 .su 文件：</b>软件将自动调用内置 <code>objdump</code> 反汇编二进制程序，扫描 <code>sub $0xXX, %rsp</code> 或 <code>push</code> 指令提取局部栈。</li>"
        "</ul>"
        "<h3>2. 操作使用流程</h3>"
        "<ol>"
        "  <li><b>指定目标程序：</b>点击顶部【浏览...】按钮，选择编译生成的 <code>.exe</code>、<code>.elf</code>、<code>.axf</code> 或 <code>.o</code> 文件。</li>"
        "  <li><b>加载栈记录 (.su)：</b>若存在对应 <code>.su</code> 文件，软件会自动在同目录及工程 Source 目录下匹配加载；用户亦可手动指定。</li>"
        "  <li><b>执行分析：</b>点击【⚡ 执行静态堆栈分析】按钮，系统将自动完成反汇编、符号解析、全局调用拓扑构建与 DFS 最坏情况路径推导。</li>"
        "  <li><b>查看结果与指标：</b>"
        "    <ul>"
        "      <li><b>全局最大调用栈：</b>显示程序在运行过程中可能达到的最坏情况最大栈深度峰值；</li>"
        "      <li><b>函数清单列表：</b>点击任一行即可查看对应函数的反汇编汇编代码与入栈分配指令；</li>"
        "      <li><b>MISRA 17.2 规约检查：</b>自动分析并警示代码中的直接或间接递归风险；</li>"
        "    </ul>"
        "  </li>"
        "  <li><b>Graphviz 拓扑连线渲染：</b>在右侧【调用链清单】中点击任意节点或点击【🎨 渲染拓扑调用图】，系统将自动使用 Graphviz 渲染出由根节点至最深子函数的连线拓扑图。</li>"
        "</ol>"
        "<h3>3. 报告导出</h3>"
        "<p>点击【导出报告】按钮，可将完整的静态堆栈分析结果导出为格式化纯文本、Markdown 或 JSON 格式，方便纳入功能安全设计验证文档。</p>"
    );
    txt->setHtml(html);
    lay->addWidget(txt);

    auto* btnClose = new QPushButton(QStringLiteral("关闭 (Close)"), &dlg);
    btnClose->setFixedWidth(100);
    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);
    auto* bLay = new QHBoxLayout();
    bLay->addStretch();
    bLay->addWidget(btnClose);
    lay->addLayout(bLay);

    dlg.exec();
}

void StackAnalysisView::onAnalyzeClicked() {
    QString bin = m_editBinaryPath->text().trimmed();
    if (bin.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先指定待分析的二进制程序路径！"));
        return;
    }

    if (!QFile::exists(bin)) {
        QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("指定的二进制程序文件不存在！"));
        return;
    }

    QString su = m_editSuPath->text().trimmed();
    emit runAnalysisRequested(bin, su);

    StackAnalysisReport rep = StackAnalyzer::analyzeBinary(bin, su);
    setReport(rep);
}

void StackAnalysisView::onExportClicked() {
    if (m_report.functions.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("暂无分析数据，请先执行静态堆栈分析。"));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出堆栈分析报告"),
        QStringLiteral("reports/stack_analysis_report.txt"),
        QStringLiteral("Text Report (*.txt);;Markdown Report (*.md);;JSON (*.json)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("无法写入目标报告文件！"));
        return;
    }

    if (path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        QJsonDocument doc(m_report.toJson());
        file.write(doc.toJson(QJsonDocument::Indented));
    } else {
        QString text = StackAnalyzer::exportReportToText(m_report);
        file.write(text.toUtf8());
    }
    file.close();

    QMessageBox::information(this, QStringLiteral("导出成功"),
        QStringLiteral("静态堆栈与调用深度分析报告已成功导出至:\n%1").arg(path));
}

void StackAnalysisView::setReport(const StackAnalysisReport& report) {
    m_report = report;
    updateKpiCards();
    populateFunctionTable();
    displayWorstCaseChain();
    renderWorstCaseCallGraph();

    if (!m_report.functions.isEmpty()) {
        m_tableFunctions->selectRow(0);
    }
}

void StackAnalysisView::updateKpiCards() {
    bool isLight = (m_currentTheme == ThemeType::LightModern);

    // 1. 最大调用栈
    m_lblKpiMaxStack->setText(QString("%1 字节 (Bytes)").arg(m_report.maxWorstCaseStackBytes));
    if (m_report.maxWorstCaseStackBytes > 2048) {
        m_lblKpiMaxStack->setStyleSheet(isLight ?
            QStringLiteral("font-size: 15px; font-weight: bold; color: #dc2626;") :
            QStringLiteral("font-size: 15px; font-weight: bold; color: #f38ba8;"));
    } else if (m_report.maxWorstCaseStackBytes > 512) {
        m_lblKpiMaxStack->setStyleSheet(isLight ?
            QStringLiteral("font-size: 15px; font-weight: bold; color: #b45309;") :
            QStringLiteral("font-size: 15px; font-weight: bold; color: #fab387;"));
    } else {
        m_lblKpiMaxStack->setStyleSheet(isLight ?
            QStringLiteral("font-size: 15px; font-weight: bold; color: #16a34a;") :
            QStringLiteral("font-size: 15px; font-weight: bold; color: #a6e3a1;"));
    }

    // 2. 根函数
    m_lblKpiRootFunc->setText(m_report.worstCaseRootFunction.isEmpty() ? QStringLiteral("无") : m_report.worstCaseRootFunction + QStringLiteral("()"));
    m_lblKpiRootFunc->setStyleSheet(isLight ?
        QStringLiteral("font-size: 14px; font-weight: bold; color: #1a73e8;") :
        QStringLiteral("font-size: 14px; font-weight: bold; color: #89b4fa;"));

    // 3. 函数总数
    m_lblKpiTotalFuncs->setText(QString("%1 个有效函数").arg(m_report.totalFunctions));
    m_lblKpiTotalFuncs->setStyleSheet(isLight ?
        QStringLiteral("font-size: 15px; font-weight: bold; color: #202124;") :
        QStringLiteral("font-size: 15px; font-weight: bold; color: #cdd6f4;"));

    // 4. 递归违规
    if (m_report.hasRecursion) {
        m_lblKpiRecursion->setText(QStringLiteral("⚠️ 发现递归环路 (违背 MISRA 17.2)"));
        m_lblKpiRecursion->setStyleSheet(isLight ?
            QStringLiteral("font-size: 13px; font-weight: bold; color: #dc2626;") :
            QStringLiteral("font-size: 13px; font-weight: bold; color: #f38ba8;"));
    } else {
        m_lblKpiRecursion->setText(QStringLiteral("✅ 确定性通过 (无递归)"));
        m_lblKpiRecursion->setStyleSheet(isLight ?
            QStringLiteral("font-size: 14px; font-weight: bold; color: #16a34a;") :
            QStringLiteral("font-size: 14px; font-weight: bold; color: #a6e3a1;"));
    }
}

void StackAnalysisView::populateFunctionTable() {
    m_tableFunctions->setRowCount(0);

    bool isLight = (m_currentTheme == ThemeType::LightModern);
    for (int i = 0; i < m_report.functions.size(); ++i) {
        const auto& fn = m_report.functions[i];
        m_tableFunctions->insertRow(i);

        // 0: 序号
        auto* itemIdx = new QTableWidgetItem(QString::number(i + 1));
        itemIdx->setTextAlignment(Qt::AlignCenter);

        // 1: 函数名称
        auto* itemName = new QTableWidgetItem(fn.name);
        itemName->setData(Qt::UserRole, i);

        // 2: 所在源码文件
        QString fileStr = fn.sourceFile.isEmpty() ? QStringLiteral("-") : fn.sourceFile;
        auto* itemFile = new QTableWidgetItem(fileStr);
        itemFile->setTextAlignment(Qt::AlignCenter);

        // 3: 行号
        QString lineStr = (fn.line > 0) ? QString("L%1").arg(fn.line) : QStringLiteral("-");
        auto* itemLine = new QTableWidgetItem(lineStr);
        itemLine->setTextAlignment(Qt::AlignCenter);

        // 4: 自身堆栈大小
        auto* itemLocal = new QTableWidgetItem(QString::number(fn.localStackBytes));
        itemLocal->setTextAlignment(Qt::AlignCenter);

        // 5: 最大调用栈
        auto* itemMax = new QTableWidgetItem(QString::number(fn.maxCallStackBytes));
        itemMax->setTextAlignment(Qt::AlignCenter);

        // 6: 调用类型
        auto* itemType = new QTableWidgetItem(fn.frameType);
        itemType->setTextAlignment(Qt::AlignCenter);

        // 7: 递归风险
        auto* itemRec = new QTableWidgetItem(fn.isRecursive ? QStringLiteral("⚠️ 递归风险") : QStringLiteral("安全"));
        itemRec->setTextAlignment(Qt::AlignCenter);

        if (fn.isRecursive) {
            itemRec->setForeground(isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8")));
        } else {
            itemRec->setForeground(isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#a6e3a1")));
        }

        if (fn.maxCallStackBytes == m_report.maxWorstCaseStackBytes && m_report.maxWorstCaseStackBytes > 0) {
            itemName->setForeground(isLight ? QColor(QStringLiteral("#b45309")) : QColor(QStringLiteral("#fab387")));
            itemMax->setForeground(isLight ? QColor(QStringLiteral("#b45309")) : QColor(QStringLiteral("#fab387")));
        }

        m_tableFunctions->setItem(i, 0, itemIdx);
        m_tableFunctions->setItem(i, 1, itemName);
        m_tableFunctions->setItem(i, 2, itemFile);
        m_tableFunctions->setItem(i, 3, itemLine);
        m_tableFunctions->setItem(i, 4, itemLocal);
        m_tableFunctions->setItem(i, 5, itemMax);
        m_tableFunctions->setItem(i, 6, itemType);
        m_tableFunctions->setItem(i, 7, itemRec);
    }
}

void StackAnalysisView::onFunctionTableSelectionChanged() {
    int row = m_tableFunctions->currentRow();
    if (row < 0 || row >= m_report.functions.size()) return;

    const auto& fn = m_report.functions[row];
    displayFunctionDetails(fn);
}

void StackAnalysisView::displayFunctionDetails(const StackFunctionInfo& info) {
    m_lblDisasmHeader->setText(QString("🔍 函数 %1() 反汇编与局部栈分配指令检视 [%2:%3 | 局部栈: %4 字节 | 最大栈: %5 字节]:")
        .arg(info.name)
        .arg(info.sourceFile.isEmpty() ? QStringLiteral("未知文件") : info.sourceFile)
        .arg(info.line > 0 ? QString::number(info.line) : QStringLiteral("-"))
        .arg(info.localStackBytes)
        .arg(info.maxCallStackBytes));

    m_txtDisasm->setPlainText(info.disassembly);
}

void StackAnalysisView::displayWorstCaseChain() {
    m_treeWorstChain->clear();

    if (m_report.worstCaseCallChain.isEmpty()) {
        auto* item = new QTreeWidgetItem(m_treeWorstChain, {QStringLiteral("暂无调用链数据"), QStringLiteral("-"), QStringLiteral("-")});
        return;
    }

    int cumulative = 0;
    QTreeWidgetItem* parentItem = nullptr;

    for (int i = 0; i < m_report.worstCaseCallChain.size(); ++i) {
        QString stepStr = m_report.worstCaseCallChain[i];
        QRegularExpression reg(QStringLiteral(R"(^(.*?)\s*\((\d+)B\))"));
        auto m = reg.match(stepStr);
        QString fnName = stepStr;
        int bytes = 0;
        if (m.hasMatch()) {
            fnName = m.captured(1).trimmed();
            bytes = m.captured(2).toInt();
        }
        cumulative += bytes;

        QTreeWidgetItem* curItem = nullptr;
        if (parentItem == nullptr) {
            curItem = new QTreeWidgetItem(m_treeWorstChain, {QString("▶ %1()").arg(fnName), QString("%1 B").arg(bytes), QString("%1 B").arg(cumulative)});
        } else {
            curItem = new QTreeWidgetItem(parentItem, {QString("↳ %1()").arg(fnName), QString("%1 B").arg(bytes), QString("%1 B").arg(cumulative)});
        }

        curItem->setData(0, Qt::UserRole, fnName);
        curItem->setForeground(0, QColor(QStringLiteral("#89b4fa")));
        curItem->setForeground(1, QColor(QStringLiteral("#fab387")));
        curItem->setForeground(2, QColor(QStringLiteral("#a6e3a1")));
        curItem->setExpanded(true);

        parentItem = curItem;
    }

    m_treeWorstChain->expandAll();
}

void StackAnalysisView::onWorstChainItemClicked(QTreeWidgetItem* item, int /* column */) {
    if (!item) return;
    QString fnName = item->data(0, Qt::UserRole).toString();
    for (int i = 0; i < m_report.functions.size(); ++i) {
        if (m_report.functions[i].name == fnName) {
            m_tableFunctions->selectRow(i);
            break;
        }
    }
    // 渲染拓扑调用图并切换至图谱标签页
    onRenderGraphClicked();
}

void StackAnalysisView::onRenderGraphClicked() {
    renderWorstCaseCallGraph();
    if (m_chainTabs) {
        m_chainTabs->setCurrentIndex(1); // 切换至 Graphviz 拓扑图标签
    }
}

void StackAnalysisView::renderWorstCaseCallGraph() {
    if (m_report.worstCaseCallChain.isEmpty()) {
        return;
    }

    bool isLight = (m_currentTheme == ThemeType::LightModern);

    QString dot;
    dot += QStringLiteral("digraph StackCallGraph {\n");
    dot += QStringLiteral("    rankdir=TB;\n");
    dot += QString("    bgcolor=\"%1\";\n").arg(isLight ? QStringLiteral("#ffffff") : QStringLiteral("#181825"));
    dot += QStringLiteral("    node [fontname=\"Consolas, Segoe UI\", fontsize=10, shape=box, style=\"rounded,filled\", margin=\"0.25,0.15\", penwidth=2.0];\n");
    dot += QString("    edge [fontname=\"Segoe UI\", fontsize=9, color=\"%1\", fontcolor=\"%2\", arrowsize=0.9, penwidth=2.0];\n\n")
               .arg(isLight ? QStringLiteral("#16a34a") : QStringLiteral("#10b981"))
               .arg(isLight ? QStringLiteral("#16a34a") : QStringLiteral("#a6e3a1"));

    QString prevFunc;
    for (int i = 0; i < m_report.worstCaseCallChain.size(); ++i) {
        QString stepStr = m_report.worstCaseCallChain[i];
        QRegularExpression reg(QStringLiteral(R"(^(.*?)\s*\((\d+)B\))"));
        auto m = reg.match(stepStr);
        QString fnName = stepStr;
        int bytes = 0;
        if (m.hasMatch()) {
            fnName = m.captured(1).trimmed();
            bytes = m.captured(2).toInt();
        }

        QString fillColor;
        QString fontColor;
        QString borderColor = isLight ? QStringLiteral("#1a73e8") : QStringLiteral("#10b981");

        if (isLight) {
            if (i == 0) {
                fillColor = QStringLiteral("#e8f0fe");
                fontColor = QStringLiteral("#1a73e8");
                borderColor = QStringLiteral("#1a73e8");
            } else if (i == m_report.worstCaseCallChain.size() - 1) {
                fillColor = QStringLiteral("#fef3c7");
                fontColor = QStringLiteral("#92400e");
                borderColor = QStringLiteral("#d97706");
            } else {
                fillColor = QStringLiteral("#f1f5f9");
                fontColor = QStringLiteral("#0f172a");
                borderColor = QStringLiteral("#64748b");
            }
        } else {
            if (i == 0) {
                fillColor = QStringLiteral("#89b4fa");
                fontColor = QStringLiteral("#11111b");
            } else if (i == m_report.worstCaseCallChain.size() - 1) {
                fillColor = QStringLiteral("#fab387");
                fontColor = QStringLiteral("#11111b");
            } else {
                fillColor = QStringLiteral("#3b82f6");
                fontColor = QStringLiteral("#ffffff");
            }
        }

        QString label = QString("%1()\\n[局部栈: %2 B]").arg(fnName).arg(bytes);
        dot += QString("    \"%1\" [fillcolor=\"%2\", color=\"%3\", fontcolor=\"%4\", label=\"%5\"];\n")
                   .arg(fnName, fillColor, borderColor, fontColor, label);

        if (!prevFunc.isEmpty()) {
            dot += QString("    \"%1\" -> \"%2\" [label=\"+%3 B\"];\n").arg(prevFunc, fnName).arg(bytes);
        }
        prevFunc = fnName;
    }

    dot += QStringLiteral("}\n");

    LayoutGraph layout = GraphLayoutEngine::computeLayout(dot, nullptr);
    if (m_chainCanvasView) {
        m_chainCanvasView->setGraph(layout);
    }
}

void StackAnalysisView::applyTheme(ThemeType type) {
    m_currentTheme = type;
    bool isLight = (type == ThemeType::LightModern);

    QString cardBg = isLight ? QStringLiteral("#ffffff") : QStringLiteral("#181825");
    QString cardBorder = isLight ? QStringLiteral("#dadce0") : QStringLiteral("#313244");
    QString cardText = isLight ? QStringLiteral("#5f6368") : QStringLiteral("#a6adc8");

    QString cardStyle = QString("QWidget { background-color: %1; border: 1px solid %2; border-radius: 6px; } "
                                "QLabel#kpiTitle { color: %3; font-size: 11px; border: none; background: transparent; font-weight: 500; }")
                            .arg(cardBg, cardBorder, cardText);

    if (m_cardMaxStack) m_cardMaxStack->setStyleSheet(cardStyle);
    if (m_cardRootFunc) m_cardRootFunc->setStyleSheet(cardStyle);
    if (m_cardTotalFuncs) m_cardTotalFuncs->setStyleSheet(cardStyle);
    if (m_cardRecursion) m_cardRecursion->setStyleSheet(cardStyle);

    if (m_btnAnalyze) {
        if (isLight) {
            m_btnAnalyze->setStyleSheet(QStringLiteral("QPushButton { background-color: #1a73e8; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px; outline: none; }"
                                                       "QPushButton:hover { background-color: #1557b0; }"));
        } else {
            m_btnAnalyze->setStyleSheet(QStringLiteral("QPushButton { background-color: #89b4fa; color: #11111b; font-weight: bold; padding: 6px 14px; border-radius: 4px; outline: none; }"
                                                       "QPushButton:hover { background-color: #b4befe; }"));
        }
    }

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
    if (m_tableFunctions) m_tableFunctions->setStyleSheet(tableStyle);

    QString treeStyle = isLight ?
        QStringLiteral(
            "QTreeWidget { background-color: #ffffff; alternate-background-color: #f8f9fa; border: 1px solid #dadce0; color: #202124; }"
            "QHeaderView::section { background-color: #f1f3f4; color: #1a73e8; font-weight: bold; border: 1px solid #dadce0; padding: 6px; }"
            "QTreeWidget::item:selected { background-color: #e8f0fe; color: #1a73e8; font-weight: 500; }"
        ) :
        QStringLiteral(
            "QTreeWidget { background-color: #1e1e2e; alternate-background-color: #181825; border: 1px solid #313244; color: #cdd6f4; }"
            "QHeaderView::section { background-color: #181825; color: #89b4fa; font-weight: bold; border: 1px solid #313244; padding: 6px; }"
            "QTreeWidget::item:selected { background-color: #45475a; color: #a6e3a1; font-weight: 500; }"
        );
    if (m_treeWorstChain) m_treeWorstChain->setStyleSheet(treeStyle);

    if (m_chainTabs) {
        if (isLight) {
            m_chainTabs->setStyleSheet(QStringLiteral(
                "QTabWidget::pane { border: 1px solid #dadce0; background: #ffffff; border-radius: 4px; }"
                "QTabBar::tab { background: #f1f3f4; color: #5f6368; padding: 6px 14px; border: 1px solid #dadce0; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
                "QTabBar::tab:selected { background: #ffffff; color: #1a73e8; font-weight: bold; border-bottom: 2px solid #1a73e8; }"
                "QTabBar::tab:hover:!selected { background: #e8eaed; }"
            ));
        } else {
            m_chainTabs->setStyleSheet(QStringLiteral(
                "QTabWidget::pane { border: 1px solid #313244; background: #1e1e2e; border-radius: 4px; }"
                "QTabBar::tab { background: #181825; color: #a6adc8; padding: 6px 14px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
                "QTabBar::tab:selected { background: #1e1e2e; color: #89b4fa; font-weight: bold; border-bottom: 2px solid #89b4fa; }"
                "QTabBar::tab:hover:!selected { background: #262738; }"
            ));
        }
    }

    if (m_lblTableTitle) {
        m_lblTableTitle->setStyleSheet(isLight ?
            QStringLiteral("font-weight: bold; color: #1a73e8;") :
            QStringLiteral("font-weight: bold; color: #89b4fa;"));
    }

    if (m_txtDisasm) {
        if (isLight) {
            m_txtDisasm->setStyleSheet(QStringLiteral("QTextEdit { background-color: #ffffff; color: #202124; border: 1px solid #dadce0; border-radius: 4px; padding: 4px; }"));
        } else {
            m_txtDisasm->setStyleSheet(QStringLiteral("QTextEdit { background-color: #11111b; color: #cdd6f4; border: 1px solid #313244; border-radius: 4px; padding: 4px; }"));
        }
    }

    if (m_lblDisasmHeader) {
        m_lblDisasmHeader->setStyleSheet(isLight ? QStringLiteral("font-weight: bold; color: #1a73e8;") : QStringLiteral("font-weight: bold; color: #89b4fa;"));
    }

    if (m_chainCanvasView) {
        m_chainCanvasView->applyTheme();
    }

    updateKpiCards();
    populateFunctionTable();
    renderWorstCaseCallGraph();
}

void StackAnalysisView::retranslateUi() {
    // 预留国际化多语言翻译
}

} // namespace Coverage
