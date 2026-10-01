#pragma once

#include "coverage/models.h"
#include <QString>
#include <QVector>
#include <QPointF>

namespace Coverage {

/**
 * @brief 布局计算后的节点数据结构
 */
struct LayoutNode {
    QString id;          ///< 节点唯一标识
    QString name;        ///< 函数名称或语句标识
    QString label;       ///< 节点显示多行文本
    qreal x = 0;         ///< 节点左上角 X (像素)
    qreal y = 0;         ///< 节点左上角 Y (像素)
    qreal width = 0;     ///< 节点宽度 (像素)
    qreal height = 0;    ///< 节点高度 (像素)
    QString shape;       ///< 形状 (box, diamond, ellipse)
    QString color;       ///< 边框颜色 (如 #89b4fa)
    QString fillColor;   ///< 填充颜色 (如 #a6e3a1)
    int startLine = 0;   ///< 对应源代码起始行号 (供跳转)
    int endLine = 0;     ///< 对应源代码结束行号
    int complexity = 1;  ///< McCabe 圈复杂度
    QString type;        ///< 节点语义类别
};

/**
 * @brief 布局计算后的边数据结构 (包含三次贝塞尔曲线控制点)
 */
struct LayoutEdge {
    QString sourceId;               ///< 起点节点 ID
    QString targetId;               ///< 终点节点 ID
    QVector<QPointF> splinePoints;  ///< 三次贝塞尔曲线控制点序列 (p0, p1, p2, p3...)
    QString label;                  ///< 边文本 (如 True / False)
    QPointF labelPos;               ///< 边文本中心坐标
    QString color;                  ///< 边线条颜色 (如 #89b4fa)
};

/**
 * @brief 完整的图布局数据
 */
struct LayoutGraph {
    qreal width = 0;
    qreal height = 0;
    QVector<LayoutNode> nodes;
    QVector<LayoutEdge> edges;
    bool isValid = false;
    QString errorMessage;
};

/**
 * @brief Graphviz 内存布局引擎
 * 直接调用 Graphviz C 库 (gvc / cgraph) 或本地引擎计算无失真的点线拓扑坐标，
 * 供 Qt 原生 QGraphicsScene/QGraphicsItem 图元无失真加载与交互。
 */
class GraphLayoutEngine {
public:
    static LayoutGraph computeLayout(const QString& dotContent,
                                     const StaticAnalysisReport* report = nullptr,
                                     QString* outError = nullptr);

    static LayoutGraph parsePlainOutput(const QString& plainText,
                                        const StaticAnalysisReport* report);
};

} // namespace Coverage
