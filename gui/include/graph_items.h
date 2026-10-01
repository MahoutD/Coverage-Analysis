#pragma once

#include "coverage/graph_layout.h"
#include <QGraphicsObject>
#include <QGraphicsPathItem>
#include <QPen>
#include <QBrush>
#include <QFont>

namespace Coverage {

/**
 * @brief Qt 原生图元：函数/代码块节点 (NodeGraphicsItem)
 * 具备抗锯齿圆角矩形/菱形绘制、圈复杂度自适应色彩映射、悬停高亮、选中光晕与鼠标点击信号。
 */
class NodeGraphicsItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit NodeGraphicsItem(const LayoutNode& data, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    const LayoutNode& data() const { return m_data; }
    void applyThemeColors();

signals:
    void nodeClicked(const QString& name, int startLine, int endLine);
    void nodeDoubleClicked(const QString& name, int startLine, int endLine);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    LayoutNode m_data;
    QRectF m_rect;
    bool m_isHovered = false;
    QFont m_titleFont;
    QFont m_subFont;
};

/**
 * @brief Qt 原生图元：调用/控制流关系边 (EdgeGraphicsItem)
 * 采用三次贝塞尔样条曲线平滑连接，带有末端定向箭头与条件分支标签。
 */
class EdgeGraphicsItem : public QGraphicsPathItem {
public:
    explicit EdgeGraphicsItem(const LayoutEdge& data, QGraphicsItem* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    LayoutEdge m_data;
    QPolygonF m_arrowHead;
    QPointF m_labelPos;
    QString m_label;
};

} // namespace Coverage
