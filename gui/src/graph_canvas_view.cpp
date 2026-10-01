#include "graph_canvas_view.h"
#include "theme_manager.h"
#include <QWheelEvent>
#include <QScrollBar>

namespace Coverage {

GraphCanvasView::GraphCanvasView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // 启用最高质量的抗锯齿与平滑缩放变换
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    applyTheme();
}

void GraphCanvasView::applyTheme() {
    ThemeType theme = ThemeManager::currentTheme();
    bool isLight = (theme == ThemeType::LightModern);
    QColor bgColor(ThemeManager::getGraphBgColor(theme));
    setBackgroundBrush(QBrush(bgColor));
    setStyleSheet(isLight ?
        QStringLiteral("QGraphicsView { border: 1px solid #dadce0; border-radius: 6px; }") :
        QStringLiteral("QGraphicsView { border: 1px solid #313244; border-radius: 6px; }"));
}

void GraphCanvasView::setGraph(const LayoutGraph& graph) {
    m_scene->clear();
    m_currentScale = 1.0;
    resetTransform();

    if (!graph.isValid || graph.nodes.isEmpty()) {
        bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
        auto* textItem = m_scene->addText(QStringLiteral("暂无可展示的代码拓扑图数据"));
        textItem->setDefaultTextColor(isLight ? QColor(QStringLiteral("#5f6368")) : QColor(QStringLiteral("#a6adc8")));
        textItem->setFont(QFont(QStringLiteral("Segoe UI"), 12));
        setSceneRect(QRectF(0, 0, 300, 100));
        return;
    }

    // 1. 先添加所有的边图元 (保证边在节点图元底层)
    for (const auto& edgeData : graph.edges) {
        auto* edgeItem = new EdgeGraphicsItem(edgeData);
        m_scene->addItem(edgeItem);
    }

    // 2. 添加所有的节点图元
    for (const auto& nodeData : graph.nodes) {
        auto* nodeItem = new NodeGraphicsItem(nodeData);
        m_scene->addItem(nodeItem);

        // 转发点击信号
        connect(nodeItem, &NodeGraphicsItem::nodeClicked,
                this, &GraphCanvasView::nodeSelected);
        connect(nodeItem, &NodeGraphicsItem::nodeDoubleClicked,
                this, &GraphCanvasView::nodeSelected);
    }

    // 扩展场景范围以预留边距
    QRectF bounds = m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40);
    setSceneRect(bounds);

    // 自动适配视口大小
    fitToWindow();
}

void GraphCanvasView::zoomIn() {
    qreal factor = 1.25;
    scale(factor, factor);
    m_currentScale *= factor;
}

void GraphCanvasView::zoomOut() {
    qreal factor = 0.8;
    scale(factor, factor);
    m_currentScale *= factor;
}

void GraphCanvasView::fitToWindow() {
    if (m_scene->items().isEmpty()) return;
    QRectF bounds = m_scene->itemsBoundingRect().adjusted(-20, -20, 20, 20);
    fitInView(bounds, Qt::KeepAspectRatio);
}

void GraphCanvasView::resetZoom() {
    resetTransform();
    m_currentScale = 1.0;
}

void GraphCanvasView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier || true) {
        // 平滑缩放
        const qreal factor = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
        scale(factor, factor);
        m_currentScale *= factor;
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void GraphCanvasView::showEvent(QShowEvent* event) {
    QGraphicsView::showEvent(event);
    fitToWindow();
}

void GraphCanvasView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
}

} // namespace Coverage
