#include "graph_view.h"
#include "coverage/graph_generator.h"
#include "coverage/graph_layout.h"
#include "coverage/logger.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QImage>

namespace Coverage {

GraphView::GraphView(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void GraphView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // 1. 顶部控制工具栏
    auto* topLayout = new QHBoxLayout();

    // 模式选择
    topLayout->addWidget(new QLabel(QStringLiteral("视图模式:"), this));
    m_cmbGraphMode = new QComboBox(this);
    m_cmbGraphMode->addItem(QStringLiteral("全局函数调用拓扑图 (Call Graph)"), 0);
    m_cmbGraphMode->addItem(QStringLiteral("函数内部控制流图 (Control Flow Graph - CFG)"), 1);
    topLayout->addWidget(m_cmbGraphMode);

    // 函数选择
    topLayout->addWidget(new QLabel(QStringLiteral("目标函数:"), this));
    m_cmbFunctionSelect = new QComboBox(this);
    m_cmbFunctionSelect->setEnabled(false);
    topLayout->addWidget(m_cmbFunctionSelect);

    // 刷新与控制按钮
    m_btnRefresh = new QPushButton(QStringLiteral("🔄 生成/刷新图表"), this);
    m_btnRefresh->setObjectName(QStringLiteral("btnPrimary"));
    topLayout->addWidget(m_btnRefresh);

    m_btnZoomIn = new QPushButton(QStringLiteral("🔍 放大"), this);
    m_btnZoomOut = new QPushButton(QStringLiteral("🔍 缩小"), this);
    m_btnFit = new QPushButton(QStringLiteral("📐 适应窗口"), this);
    m_btnReset = new QPushButton(QStringLiteral("100% 重置"), this);
    topLayout->addWidget(m_btnZoomIn);
    topLayout->addWidget(m_btnZoomOut);
    topLayout->addWidget(m_btnFit);
    topLayout->addWidget(m_btnReset);

    // 导出按钮
    m_btnExportDot = new QPushButton(QStringLiteral("💾 导出 DOT"), this);
    m_btnExportImage = new QPushButton(QStringLiteral("🖼️ 导出高清图片"), this);
    topLayout->addWidget(m_btnExportDot);
    topLayout->addWidget(m_btnExportImage);

    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // 2. 状态提示条 (显示当前节点数量、跳转提示等)
    m_lblStatus = new QLabel(QStringLiteral("💡 提示：所有节点与线条均使用 Qt 原生矢量画布抗锯齿绘制，单击或双击任意函数节点可直接在代码编辑器中定位跳转至源码行。"), this);
    m_lblStatus->setStyleSheet(QStringLiteral("color: #a6adc8; padding: 4px 8px; background: #181825; border-radius: 4px; font-size: 11px;"));
    mainLayout->addWidget(m_lblStatus);

    // 3. 核心展示区域：选项卡 (原生矢量画布 / DOT 源码)
    m_tabs = new QTabWidget(this);

    // Tab 1: 原生画布视口
    m_canvasView = new GraphCanvasView(this);
    m_tabs->addTab(m_canvasView, QStringLiteral("🎨 原生矢量交互图表 (可交互/可选中)"));

    // Tab 2: DOT 源码视图 (支持在线微调)
    auto* dotTab = new QWidget(this);
    auto* dotLayout = new QVBoxLayout(dotTab);
    dotLayout->setContentsMargins(4, 4, 4, 4);

    m_txtDotSource = new QTextEdit(this);
    m_txtDotSource->setStyleSheet(ThemeManager::getEditorStyleSheet());
    dotLayout->addWidget(m_txtDotSource);

    auto* btnApplyDot = new QPushButton(QStringLiteral("🚀 重新解析并渲染上方 DOT 代码至原生图元画布"), this);
    btnApplyDot->setStyleSheet(QStringLiteral("font-weight: bold; padding: 6px;"));
    dotLayout->addWidget(btnApplyDot);
    m_tabs->addTab(dotTab, QStringLiteral("📝 Graphviz DOT 脚本源码"));

    mainLayout->addWidget(m_tabs, 1);

    // 信号连接
    connect(m_cmbGraphMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GraphView::onModeChanged);
    connect(m_cmbFunctionSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GraphView::onFunctionChanged);
    connect(m_btnRefresh, &QPushButton::clicked,
            this, &GraphView::onGenerateGraph);

    connect(m_btnZoomIn, &QPushButton::clicked, m_canvasView, &GraphCanvasView::zoomIn);
    connect(m_btnZoomOut, &QPushButton::clicked, m_canvasView, &GraphCanvasView::zoomOut);
    connect(m_btnFit, &QPushButton::clicked, m_canvasView, &GraphCanvasView::fitToWindow);
    connect(m_btnReset, &QPushButton::clicked, m_canvasView, &GraphCanvasView::resetZoom);

    connect(m_btnExportDot, &QPushButton::clicked, this, &GraphView::onExportDot);
    connect(m_btnExportImage, &QPushButton::clicked, this, &GraphView::onExportImage);

    connect(btnApplyDot, &QPushButton::clicked, this, [this]() {
        m_currentDot = m_txtDotSource->toPlainText();
        renderCurrentDot();
    });

    // 原生节点单击/双击跳转连接
    connect(m_canvasView, &GraphCanvasView::nodeSelected,
            this, &GraphView::nodeSelected);

    applyTheme(ThemeManager::currentTheme());
}

void GraphView::applyTheme(ThemeType type) {
    m_currentTheme = type;
    bool isLight = (type == ThemeType::LightModern);

    if (m_lblStatus) {
        if (isLight) {
            m_lblStatus->setStyleSheet(QStringLiteral("color: #5f6368; padding: 4px 8px; background: #ffffff; border: 1px solid #dadce0; border-radius: 4px; font-size: 11px;"));
        } else {
            m_lblStatus->setStyleSheet(QStringLiteral("color: #a6adc8; padding: 4px 8px; background: #181825; border: 1px solid #313244; border-radius: 4px; font-size: 11px;"));
        }
    }

    if (m_txtDotSource) {
        m_txtDotSource->setStyleSheet(ThemeManager::getEditorStyleSheet(type));
    }

    if (m_canvasView) {
        m_canvasView->applyTheme();
    }

    if (m_tabs) {
        if (isLight) {
            m_tabs->setStyleSheet(QStringLiteral(
                "QTabWidget::pane { border: 1px solid #dadce0; background: #ffffff; border-radius: 4px; }"
                "QTabBar::tab { background: #f1f3f4; color: #5f6368; padding: 6px 14px; border: 1px solid #dadce0; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
                "QTabBar::tab:selected { background: #ffffff; color: #1a73e8; font-weight: bold; border-bottom: 2px solid #1a73e8; }"
                "QTabBar::tab:hover:!selected { background: #e8eaed; }"
            ));
        } else {
            m_tabs->setStyleSheet(QStringLiteral(
                "QTabWidget::pane { border: 1px solid #313244; background: #1e1e2e; border-radius: 4px; }"
                "QTabBar::tab { background: #181825; color: #a6adc8; padding: 6px 14px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
                "QTabBar::tab:selected { background: #1e1e2e; color: #89b4fa; font-weight: bold; border-bottom: 2px solid #89b4fa; }"
                "QTabBar::tab:hover:!selected { background: #262738; }"
            ));
        }
    }
}

void GraphView::updateData(const StaticAnalysisReport& report, const QString& sourceCode) {
    m_report = report;
    m_sourceCode = sourceCode;

    m_cmbFunctionSelect->blockSignals(true);
    m_cmbFunctionSelect->clear();
    for (const auto& func : report.functions) {
        m_cmbFunctionSelect->addItem(QString("%1() [行 %2, 复杂度: %3]")
                                         .arg(func.name)
                                         .arg(func.startLine)
                                         .arg(func.cyclomaticComplexity),
                                     func.name);
    }
    m_cmbFunctionSelect->blockSignals(false);

    onGenerateGraph();
}

void GraphView::clear() {
    m_currentDot.clear();
    m_txtDotSource->clear();
    m_canvasView->setGraph(LayoutGraph());
}

void GraphView::onModeChanged(int index) {
    bool isCfg = (index == 1);
    m_cmbFunctionSelect->setEnabled(isCfg && m_cmbFunctionSelect->count() > 0);
    onGenerateGraph();
}

void GraphView::onFunctionChanged(int /* index */) {
    if (m_cmbGraphMode->currentIndex() == 1) {
        onGenerateGraph();
    }
}

void GraphView::onGenerateGraph() {
    if (m_cmbGraphMode->currentIndex() == 0) {
        // 模式 1: 全局函数调用图
        m_currentDot = GraphGenerator::generateCallGraphDot(m_report, m_sourceCode);
    } else {
        // 模式 2: 单个函数内部控制流图 (CFG)
        QString funcName = m_cmbFunctionSelect->currentData().toString();
        if (funcName.isEmpty() && m_cmbFunctionSelect->count() > 0) {
            funcName = m_cmbFunctionSelect->itemData(0).toString();
        }
        FunctionMetrics targetFunc;
        for (const auto& f : m_report.functions) {
            if (f.name == funcName) {
                targetFunc = f;
                break;
            }
        }
        if (targetFunc.name.isEmpty()) {
            targetFunc.name = funcName;
            targetFunc.startLine = 1;
            targetFunc.endLine = 50;
        }
        m_currentDot = GraphGenerator::generateFunctionCfgDot(targetFunc, m_sourceCode);
    }

    m_txtDotSource->setPlainText(m_currentDot);
    renderCurrentDot();
}

void GraphView::renderCurrentDot() {
    if (m_currentDot.isEmpty()) {
        m_canvasView->setGraph(LayoutGraph());
        return;
    }

    QString err;
    LayoutGraph layout = GraphLayoutEngine::computeLayout(m_currentDot, &m_report, &err);

    if (layout.isValid) {
        m_canvasView->setGraph(layout);
        m_lblStatus->setText(QString("✅ 图元生成成功！包含 %1 个节点和 %2 条拓扑关系边。单击节点即可高亮跳转代码。")
                                 .arg(layout.nodes.size())
                                 .arg(layout.edges.size()));
    } else {
        m_lblStatus->setText(QStringLiteral("❌ 图元解析失败: ") + (err.isEmpty() ? QStringLiteral("未知错误") : err));
        Logger::instance().log(LogLevel::Error, QStringLiteral("GraphView"),
                               QStringLiteral("生成图元失败: ") + err);
    }
}

void GraphView::onExportDot() {
    if (m_currentDot.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("当前图表为空，无需导出。"));
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出 Graphviz DOT 脚本"),
                                                QStringLiteral("graph.dot"), QStringLiteral("DOT Files (*.dot);;All Files (*.*)"));
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_currentDot;
            file.close();
            QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("DOT 脚本已保存至:\n") + path);
        }
    }
}

void GraphView::onExportImage() {
    if (!m_canvasView->scene() || m_canvasView->scene()->items().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("画布为空，无可导出图元。"));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出高清拓扑图"),
                                                QStringLiteral("topology_graph.png"),
                                                QStringLiteral("PNG Images (*.png);;JPEG Images (*.jpg)"));
    if (!path.isEmpty()) {
        QRectF sceneRect = m_canvasView->scene()->itemsBoundingRect().adjusted(-20, -20, 20, 20);
        QImage image(sceneRect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
        image.fill(ThemeManager::getGraphBgColor(ThemeManager::currentTheme()));

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        m_canvasView->scene()->render(&painter, QRectF(), sceneRect);
        painter.end();

        if (image.save(path)) {
            QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("高清图表已保存至:\n") + path);
        }
    }
}

} // namespace Coverage
