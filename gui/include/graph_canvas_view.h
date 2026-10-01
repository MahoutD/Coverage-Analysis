#pragma once

#include "graph_items.h"
#include <QGraphicsView>
#include <QGraphicsScene>

namespace Coverage {

/**
 * @brief Qt 原生画布视图组件 (GraphCanvasView)
 * 承载 Graphviz 计算生成的 Qt 原生图元 (QGraphicsItem)，
 * 支持无级平滑缩放、手势拖拽平移、适应窗口、节点点击联动及主题底色自适应。
 */
class GraphCanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit GraphCanvasView(QWidget* parent = nullptr);
    ~GraphCanvasView() override = default;

    void setGraph(const LayoutGraph& graph);
    void applyTheme();

public slots:
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void resetZoom();

signals:
    void nodeSelected(const QString& name, int startLine, int endLine);

protected:
    void showEvent(QShowEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene* m_scene;
    qreal m_currentScale = 1.0;
};

} // namespace Coverage
