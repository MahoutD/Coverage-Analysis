#include "graph_items.h"
#include "theme_manager.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QLinearGradient>
#include <QCursor>
#include <QFontMetrics>
#include <QtMath>

namespace Coverage {

NodeGraphicsItem::NodeGraphicsItem(const LayoutNode& data, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_data(data)
    , m_rect(data.x, data.y, data.width, data.height)
{
    setFlags(ItemIsSelectable | ItemIsFocusable);
    setAcceptHoverEvents(true);
    setCursor(Qt::PointingHandCursor);

    m_titleFont = QFont(QStringLiteral("Segoe UI, Microsoft YaHei"), 10, QFont::Bold);
    m_subFont = QFont(QStringLiteral("Consolas, monospace"), 8);

    // 自适应图元尺寸度量计算，确保长函数名、行号与复杂度文本完全无裁切显示 (Requirement 5)
    QFontMetrics fmTitle(m_titleFont);
    QFontMetrics fmSub(m_subFont);

    QStringList lines = data.label.split(QStringLiteral("\n"));
    if (lines.isEmpty()) {
        lines.append(data.name);
    }

    int textW = 0;
    if (lines.size() >= 1) {
        textW = qMax(textW, fmTitle.horizontalAdvance(lines[0]));
    }
    if (lines.size() >= 2) {
        textW = qMax(textW, fmSub.horizontalAdvance(lines.mid(1).join(QStringLiteral(" "))));
    }

    qreal requiredW = textW + 28.0;
    qreal requiredH = (lines.size() > 1) ? (fmTitle.height() + fmSub.height() + 16.0) : (fmTitle.height() + 16.0);

    if (data.shape == QStringLiteral("diamond")) {
        requiredW *= 1.35;
        requiredH *= 1.35;
    }

    qreal finalW = qMax(static_cast<qreal>(data.width), requiredW);
    qreal finalH = qMax(static_cast<qreal>(data.height), requiredH);

    qreal cx = data.x + data.width / 2.0;
    qreal cy = data.y + data.height / 2.0;
    m_rect = QRectF(cx - finalW / 2.0, cy - finalH / 2.0, finalW, finalH);

    QString tip = QString("<b>函数/节点:</b> %1<br/>"
                          "<b>源码位置:</b> 第 %2 ~ %3 行<br/>"
                          "<b>圈复杂度:</b> %4<br/>"
                          "<i>💡 单击可直接跳转至源码对应代码行</i>")
                      .arg(data.name)
                      .arg(data.startLine)
                      .arg(data.endLine > 0 ? data.endLine : data.startLine)
                      .arg(data.complexity);
    setToolTip(tip);
}

QRectF NodeGraphicsItem::boundingRect() const {
    // 预留选中阴影与高亮外边框边距
    return m_rect.adjusted(-6, -6, 6, 6);
}

QPainterPath NodeGraphicsItem::shape() const {
    QPainterPath path;
    if (m_data.shape == QStringLiteral("diamond")) {
        QPolygonF poly;
        poly << QPointF(m_rect.center().x(), m_rect.top())
             << QPointF(m_rect.right(), m_rect.center().y())
             << QPointF(m_rect.center().x(), m_rect.bottom())
             << QPointF(m_rect.left(), m_rect.center().y());
        path.addPolygon(poly);
    } else if (m_data.shape == QStringLiteral("ellipse")) {
        path.addEllipse(m_rect);
    } else {
        path.addRoundedRect(m_rect, 6, 6);
    }
    return path;
}

void NodeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /* option */, QWidget* /* widget */) {
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    // 1. 选中或悬停时的外发光光晕
    if (isSelected()) {
        painter->setPen(QPen(QColor(QStringLiteral("#89b4fa")), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(m_rect.adjusted(-3, -3, 3, 3), 8, 8);
    } else if (m_isHovered) {
        painter->setPen(QPen(QColor(QStringLiteral("#ffd866")), 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(m_rect.adjusted(-2, -2, 2, 2), 7, 7);
    }

    // 2. 节点渐变底色与边框 (选中的路径执行到的节点边框着亮绿色)
    QColor baseColor(m_data.fillColor.isEmpty() ? QStringLiteral("#89b4fa") : m_data.fillColor);
    QLinearGradient grad(m_rect.topLeft(), m_rect.bottomLeft());
    grad.setColorAt(0.0, baseColor.lighter(115));
    grad.setColorAt(1.0, baseColor.darker(110));

    bool isPathNode = (m_data.color == QStringLiteral("#10b981") || m_data.color == QStringLiteral("#a6e3a1") || m_data.fillColor == QStringLiteral("#10b981"));
    QColor borderColor;
    qreal borderWidth = 1.2;
    if (isSelected()) {
        borderColor = QColor(QStringLiteral("#ffffff"));
        borderWidth = 2.5;
    } else if (isPathNode) {
        borderColor = QColor(QStringLiteral("#10b981"));
        borderWidth = 2.8;
    } else if (!m_data.color.isEmpty()) {
        borderColor = QColor(m_data.color);
        borderWidth = 1.5;
    } else {
        borderColor = baseColor.darker(140);
        borderWidth = 1.2;
    }
    painter->setPen(QPen(borderColor, borderWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(grad);

    if (m_data.shape == QStringLiteral("diamond")) {
        QPolygonF poly;
        poly << QPointF(m_rect.center().x(), m_rect.top())
             << QPointF(m_rect.right(), m_rect.center().y())
             << QPointF(m_rect.center().x(), m_rect.bottom())
             << QPointF(m_rect.left(), m_rect.center().y());
        painter->drawPolygon(poly);
    } else if (m_data.shape == QStringLiteral("ellipse")) {
        painter->drawEllipse(m_rect);
    } else {
        painter->drawRoundedRect(m_rect, 6, 6);
    }

    // 3. 绘制节点文字标签
    QStringList lines = m_data.label.split(QStringLiteral("\n"));
    if (lines.isEmpty()) {
        lines.append(m_data.name);
    }

    QColor textColor = (baseColor.lightness() > 140) ? QColor(QStringLiteral("#11111b")) : QColor(QStringLiteral("#ffffff"));

    if (lines.size() == 1) {
        painter->setFont(m_titleFont);
        painter->setPen(textColor);
        painter->drawText(m_rect, Qt::AlignCenter, lines[0]);
    } else {
        QRectF topRect = QRectF(m_rect.left(), m_rect.top() + 4, m_rect.width(), m_rect.height() * 0.55);
        QRectF botRect = QRectF(m_rect.left(), m_rect.top() + m_rect.height() * 0.50, m_rect.width(), m_rect.height() * 0.45);

        painter->setFont(m_titleFont);
        painter->setPen(textColor);
        painter->drawText(topRect, Qt::AlignCenter, lines[0]);

        painter->setFont(m_subFont);
        QColor subColor = textColor;
        subColor.setAlpha(200);
        painter->setPen(subColor);
        painter->drawText(botRect, Qt::AlignCenter, lines.mid(1).join(QStringLiteral(" ")));
    }
}

void NodeGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent* /* event */) {
    m_isHovered = true;
    update();
}

void NodeGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* /* event */) {
    m_isHovered = false;
    update();
}

void NodeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsObject::mousePressEvent(event);
    emit nodeClicked(m_data.name, m_data.startLine, m_data.endLine);
}

void NodeGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsObject::mouseDoubleClickEvent(event);
    emit nodeDoubleClicked(m_data.name, m_data.startLine, m_data.endLine);
}

// ----------------------------------------------------------------------------------
// EdgeGraphicsItem 实现
// ----------------------------------------------------------------------------------

EdgeGraphicsItem::EdgeGraphicsItem(const LayoutEdge& data, QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
    , m_data(data)
    , m_label(data.label)
    , m_labelPos(data.labelPos)
{
    setZValue(-1); // 放置于节点下方

    QPainterPath path;
    const auto& pts = data.splinePoints;

    if (pts.size() >= 4) {
        path.moveTo(pts[0]);
        for (int i = 1; i + 2 < pts.size(); i += 3) {
            path.cubicTo(pts[i], pts[i + 1], pts[i + 2]);
        }

        // 计算末端箭头多边形
        QPointF pEnd = pts.last();
        QPointF pPre = pts[pts.size() - 2];
        qreal dx = pEnd.x() - pPre.x();
        qreal dy = pEnd.y() - pPre.y();
        qreal angle = qAtan2(dy, dx);

        qreal arrowSize = 9.0;
        QPointF p1 = pEnd;
        QPointF p2 = pEnd - QPointF(qCos(angle - M_PI / 6.0) * arrowSize, qSin(angle - M_PI / 6.0) * arrowSize);
        QPointF p3 = pEnd - QPointF(qCos(angle + M_PI / 6.0) * arrowSize, qSin(angle + M_PI / 6.0) * arrowSize);

        m_arrowHead << p1 << p2 << p3;
    }

    setPath(path);

    QColor edgeColor(data.color.isEmpty() ? QStringLiteral("#89b4fa") : data.color);
    bool isPathEdge = (data.color == QStringLiteral("#10b981") || data.color == QStringLiteral("#a6e3a1") || data.color == QStringLiteral("#22c55e"));
    setPen(QPen(edgeColor, isPathEdge ? 2.8 : 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setBrush(Qt::NoBrush);
}

void EdgeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    QGraphicsPathItem::paint(painter, option, widget);

    // 绘制箭头
    if (!m_arrowHead.isEmpty()) {
        QColor edgeColor(m_data.color.isEmpty() ? QStringLiteral("#89b4fa") : m_data.color);
        painter->setPen(Qt::NoPen);
        painter->setBrush(edgeColor);
        painter->drawPolygon(m_arrowHead);
    }

    // 绘制边标签 (如分支判定结果 True / False)
    if (!m_label.isEmpty() && !m_labelPos.isNull()) {
        QFont font(QStringLiteral("Segoe UI"), 8, QFont::Bold);
        painter->setFont(font);

        QFontMetrics fm(font);
        int tw = fm.horizontalAdvance(m_label);
        int th = fm.height();
        QRectF bgRect(m_labelPos.x() - tw / 2.0 - 4, m_labelPos.y() - th / 2.0 - 2, tw + 8, th + 4);

        bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
        painter->setPen(isLight ? QColor(QStringLiteral("#dadce0")) : QColor(QStringLiteral("#313244")));
        painter->setBrush(isLight ? QColor(QStringLiteral("#ffffff")) : QColor(QStringLiteral("#181825")));
        painter->drawRoundedRect(bgRect, 4, 4);

        painter->setPen(isLight ? QColor(QStringLiteral("#202124")) : QColor(QStringLiteral("#cdd6f4")));
        painter->drawText(bgRect, Qt::AlignCenter, m_label);
    }
}

} // namespace Coverage
