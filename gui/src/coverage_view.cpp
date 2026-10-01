#include "coverage_view.h"
#include "syntax_highlighter.h"
#include "localization_manager.h"
#include "coverage/project.h"
#include "coverage/instrumenter.h"
#include "coverage/coverage_engine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QToolTip>
#include <QTextBlock>
#include <QScrollBar>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <cmath>

namespace Coverage {

// =============================================================================
// 1. CoverageBarChartWidget 覆盖率矢量柱状图
// =============================================================================

CoverageBarChartWidget::CoverageBarChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(240);
    setMouseTracking(true);
}

void CoverageBarChartWidget::setData(const QVector<FunctionCoverageInfo>& data) {
    m_data = data;
    m_hoveredIndex = -1;
    m_barRects.clear();
    m_linePoints.clear();
    m_pieSlices.clear();
    update();
}

void CoverageBarChartWidget::setSelectedFunction(const QString& funcName) {
    m_selectedFunction = funcName;
    update();
}

void CoverageBarChartWidget::setChartType(CoverageChartType type) {
    if (m_chartType != type) {
        m_chartType = type;
        m_hoveredIndex = -1;
        update();
        emit chartTypeChanged(type);
    }
}

void CoverageBarChartWidget::applyTheme(ThemeType theme) {
    m_theme = theme;
    update();
}

void CoverageBarChartWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.setTitle(QStringLiteral("图表展示样式"));

    auto* actBar = menu.addAction(QIcon(QStringLiteral(":/icons/static_analysis.svg")), QStringLiteral("📊 柱状分布图 (Bar Chart)"));
    actBar->setCheckable(true);
    actBar->setChecked(m_chartType == CoverageChartType::BarChart);

    auto* actLine = menu.addAction(QIcon(QStringLiteral(":/icons/path_explorer.svg")), QStringLiteral("📈 趋势折线图 (Line Chart)"));
    actLine->setCheckable(true);
    actLine->setChecked(m_chartType == CoverageChartType::LineChart);

    auto* actPie = menu.addAction(QIcon(QStringLiteral(":/icons/run_sim.svg")), QStringLiteral("🥧 覆盖率饼图 (Pie Chart)"));
    actPie->setCheckable(true);
    actPie->setChecked(m_chartType == CoverageChartType::PieChart);

    connect(actBar, &QAction::triggered, this, [this]() { setChartType(CoverageChartType::BarChart); });
    connect(actLine, &QAction::triggered, this, [this]() { setChartType(CoverageChartType::LineChart); });
    connect(actPie, &QAction::triggered, this, [this]() { setChartType(CoverageChartType::PieChart); });

    menu.exec(event->globalPos());
}

void CoverageBarChartWidget::paintEvent(QPaintEvent* /* event */) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    bool isLight = (m_theme == ThemeType::LightModern);
    QColor bgColor = isLight ? QColor(QStringLiteral("#ffffff")) : QColor(QStringLiteral("#1e1e2e"));
    painter.fillRect(rect(), bgColor);

    if (m_data.isEmpty()) {
        QColor axisColor = isLight ? QColor(QStringLiteral("#5f6368")) : QColor(QStringLiteral("#6c7086"));
        painter.setPen(axisColor);
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 11));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("暂无测试覆盖统计数据 (请先在项目内执行测试激励)"));
        return;
    }

    if (m_chartType == CoverageChartType::BarChart) {
        drawBarChart(painter);
    } else if (m_chartType == CoverageChartType::LineChart) {
        drawLineChart(painter);
    } else if (m_chartType == CoverageChartType::PieChart) {
        drawPieChart(painter);
    }
}

void CoverageBarChartWidget::drawBarChart(QPainter& painter) {
    bool isLight = (m_theme == ThemeType::LightModern);
    QColor axisColor = isLight ? QColor(QStringLiteral("#5f6368")) : QColor(QStringLiteral("#6c7086"));
    QColor gridColor = isLight ? QColor(QStringLiteral("#f1f3f4")) : QColor(QStringLiteral("#313244"));
    QColor textColor = isLight ? QColor(QStringLiteral("#202124")) : QColor(QStringLiteral("#cdd6f4"));
    QColor barColor = isLight ? QColor(QStringLiteral("#0f4c64")) : QColor(QStringLiteral("#0284c7"));
    QColor barCoveredColor = isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#10b981"));
    QColor barHoverColor = isLight ? QColor(QStringLiteral("#0284c7")) : QColor(QStringLiteral("#38bdf8"));
    QColor barSelectedColor = isLight ? QColor(QStringLiteral("#eab308")) : QColor(QStringLiteral("#f59e0b"));

    // 1. 顶部标题与图例
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 11, QFont::Bold));
    painter.setPen(textColor);
    painter.drawText(QRect(0, 8, width(), 24), Qt::AlignCenter, QStringLiteral("语句覆盖度量 - 柱状图"));

    // 右上角图例
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    int legX = width() - 200;
    painter.fillRect(QRect(legX, 12, 12, 12), barColor);
    painter.setPen(textColor);
    painter.drawText(QRect(legX + 16, 10, 65, 16), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("总语句数"));

    painter.fillRect(QRect(legX + 85, 12, 12, 12), barCoveredColor);
    painter.setPen(textColor);
    painter.drawText(QRect(legX + 101, 10, 85, 16), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("已覆盖语句"));

    int leftMargin = 55;
    int rightMargin = 30;
    int topMargin = 36;
    int bottomMargin = 60;

    int plotWidth = width() - leftMargin - rightMargin;
    int plotHeight = height() - topMargin - bottomMargin;
    if (plotWidth <= 20 || plotHeight <= 20) return;

    int maxVal = 10;
    for (const auto& item : m_data) {
        if (item.statementCount > maxVal) maxVal = item.statementCount;
    }
    int step = 25;
    if (maxVal > 150) step = 50;
    if (maxVal > 300) step = 100;
    int yMax = ((maxVal + step - 1) / step) * step;
    if (yMax < 50) yMax = 50;

    // 2. 水平网格线与 Y 轴刻度
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    for (int v = 0; v <= yMax; v += step) {
        int y = topMargin + plotHeight - (v * plotHeight / yMax);
        painter.setPen(QPen(gridColor, 1, Qt::SolidLine));
        painter.drawLine(leftMargin, y, leftMargin + plotWidth, y);

        painter.setPen(axisColor);
        painter.drawText(QRect(0, y - 8, leftMargin - 6, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(v));
    }

    // 3. 坐标轴及箭头
    painter.setPen(QPen(axisColor, 1.5));
    painter.drawLine(leftMargin, topMargin + plotHeight, leftMargin, topMargin - 10);
    QPolygonF yArrow;
    yArrow << QPointF(leftMargin, topMargin - 15)
           << QPointF(leftMargin - 4, topMargin - 8)
           << QPointF(leftMargin + 4, topMargin - 8);
    painter.setBrush(axisColor);
    painter.drawPolygon(yArrow);
    painter.drawText(leftMargin - 16, topMargin - 12, QStringLiteral("y"));

    painter.drawLine(leftMargin, topMargin + plotHeight, leftMargin + plotWidth + 10, topMargin + plotHeight);
    QPolygonF xArrow;
    xArrow << QPointF(leftMargin + plotWidth + 15, topMargin + plotHeight)
           << QPointF(leftMargin + plotWidth + 8, topMargin + plotHeight - 4)
           << QPointF(leftMargin + plotWidth + 8, topMargin + plotHeight + 4);
    painter.drawPolygon(xArrow);
    painter.drawText(leftMargin + plotWidth + 4, topMargin + plotHeight + 16, QStringLiteral("x"));

    // 4. 绘制各函数柱子
    int count = m_data.size();
    double slotWidth = static_cast<double>(plotWidth) / qMax(1, count);
    double barWidth = qBound(8.0, slotWidth * 0.65, 36.0);

    m_barRects.resize(count);

    for (int i = 0; i < count; ++i) {
        const auto& item = m_data[i];
        double cx = leftMargin + (i + 0.5) * slotWidth;
        double bx = cx - barWidth / 2.0;

        int totalH = qRound(static_cast<double>(item.statementCount) * plotHeight / yMax);
        totalH = qMax(2, totalH);
        double by = topMargin + plotHeight - totalH;

        QRectF barRect(bx, by, barWidth, totalH);
        m_barRects[i] = barRect;

        // 颜色选择
        QColor curTotalColor = barColor;
        if (i == m_hoveredIndex) {
            curTotalColor = barHoverColor;
        }

        // 绘制总语句柱
        painter.setPen(Qt::NoPen);
        painter.setBrush(curTotalColor);
        painter.drawRoundedRect(barRect, 2, 2);

        // 绘制覆盖语句柱 (覆在上方)
        if (item.coveredStatements > 0) {
            int covH = qRound(static_cast<double>(item.coveredStatements) * plotHeight / yMax);
            covH = qBound(2, covH, totalH);
            double covY = topMargin + plotHeight - covH;
            QRectF covRect(bx, covY, barWidth, covH);
            painter.setBrush(barCoveredColor);
            painter.drawRoundedRect(covRect, 2, 2);
        }

        // 选中高亮边框
        if (item.functionName == m_selectedFunction) {
            painter.setPen(QPen(barSelectedColor, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(barRect.adjusted(-2, -2, 2, 0), 3, 3);
        }

        // X 轴下方函数标签 (-45 度旋转)
        painter.save();
        painter.translate(cx, topMargin + plotHeight + 6);
        painter.rotate(-45);
        painter.setPen(textColor);
        painter.setFont(QFont(QStringLiteral("Consolas"), 8));

        QString label = item.functionName;
        if (label.length() > 18) {
            label = label.left(16) + QStringLiteral("..");
        }
        painter.drawText(QRect(-60, 0, 80, 16), Qt::AlignRight | Qt::AlignVCenter, label);
        painter.restore();
    }
}

void CoverageBarChartWidget::drawLineChart(QPainter& painter) {
    bool isLight = (m_theme == ThemeType::LightModern);
    QColor axisColor = isLight ? QColor(QStringLiteral("#5f6368")) : QColor(QStringLiteral("#6c7086"));
    QColor gridColor = isLight ? QColor(QStringLiteral("#f1f3f4")) : QColor(QStringLiteral("#313244"));
    QColor textColor = isLight ? QColor(QStringLiteral("#202124")) : QColor(QStringLiteral("#cdd6f4"));
    QColor lineTotalColor = isLight ? QColor(QStringLiteral("#0284c7")) : QColor(QStringLiteral("#38bdf8"));
    QColor lineCoveredColor = isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#10b981"));

    // 1. 标题与图例
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 11, QFont::Bold));
    painter.setPen(textColor);
    painter.drawText(QRect(0, 8, width(), 24), Qt::AlignCenter, QStringLiteral("语句覆盖趋势 - 折线图"));

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    int legX = width() - 200;
    painter.setPen(QPen(lineTotalColor, 2));
    painter.drawLine(legX, 18, legX + 16, 18);
    painter.fillRect(QRect(legX + 6, 15, 6, 6), lineTotalColor);
    painter.setPen(textColor);
    painter.drawText(QRect(legX + 22, 10, 65, 16), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("总语句数"));

    painter.setPen(QPen(lineCoveredColor, 2));
    painter.drawLine(legX + 90, 18, legX + 106, 18);
    painter.fillRect(QRect(legX + 96, 15, 6, 6), lineCoveredColor);
    painter.setPen(textColor);
    painter.drawText(QRect(legX + 112, 10, 85, 16), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("已覆盖语句"));

    int leftMargin = 55;
    int rightMargin = 30;
    int topMargin = 36;
    int bottomMargin = 60;

    int plotWidth = width() - leftMargin - rightMargin;
    int plotHeight = height() - topMargin - bottomMargin;
    if (plotWidth <= 20 || plotHeight <= 20) return;

    int maxVal = 10;
    for (const auto& item : m_data) {
        if (item.statementCount > maxVal) maxVal = item.statementCount;
    }
    int step = 25;
    if (maxVal > 150) step = 50;
    if (maxVal > 300) step = 100;
    int yMax = ((maxVal + step - 1) / step) * step;
    if (yMax < 50) yMax = 50;

    // 水平网格线与 Y 轴刻度
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    for (int v = 0; v <= yMax; v += step) {
        int y = topMargin + plotHeight - (v * plotHeight / yMax);
        painter.setPen(QPen(gridColor, 1, Qt::SolidLine));
        painter.drawLine(leftMargin, y, leftMargin + plotWidth, y);

        painter.setPen(axisColor);
        painter.drawText(QRect(0, y - 8, leftMargin - 6, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(v));
    }

    // 坐标轴
    painter.setPen(QPen(axisColor, 1.5));
    painter.drawLine(leftMargin, topMargin + plotHeight, leftMargin, topMargin - 10);
    painter.drawLine(leftMargin, topMargin + plotHeight, leftMargin + plotWidth + 10, topMargin + plotHeight);

    int count = m_data.size();
    double slotWidth = static_cast<double>(plotWidth) / qMax(1, count);

    m_linePoints.resize(count);
    m_barRects.resize(count);

    QPolygonF polyTotal;
    QPolygonF polyCovered;

    for (int i = 0; i < count; ++i) {
        const auto& item = m_data[i];
        double cx = leftMargin + (i + 0.5) * slotWidth;
        double cyTotal = topMargin + plotHeight - (static_cast<double>(item.statementCount) * plotHeight / yMax);
        double cyCovered = topMargin + plotHeight - (static_cast<double>(item.coveredStatements) * plotHeight / yMax);

        polyTotal << QPointF(cx, cyTotal);
        polyCovered << QPointF(cx, cyCovered);
        m_linePoints[i] = QPointF(cx, cyCovered);
        m_barRects[i] = QRectF(cx - slotWidth / 2.0, topMargin, slotWidth, plotHeight);

        // X 轴下方标签
        painter.save();
        painter.translate(cx, topMargin + plotHeight + 6);
        painter.rotate(-45);
        painter.setPen(textColor);
        painter.setFont(QFont(QStringLiteral("Consolas"), 8));
        QString label = item.functionName;
        if (label.length() > 18) label = label.left(16) + QStringLiteral("..");
        painter.drawText(QRect(-60, 0, 80, 16), Qt::AlignRight | Qt::AlignVCenter, label);
        painter.restore();
    }

    // 绘制覆盖区域渐变填充
    if (!polyCovered.isEmpty()) {
        QPolygonF fillCovered = polyCovered;
        fillCovered << QPointF(polyCovered.last().x(), topMargin + plotHeight);
        fillCovered << QPointF(polyCovered.first().x(), topMargin + plotHeight);

        QColor covFill = lineCoveredColor;
        covFill.setAlpha(isLight ? 45 : 35);
        painter.setPen(Qt::NoPen);
        painter.setBrush(covFill);
        painter.drawPolygon(fillCovered);
    }

    // 绘制折线
    painter.setPen(QPen(lineTotalColor, 2.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(polyTotal);

    painter.setPen(QPen(lineCoveredColor, 2.5));
    painter.drawPolyline(polyCovered);

    // 绘制各数据点
    for (int i = 0; i < count; ++i) {
        QPointF ptTotal = polyTotal[i];
        QPointF ptCovered = polyCovered[i];

        // 总量数据点
        painter.setPen(QPen(lineTotalColor, 2));
        painter.setBrush(isLight ? Qt::white : QColor(QStringLiteral("#1e1e2e")));
        painter.drawEllipse(ptTotal, 4.0, 4.0);

        // 覆盖数据点
        bool isHovered = (i == m_hoveredIndex);
        bool isSelected = (m_data[i].functionName == m_selectedFunction);

        painter.setPen(QPen(lineCoveredColor, 2));
        painter.setBrush(isSelected ? QColor(QStringLiteral("#f59e0b")) : (isHovered ? lineCoveredColor : (isLight ? Qt::white : QColor(QStringLiteral("#1e1e2e")))));
        double r = (isHovered || isSelected) ? 6.0 : 4.0;
        painter.drawEllipse(ptCovered, r, r);
    }
}

void CoverageBarChartWidget::drawPieChart(QPainter& painter) {
    bool isLight = (m_theme == ThemeType::LightModern);
    QColor textColor = isLight ? QColor(QStringLiteral("#202124")) : QColor(QStringLiteral("#cdd6f4"));
    QColor subColor = isLight ? QColor(QStringLiteral("#5f6368")) : QColor(QStringLiteral("#a6adc8"));
    QColor bgColor = isLight ? QColor(QStringLiteral("#ffffff")) : QColor(QStringLiteral("#1e1e2e"));

    // 标题
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 11, QFont::Bold));
    painter.setPen(textColor);
    painter.drawText(QRect(0, 8, width(), 24), Qt::AlignCenter, QStringLiteral("测试覆盖综合分布 - 饼图"));

    int totalStmts = 0;
    int totalCovered = 0;
    for (const auto& item : m_data) {
        totalStmts += item.statementCount;
        totalCovered += item.coveredStatements;
    }
    int totalUncovered = qMax(0, totalStmts - totalCovered);
    double covPercent = (totalStmts > 0) ? (100.0 * totalCovered / totalStmts) : 100.0;

    int leftMargin = 20;
    int rightMargin = 20;
    int topMargin = 38;
    int plotW = width() - leftMargin - rightMargin;
    int plotH = height() - topMargin - 20;
    if (plotW < 100 || plotH < 100) return;

    // 左圆环 (总体覆盖对比) + 右圆盘 (各函数健康占比)
    double rLeft = qBound(35.0, qMin(plotW * 0.18, plotH * 0.40), 95.0);
    QPointF centerLeft(leftMargin + plotW * 0.28, topMargin + plotH * 0.50);

    double rRight = rLeft;
    QPointF centerRight(leftMargin + plotW * 0.72, topMargin + plotH * 0.50);

    m_pieCenter = centerRight;
    m_pieRadius = rRight;
    m_pieSlices.clear();

    // -------------------------------------------------------------
    // 左环：总体语句覆盖率 (Covered vs Uncovered)
    // -------------------------------------------------------------
    QRectF rectLeft(centerLeft.x() - rLeft, centerLeft.y() - rLeft, rLeft * 2.0, rLeft * 2.0);
    double angleCovered = (totalStmts > 0) ? (360.0 * totalCovered / totalStmts) : 360.0;
    double angleUncovered = 360.0 - angleCovered;

    // 已覆盖扇形 (绿)
    painter.setPen(Qt::NoPen);
    painter.setBrush(isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#10b981")));
    painter.drawPie(rectLeft, 90 * 16, -qRound(angleCovered * 16));

    // 未覆盖扇形 (红)
    if (angleUncovered > 0.01) {
        painter.setBrush(isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8")));
        painter.drawPie(rectLeft, qRound((90 - angleCovered) * 16), -qRound(angleUncovered * 16));
    }

    // 挖去中心形成精致圆环 (Donut)
    double innerR = rLeft * 0.58;
    QRectF holeLeft(centerLeft.x() - innerR, centerLeft.y() - innerR, innerR * 2.0, innerR * 2.0);
    painter.setBrush(bgColor);
    painter.drawEllipse(holeLeft);

    // 圆环中心文字
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 13, QFont::Bold));
    painter.setPen(isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#10b981")));
    painter.drawText(holeLeft.adjusted(0, -6, 0, -6), Qt::AlignCenter, QString::number(covPercent, 'f', 1) + QStringLiteral("%"));

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    painter.setPen(subColor);
    painter.drawText(holeLeft.adjusted(0, 16, 0, 16), Qt::AlignCenter, QStringLiteral("总体覆盖率"));

    // 左侧副标题与数据
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 9, QFont::Bold));
    painter.setPen(textColor);
    painter.drawText(QRect(leftMargin, topMargin + 2, qRound(plotW * 0.45), 18), Qt::AlignCenter, QStringLiteral("总体语句覆盖比例"));

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    painter.drawText(QRect(leftMargin, topMargin + plotH - 18, qRound(plotW * 0.45), 18), Qt::AlignCenter,
                     QString("已覆盖 %1 行 / 未覆盖 %2 行 (总 %3 行)").arg(totalCovered).arg(totalUncovered).arg(totalStmts));

    // -------------------------------------------------------------
    // 右盘：各函数语句份额与覆盖率层级饼图
    // -------------------------------------------------------------
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 9, QFont::Bold));
    painter.setPen(textColor);
    painter.drawText(QRect(leftMargin + qRound(plotW * 0.50), topMargin + 2, qRound(plotW * 0.45), 18), Qt::AlignCenter, QStringLiteral("各函数规模与健康度分布"));

    double curAngle = 90.0;
    int count = m_data.size();
    for (int i = 0; i < count; ++i) {
        const auto& item = m_data[i];
        double sweep = (totalStmts > 0) ? (360.0 * item.statementCount / totalStmts) : (360.0 / qMax(1, count));
        if (sweep < 0.5) sweep = 0.5;

        // 颜色分层：绿色 (>=80%), 橙黄 (50-79%), 红色 (<50%)
        QColor sliceColor = (item.statementCoveragePercent >= 80.0) ? (isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#10b981"))) :
                            (item.statementCoveragePercent >= 50.0) ? (isLight ? QColor(QStringLiteral("#d97706")) : QColor(QStringLiteral("#f59e0b"))) :
                            (isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8")));

        bool isHovered = (i == m_hoveredIndex);
        bool isSelected = (item.functionName == m_selectedFunction);

        // 若被悬停或选中，扇形沿角平分线向外微突 7px (Explode effect)
        double midRad = (curAngle - sweep / 2.0) * M_PI / 180.0;
        double offset = (isHovered || isSelected) ? 7.0 : 0.0;
        double sx = centerRight.x() + offset * std::cos(midRad);
        double sy = centerRight.y() - offset * std::sin(midRad);

        QRectF sliceRect(sx - rRight, sy - rRight, rRight * 2.0, rRight * 2.0);

        painter.setPen(QPen(bgColor, 1.5));
        painter.setBrush(sliceColor);
        painter.drawPie(sliceRect, qRound(curAngle * 16), -qRound(sweep * 16));

        // 记录扇区用于命中测试
        PieSlice ps;
        ps.index = i;
        ps.startAngle = curAngle;
        ps.sweepAngle = sweep;
        ps.color = sliceColor;
        ps.label = item.functionName;
        m_pieSlices.append(ps);

        curAngle -= sweep;
    }

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    painter.setPen(subColor);
    painter.drawText(QRect(leftMargin + qRound(plotW * 0.50), topMargin + plotH - 18, qRound(plotW * 0.45), 18), Qt::AlignCenter,
                     QStringLiteral("💡 右键可切换柱状图/折线图，点击扇区可筛选函数"));
}

void CoverageBarChartWidget::mouseMoveEvent(QMouseEvent* event) {
    int oldHover = m_hoveredIndex;
    m_hoveredIndex = -1;
    QPointF pos = event->position();

    if (m_chartType == CoverageChartType::BarChart || m_chartType == CoverageChartType::LineChart) {
        for (int i = 0; i < m_barRects.size(); ++i) {
            if (m_barRects[i].contains(pos)) {
                m_hoveredIndex = i;
                break;
            }
        }
    } else if (m_chartType == CoverageChartType::PieChart) {
        // 测试右侧饼图各个扇区
        double dx = pos.x() - m_pieCenter.x();
        double dy = pos.y() - m_pieCenter.y();
        double dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= m_pieRadius + 12.0 && dist >= 8.0) {
            // 计算从正上方起逆时针的角度 [0, 360)
            double rad = std::atan2(-dy, dx); // [-PI, PI], y 向上
            double deg = rad * 180.0 / M_PI;  // [-180, 180]
            if (deg < 0) deg += 360.0;        // [0, 360]

            for (const auto& slice : m_pieSlices) {
                double endA = slice.startAngle - slice.sweepAngle;
                // 处理跨越 0 度情况
                double sa = slice.startAngle;
                while (sa < 0) sa += 360.0;
                while (sa >= 360.0) sa -= 360.0;

                // 简化判断角度范围
                double normDeg = deg;
                double aDiff = slice.startAngle - normDeg;
                while (aDiff < 0) aDiff += 360.0;
                if (aDiff <= slice.sweepAngle) {
                    m_hoveredIndex = slice.index;
                    break;
                }
            }
        }
    }

    if (m_hoveredIndex != oldHover) {
        update();
        if (m_hoveredIndex >= 0 && m_hoveredIndex < m_data.size()) {
            const auto& item = m_data[m_hoveredIndex];
            QString tip = QString("<b>函数: %1</b><br/>"
                                  "所在文件: %2<br/>"
                                  "总语句数: %3 行<br/>"
                                  "已覆盖语句: %4 行 (<b>%5%</b>)<br/>"
                                  "总分支数: %6 个<br/>"
                                  "已覆盖分支: %7 个 (<b>%8%</b>)")
                              .arg(item.functionName)
                              .arg(item.fileName)
                              .arg(item.statementCount)
                              .arg(item.coveredStatements)
                              .arg(QString::number(item.statementCoveragePercent, 'f', 2))
                              .arg(item.branchCount)
                              .arg(item.coveredBranches)
                              .arg(QString::number(item.branchCoveragePercent, 'f', 2));
            QToolTip::showText(event->globalPosition().toPoint(), tip, this);
        } else {
            QToolTip::hideText();
        }
    }
}

void CoverageBarChartWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_hoveredIndex >= 0 && m_hoveredIndex < m_data.size()) {
            m_selectedFunction = m_data[m_hoveredIndex].functionName;
            update();
            emit functionClicked(m_selectedFunction);
        }
    }
}

void CoverageBarChartWidget::leaveEvent(QEvent* /* event */) {
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        update();
    }
}

// =============================================================================
// 2. CoverageGutterArea & CoverageCodeViewer
// =============================================================================

CoverageGutterArea::CoverageGutterArea(CoverageCodeViewer* viewer)
    : QWidget(viewer)
    , m_viewer(viewer)
{
}

QSize CoverageGutterArea::sizeHint() const {
    return QSize(m_viewer->gutterWidth(), 0);
}

void CoverageGutterArea::paintEvent(QPaintEvent* event) {
    m_viewer->gutterPaintEvent(event);
}

CoverageCodeViewer::CoverageCodeViewer(QWidget* parent)
    : QPlainTextEdit(parent)
{
    m_gutter = new CoverageGutterArea(this);
    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setFont(QFont(QStringLiteral("Consolas"), 10));

    m_highlighter = new CSyntaxHighlighter(document());

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CoverageCodeViewer::updateGutterWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CoverageCodeViewer::updateGutterArea);

    updateGutterWidth(0);
}

int CoverageCodeViewer::gutterWidth() const {
    int digits = 1;
    int maxLine = qMax(1, blockCount());
    while (maxLine >= 10) {
        maxLine /= 10;
        ++digits;
    }
    // 留出状态色块 (6px) + 间隙 (6px) + 行号宽度 + 右边距 (8px)
    int space = 18 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 8;
    return space;
}

void CoverageCodeViewer::updateGutterWidth(int /* newBlockCount */) {
    setViewportMargins(gutterWidth(), 0, 0, 0);
}

void CoverageCodeViewer::updateGutterArea(const QRect& rect, int dy) {
    if (dy) {
        m_gutter->scroll(0, dy);
    } else {
        m_gutter->update(0, rect.y(), m_gutter->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) {
        updateGutterWidth(0);
    }
}

void CoverageCodeViewer::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_gutter->setGeometry(QRect(cr.left(), cr.top(), gutterWidth(), cr.height()));
}

void CoverageCodeViewer::setSourceAndCoverage(const QString& sourceCode, const CoverageReport& report) {
    m_coverageReport = report;
    m_navigatedLine = -1;
    setPlainText(sourceCode);
    updateCoverageHighlights();
    m_gutter->update();
}

void CoverageCodeViewer::gotoLine(int line) {
    m_navigatedLine = line;
    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        cursor.select(QTextCursor::LineUnderCursor);
        setTextCursor(cursor);
        setFocus();
        centerCursor();
        updateCoverageHighlights();
        m_gutter->update();
    }
}

void CoverageCodeViewer::findNextUncovered() {
    int currentLine = textCursor().blockNumber() + 1;
    int total = blockCount();
    for (int l = currentLine + 1; l <= total; ++l) {
        if (m_coverageReport.lineDetails.contains(l)) {
            const auto& ld = m_coverageReport.lineDetails.value(l);
            if (ld.status == LineCoverageStatus::Uncovered || ld.status == LineCoverageStatus::PartialBranch) {
                gotoLine(l);
                QToolTip::showText(mapToGlobal(QPoint(width() / 2, 40)),
                                   QStringLiteral("📍 已定位至未覆盖/部分分支代码行: 第 %1 行").arg(l));
                return;
            }
        }
    }
    // 循环从头查找
    for (int l = 1; l <= currentLine; ++l) {
        if (m_coverageReport.lineDetails.contains(l)) {
            const auto& ld = m_coverageReport.lineDetails.value(l);
            if (ld.status == LineCoverageStatus::Uncovered || ld.status == LineCoverageStatus::PartialBranch) {
                gotoLine(l);
                QToolTip::showText(mapToGlobal(QPoint(width() / 2, 40)),
                                   QStringLiteral("📍 已循环定位至未覆盖/部分分支代码行: 第 %1 行").arg(l));
                return;
            }
        }
    }

    QToolTip::showText(mapToGlobal(QPoint(width() / 2, 40)),
                       QStringLiteral("🎉 恭喜！当前源文件中未检测到未覆盖行。"));
}

void CoverageCodeViewer::findPrevUncovered() {
    int currentLine = textCursor().blockNumber() + 1;
    int total = blockCount();
    for (int l = currentLine - 1; l >= 1; --l) {
        if (m_coverageReport.lineDetails.contains(l)) {
            const auto& ld = m_coverageReport.lineDetails.value(l);
            if (ld.status == LineCoverageStatus::Uncovered || ld.status == LineCoverageStatus::PartialBranch) {
                gotoLine(l);
                QToolTip::showText(mapToGlobal(QPoint(width() / 2, 40)),
                                   QStringLiteral("📍 已定位至上一处未覆盖/部分分支代码行: 第 %1 行").arg(l));
                return;
            }
        }
    }
    for (int l = total; l >= currentLine; --l) {
        if (m_coverageReport.lineDetails.contains(l)) {
            const auto& ld = m_coverageReport.lineDetails.value(l);
            if (ld.status == LineCoverageStatus::Uncovered || ld.status == LineCoverageStatus::PartialBranch) {
                gotoLine(l);
                QToolTip::showText(mapToGlobal(QPoint(width() / 2, 40)),
                                   QStringLiteral("📍 已循环定位至末尾未覆盖/部分分支代码行: 第 %1 行").arg(l));
                return;
            }
        }
    }

    QToolTip::showText(mapToGlobal(QPoint(width() / 2, 40)),
                       QStringLiteral("🎉 恭喜！当前源文件中未检测到未覆盖行。"));
}

void CoverageCodeViewer::updateCoverageHighlights() {
    QList<QTextEdit::ExtraSelection> selections;
    bool isLight = (m_theme == ThemeType::LightModern);

    // 覆盖颜色 (淡雅背景)
    QColor covBg = isLight ? QColor(QStringLiteral("#e6f4ea")) : QColor(QStringLiteral("#133827"));
    QColor uncovBg = isLight ? QColor(QStringLiteral("#fee2e2")) : QColor(QStringLiteral("#451a1a"));
    QColor partialBg = isLight ? QColor(QStringLiteral("#fef3c7")) : QColor(QStringLiteral("#423315"));

    for (auto it = m_coverageReport.lineDetails.constBegin(); it != m_coverageReport.lineDetails.constEnd(); ++it) {
        int lineNo = it.key();
        const auto& ld = it.value();

        QTextBlock block = document()->findBlockByLineNumber(lineNo - 1);
        if (block.isValid()) {
            QString blockText = block.text().trimmed();
            // 纯注释行或空行严禁被着色覆盖状态
            if (blockText.isEmpty() ||
                blockText.startsWith(QStringLiteral("//")) ||
                blockText.startsWith(QStringLiteral("/*")) ||
                blockText.startsWith(QLatin1Char('*')) ||
                blockText.endsWith(QStringLiteral("*/"))) {
                continue;
            }

            QTextEdit::ExtraSelection sel;
            sel.cursor = QTextCursor(block);
            sel.cursor.select(QTextCursor::LineUnderCursor);
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);

            LineCoverageStatus effectiveStatus = ld.status;
            // 若该行无条件分支关键字且被标记为 PartialBranch，则校正为 Covered 或 Uncovered
            if (effectiveStatus == LineCoverageStatus::PartialBranch) {
                if (!blockText.contains(QStringLiteral("if")) &&
                    !blockText.contains(QStringLiteral("switch")) &&
                    !blockText.contains(QStringLiteral("while")) &&
                    !blockText.contains(QStringLiteral("for")) &&
                    !blockText.contains(QLatin1Char('?')))
                {
                    effectiveStatus = (ld.hitCount > 0) ? LineCoverageStatus::Covered : LineCoverageStatus::Uncovered;
                }
            }

            if (effectiveStatus == LineCoverageStatus::Covered) {
                sel.format.setBackground(covBg);
            } else if (effectiveStatus == LineCoverageStatus::Uncovered) {
                sel.format.setBackground(uncovBg);
            } else if (effectiveStatus == LineCoverageStatus::PartialBranch) {
                sel.format.setBackground(partialBg);
            } else {
                continue;
            }
            selections.append(sel);
        }
    }

    // 醒目光标与导航焦点行高亮标记 (Requirement: 上一处/下一处未覆盖光标位置标记)
    if (m_navigatedLine > 0) {
        QTextBlock navBlock = document()->findBlockByLineNumber(m_navigatedLine - 1);
        if (navBlock.isValid()) {
            QTextEdit::ExtraSelection navSel;
            navSel.cursor = QTextCursor(navBlock);
            navSel.cursor.select(QTextCursor::LineUnderCursor);
            navSel.format.setProperty(QTextFormat::FullWidthSelection, true);

            if (isLight) {
                navSel.format.setBackground(QColor(QStringLiteral("#ffccd0")));
                navSel.format.setProperty(QTextFormat::OutlinePen, QPen(QColor(QStringLiteral("#dc2626")), 2, Qt::SolidLine));
            } else {
                navSel.format.setBackground(QColor(QStringLiteral("#6b2121")));
                navSel.format.setProperty(QTextFormat::OutlinePen, QPen(QColor(QStringLiteral("#f38ba8")), 2, Qt::SolidLine));
            }
            selections.append(navSel);
        }
    }

    setExtraSelections(selections);
}

void CoverageCodeViewer::applyTheme(ThemeType theme) {
    m_theme = theme;
    bool isLight = (m_theme == ThemeType::LightModern);

    if (isLight) {
        setStyleSheet(QStringLiteral("QPlainTextEdit { background-color: #ffffff; color: #202124; border: 1px solid #dadce0; selection-background-color: #c2dbff; }"));
    } else {
        setStyleSheet(QStringLiteral("QPlainTextEdit { background-color: #1e1e2e; color: #cdd6f4; border: 1px solid #313244; selection-background-color: #45475a; }"));
    }

    if (m_highlighter) {
        m_highlighter->rehighlight();
    }
    updateCoverageHighlights();
    m_gutter->update();
}

void CoverageCodeViewer::gutterPaintEvent(QPaintEvent* event) {
    QPainter painter(m_gutter);
    bool isLight = (m_theme == ThemeType::LightModern);

    QColor gutterBg = isLight ? QColor(QStringLiteral("#f8f9fa")) : QColor(QStringLiteral("#181825"));
    QColor numColor = isLight ? QColor(QStringLiteral("#70757a")) : QColor(QStringLiteral("#6c7086"));

    painter.fillRect(event->rect(), gutterBg);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNo = blockNumber + 1;
            QString blockText = block.text().trimmed();
            bool isComment = blockText.isEmpty() ||
                             blockText.startsWith(QStringLiteral("//")) ||
                             blockText.startsWith(QStringLiteral("/*")) ||
                             blockText.startsWith(QLatin1Char('*')) ||
                             blockText.endsWith(QStringLiteral("*/"));

            // 绘制左侧覆盖状态竖条 (过滤纯注释)
            if (!isComment && m_coverageReport.lineDetails.contains(lineNo)) {
                const auto& ld = m_coverageReport.lineDetails.value(lineNo);
                QColor statusColor = Qt::transparent;

                LineCoverageStatus effectiveStatus = ld.status;
                if (effectiveStatus == LineCoverageStatus::PartialBranch) {
                    if (!blockText.contains(QStringLiteral("if")) &&
                        !blockText.contains(QStringLiteral("switch")) &&
                        !blockText.contains(QStringLiteral("while")) &&
                        !blockText.contains(QStringLiteral("for")) &&
                        !blockText.contains(QLatin1Char('?')))
                    {
                        effectiveStatus = (ld.hitCount > 0) ? LineCoverageStatus::Covered : LineCoverageStatus::Uncovered;
                    }
                }

                if (effectiveStatus == LineCoverageStatus::Covered) {
                    statusColor = QColor(QStringLiteral("#10b981")); // 绿色
                } else if (effectiveStatus == LineCoverageStatus::Uncovered) {
                    statusColor = QColor(QStringLiteral("#ef4444")); // 红色
                } else if (effectiveStatus == LineCoverageStatus::PartialBranch) {
                    statusColor = QColor(QStringLiteral("#f59e0b")); // 橙黄
                }

                if (statusColor != Qt::transparent) {
                    painter.fillRect(QRect(2, top + 1, 4, fontMetrics().height() - 2), statusColor);
                }
            }

            // 绘制行号
            painter.setPen((lineNo == m_navigatedLine) ? QColor(QStringLiteral("#dc2626")) : numColor);
            painter.setFont(QFont(QStringLiteral("Consolas"), 9, (lineNo == m_navigatedLine) ? QFont::Bold : QFont::Normal));
            QString number = QString::number(lineNo);
            painter.drawText(0, top, m_gutter->width() - 12, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);

            // 当前导航高亮行的醒目指示箭头 ▶
            if (lineNo == m_navigatedLine) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(QStringLiteral("#dc2626")));
                QPolygonF arrow;
                int ax = m_gutter->width() - 8;
                int ay = top + (fontMetrics().height() / 2);
                arrow << QPointF(ax - 6, ay - 4)
                      << QPointF(ax, ay)
                      << QPointF(ax - 6, ay + 4);
                painter.drawPolygon(arrow);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// =============================================================================
// 3. CoverageView 测试覆盖分析视窗主实现
// =============================================================================

CoverageView::CoverageView(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void CoverageView::setupUi() {
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(4, 4, 4, 4);
    rootLayout->setSpacing(4);

    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);

    // -------------------------------------------------------------
    // 3.1 左侧面板：文件/函数单选、搜索过滤、对象列表
    // -------------------------------------------------------------
    auto* leftPanel = new QWidget(mainSplitter);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(2, 2, 2, 2);
    leftLayout->setSpacing(6);

    // 单选切换行
    auto* radioLayout = new QHBoxLayout();
    radioLayout->setContentsMargins(0, 0, 0, 0);
    radioLayout->setSpacing(12);

    m_radioFileMode = new QRadioButton(QStringLiteral("文件"), leftPanel);
    m_radioFuncMode = new QRadioButton(QStringLiteral("函数"), leftPanel);
    m_radioFuncMode->setChecked(true); // 默认选函数

    radioLayout->addWidget(m_radioFileMode);
    radioLayout->addWidget(m_radioFuncMode);
    radioLayout->addStretch();
    leftLayout->addLayout(radioLayout);

    connect(m_radioFileMode, &QRadioButton::toggled, this, &CoverageView::onModeRadioToggled);
    connect(m_radioFuncMode, &QRadioButton::toggled, this, &CoverageView::onModeRadioToggled);

    // 快速搜索输入框
    m_editSearch = new QLineEdit(leftPanel);
    m_editSearch->setPlaceholderText(QStringLiteral("Enter text to search..."));
    m_editSearch->setClearButtonEnabled(true);
    connect(m_editSearch, &QLineEdit::textChanged, this, &CoverageView::onSearchTextChanged);
    leftLayout->addWidget(m_editSearch);

    // 列表框
    m_listItems = new QListWidget(leftPanel);
    m_listItems->setAlternatingRowColors(true);
    m_listItems->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_listItems, &QListWidget::itemClicked, this, &CoverageView::onListItemClicked);
    connect(m_listItems, &QListWidget::itemDoubleClicked, this, &CoverageView::onListItemDoubleClicked);
    leftLayout->addWidget(m_listItems, 1);

    mainSplitter->addWidget(leftPanel);

    // -------------------------------------------------------------
    // 3.2 右侧主区域：底部带两大 Tab [覆盖统计] 与 [代码覆盖]
    // -------------------------------------------------------------
    m_tabWidget = new QTabWidget(mainSplitter);
    m_tabWidget->setTabPosition(QTabWidget::South); // 置于底部，与用户原型一致

    // Tab 1: 覆盖统计 (上表 + 下柱状图)
    m_tabStats = new QWidget(m_tabWidget);
    auto* statsLayout = new QVBoxLayout(m_tabStats);
    statsLayout->setContentsMargins(2, 2, 2, 2);
    statsLayout->setSpacing(4);

    auto* statsSplitter = new QSplitter(Qt::Vertical, m_tabStats);

    // 上部 9 列大表
    m_tableCoverage = new QTableWidget(statsSplitter);
    m_tableCoverage->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableCoverage->setColumnCount(9);
    m_tableCoverage->setHorizontalHeaderLabels({
        QStringLiteral("序号"),
        QStringLiteral("函数名称"),
        QStringLiteral("文件名"),
        QStringLiteral("语句数"),
        QStringLiteral("语句覆盖数"),
        QStringLiteral("语句覆盖率(%)"),
        QStringLiteral("分支数"),
        QStringLiteral("分支覆盖数"),
        QStringLiteral("分支覆盖率(%)")
    });
    m_tableCoverage->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableCoverage->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableCoverage->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    for (int col = 3; col < 9; ++col) {
        m_tableCoverage->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    }
    m_tableCoverage->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableCoverage->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableCoverage->setAlternatingRowColors(true);
    m_tableCoverage->verticalHeader()->setVisible(false);
    connect(m_tableCoverage, &QTableWidget::itemSelectionChanged, this, &CoverageView::onTableSelectionChanged);
    connect(m_tableCoverage, &QTableWidget::cellDoubleClicked, this, &CoverageView::onTableDoubleClicked);

    statsSplitter->addWidget(m_tableCoverage);

    // 下部柱状图
    m_chartWidget = new CoverageBarChartWidget(statsSplitter);
    connect(m_chartWidget, &CoverageBarChartWidget::functionClicked, this, &CoverageView::onChartFunctionClicked);
    statsSplitter->addWidget(m_chartWidget);

    statsSplitter->setStretchFactor(0, 3);
    statsSplitter->setStretchFactor(1, 2);

    statsLayout->addWidget(statsSplitter);
    m_tabWidget->addTab(m_tabStats, QStringLiteral("覆盖统计"));

    // Tab 2: 代码覆盖 (顶部状态 + 带色代码阅读器)
    m_tabCode = new QWidget(m_tabWidget);
    auto* codeLayout = new QVBoxLayout(m_tabCode);
    codeLayout->setContentsMargins(4, 4, 4, 4);
    codeLayout->setSpacing(6);

    // 顶部控制条
    auto* topCodeBar = new QHBoxLayout();
    topCodeBar->setSpacing(8);

    topCodeBar->addWidget(new QLabel(QStringLiteral("📄 源码文件:"), m_tabCode));
    m_cmbFiles = new QComboBox(m_tabCode);
    m_cmbFiles->setMinimumWidth(220);
    connect(m_cmbFiles, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CoverageView::onFileComboIndexChanged);
    topCodeBar->addWidget(m_cmbFiles);

    m_lblStmtBadge = new QLabel(QStringLiteral("语句覆盖: 0.0%"), m_tabCode);
    m_lblStmtBadge->setStyleSheet(QStringLiteral("padding: 2px 8px; border-radius: 3px; font-weight: bold; background: #10b981; color: #ffffff;"));
    topCodeBar->addWidget(m_lblStmtBadge);

    m_lblBranchBadge = new QLabel(QStringLiteral("分支覆盖: 0.0%"), m_tabCode);
    m_lblBranchBadge->setStyleSheet(QStringLiteral("padding: 2px 8px; border-radius: 3px; font-weight: bold; background: #0284c7; color: #ffffff;"));
    topCodeBar->addWidget(m_lblBranchBadge);

    m_lblFuncBadge = new QLabel(QStringLiteral("函数覆盖: 0.0%"), m_tabCode);
    m_lblFuncBadge->setStyleSheet(QStringLiteral("padding: 2px 8px; border-radius: 3px; font-weight: bold; background: #8b5cf6; color: #ffffff;"));
    topCodeBar->addWidget(m_lblFuncBadge);

    topCodeBar->addStretch();

    m_btnPrevUncovered = new QPushButton(QStringLiteral("◀ 上一未覆盖"), m_tabCode);
    m_btnNextUncovered = new QPushButton(QStringLiteral("下一未覆盖 ▶"), m_tabCode);
    connect(m_btnPrevUncovered, &QPushButton::clicked, this, [this]() {
        if (m_codeViewer) m_codeViewer->findPrevUncovered();
    });
    connect(m_btnNextUncovered, &QPushButton::clicked, this, [this]() {
        if (m_codeViewer) m_codeViewer->findNextUncovered();
    });
    topCodeBar->addWidget(m_btnPrevUncovered);
    topCodeBar->addWidget(m_btnNextUncovered);

    codeLayout->addLayout(topCodeBar);

    // 覆盖图例栏 (Requirement 5: 明确定义并展示已覆盖与未覆盖的颜色图例)
    auto* legendBar = new QHBoxLayout();
    legendBar->setContentsMargins(0, 2, 0, 2);
    legendBar->setSpacing(10);
    auto* lblLegendTitle = new QLabel(QStringLiteral("🎨 覆盖着色图例:"), m_tabCode);
    lblLegendTitle->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 11px;"));
    legendBar->addWidget(lblLegendTitle);

    auto makeLegendChip = [this](const QString& text, const QString& bgStyle) -> QLabel* {
        auto* chip = new QLabel(text, m_tabCode);
        chip->setStyleSheet(QStringLiteral("padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; ") + bgStyle);
        return chip;
    };

    legendBar->addWidget(makeLegendChip(QStringLiteral("🟩 已覆盖执行行 (Covered)"),
        QStringLiteral("background: #10b981; color: #ffffff; border: 1px solid #059669;")));
    legendBar->addWidget(makeLegendChip(QStringLiteral("🟨 部分分支覆盖 (Partial)"),
        QStringLiteral("background: #f59e0b; color: #ffffff; border: 1px solid #d97706;")));
    legendBar->addWidget(makeLegendChip(QStringLiteral("🟥 未覆盖可执行行 (Uncovered)"),
        QStringLiteral("background: #ef4444; color: #ffffff; border: 1px solid #dc2626;")));
    legendBar->addWidget(makeLegendChip(QStringLiteral("⬜ 非执行/声明/注释 (Ignored)"),
        QStringLiteral("background: #64748b; color: #ffffff; border: 1px solid #475569;")));

    legendBar->addStretch();
    codeLayout->addLayout(legendBar);

    m_codeViewer = new CoverageCodeViewer(m_tabCode);
    codeLayout->addWidget(m_codeViewer, 1);

    m_tabWidget->addTab(m_tabCode, QStringLiteral("代码覆盖"));

    mainSplitter->addWidget(m_tabWidget);

    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 4);

    rootLayout->addWidget(mainSplitter);
}

void CoverageView::setCoverageReport(const CoverageReport& report,
                                   const QVector<FunctionMetrics>& functions,
                                   const QString& sourceCode,
                                   const QString& sourceFile)
{
    m_report = report;
    if (!functions.isEmpty()) {
        m_functions = functions;
    }

    if (!sourceFile.isEmpty()) {
        m_activeFile = sourceFile;
        if (!sourceCode.isEmpty()) {
            m_fileContents[sourceFile] = sourceCode;
        }
    } else if (!report.filePath.isEmpty()) {
        m_activeFile = report.filePath;
        if (!sourceCode.isEmpty()) {
            m_fileContents[m_activeFile] = sourceCode;
        }
    }

    // 若当前源码为空，尝试从物理文件读取
    if (!m_activeFile.isEmpty() && !m_fileContents.contains(m_activeFile) && QFile::exists(m_activeFile)) {
        QFile f(m_activeFile);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_fileContents[m_activeFile] = QString::fromUtf8(f.readAll());
            f.close();
        }
    }

    // 若 report.functionCoverages 为空，依据 m_functions 自动生成
    if (m_report.functionCoverages.isEmpty() && !m_functions.isEmpty()) {
        int idx = 0;
        for (const auto& fn : m_functions) {
            FunctionCoverageInfo fc;
            fc.index = idx++;
            fc.functionName = fn.name;
            fc.fileName = !fn.filePath.isEmpty() ? fn.filePath : m_activeFile;
            fc.startLine = fn.startLine;
            fc.endLine = fn.endLine;
            fc.statementCount = qMax(1, fn.statementCount);
            fc.branchCount = fn.branchCount;

            int hitStmts = 0;
            int hitBranches = 0;
            int bCount = 0;

            for (int l = fn.startLine; l <= fn.endLine; ++l) {
                if (m_report.lineDetails.contains(l)) {
                    const auto& ld = m_report.lineDetails.value(l);
                    if (ld.hitCount > 0) hitStmts++;
                    for (const auto& bp : ld.branches) {
                        bCount++;
                        if (bp.isFullyCovered() || bp.isPartiallyCovered()) {
                            hitBranches++;
                        }
                    }
                }
            }

            fc.coveredStatements = qMin(fc.statementCount, hitStmts);
            fc.statementCoveragePercent = (fc.statementCount > 0) ? (100.0 * fc.coveredStatements / fc.statementCount) : 100.0;
            if (fc.branchCount <= 0 && bCount > 0) fc.branchCount = bCount;
            fc.coveredBranches = qMin(fc.branchCount, hitBranches);
            fc.branchCoveragePercent = (fc.branchCount > 0) ? (100.0 * fc.coveredBranches / fc.branchCount) : 100.0;

            m_report.functionCoverages.append(fc);
        }
    }

    // 自动自愈与同步指标统计 (Requirement 3: 解决函数总数为 0 问题)
    if (m_report.totalFunctions <= 0 && !m_report.functionCoverages.isEmpty()) {
        m_report.totalFunctions = m_report.functionCoverages.size();
        int covF = 0;
        int totStmts = 0;
        int covStmts = 0;
        int totBr = 0;
        int covBr = 0;
        for (const auto& fc : m_report.functionCoverages) {
            if (fc.coveredStatements > 0) covF++;
            totStmts += fc.statementCount;
            covStmts += fc.coveredStatements;
            totBr += fc.branchCount;
            covBr += fc.coveredBranches;
        }
        m_report.coveredFunctions = covF;
        if (m_report.executableLines <= 0) {
            m_report.executableLines = totStmts;
            m_report.coveredLines = covStmts;
        }
        if (m_report.totalBranches <= 0) {
            m_report.totalBranches = totBr;
            m_report.coveredBranches = covBr;
        }
    }

    populateList();
    populateTable();
    updateKpiBadges();

    // 更新柱状图
    m_chartWidget->setData(m_report.functionCoverages);

    // 更新文件下拉列表 (包含当前工程所有源文件、头文件以及已有覆盖结果的文件)
    m_cmbFiles->blockSignals(true);
    m_cmbFiles->clear();
    QSet<QString> fileSet;
    if (ProjectManager::instance().hasActiveProject()) {
        const auto& proj = ProjectManager::instance().currentProject();
        for (const QString& sf : proj.sourceFiles) {
            if (!sf.isEmpty()) fileSet.insert(QDir::cleanPath(sf));
        }
        if (!proj.rootDir.isEmpty()) {
            QDir srcDir(proj.rootDir + QStringLiteral("/Source"));
            if (!srcDir.exists()) srcDir = QDir(proj.rootDir + QStringLiteral("/source"));
            if (srcDir.exists()) {
                for (const auto& fi : srcDir.entryInfoList({QStringLiteral("*.c"), QStringLiteral("*.cpp")}, QDir::Files)) {
                    fileSet.insert(QDir::cleanPath(fi.absoluteFilePath()));
                }
            }
            QDir hdrDir(proj.rootDir + QStringLiteral("/Header"));
            if (!hdrDir.exists()) hdrDir = QDir(proj.rootDir + QStringLiteral("/header"));
            if (hdrDir.exists()) {
                for (const auto& fi : hdrDir.entryInfoList({QStringLiteral("*.h"), QStringLiteral("*.hpp")}, QDir::Files)) {
                    fileSet.insert(QDir::cleanPath(fi.absoluteFilePath()));
                }
            }
        }
    }
    for (const auto& fc : m_report.functionCoverages) {
        if (!fc.fileName.isEmpty()) fileSet.insert(QDir::cleanPath(fc.fileName));
    }
    if (!m_activeFile.isEmpty()) fileSet.insert(QDir::cleanPath(m_activeFile));

    QStringList sortedFiles = fileSet.values();
    std::sort(sortedFiles.begin(), sortedFiles.end(), [](const QString& a, const QString& b) {
        return QFileInfo(a).fileName().compare(QFileInfo(b).fileName(), Qt::CaseInsensitive) < 0;
    });

    for (const QString& fp : sortedFiles) {
        m_cmbFiles->addItem(QFileInfo(fp).fileName(), fp);
    }
    int activeIdx = m_cmbFiles->findData(QDir::cleanPath(m_activeFile));
    if (activeIdx == -1) {
        activeIdx = m_cmbFiles->findData(m_activeFile);
    }
    if (activeIdx != -1) {
        m_cmbFiles->setCurrentIndex(activeIdx);
    }
    m_cmbFiles->blockSignals(false);

    // 载入代码视图
    if (m_fileContents.contains(m_activeFile)) {
        CoverageReport fileRep = m_report;
        QString cleanActive = QDir::cleanPath(m_activeFile);
        if (m_report.fileReports.contains(cleanActive)) {
            fileRep = m_report.fileReports.value(cleanActive);
        }
        m_codeViewer->setSourceAndCoverage(m_fileContents[m_activeFile], fileRep);
    }
}

void CoverageView::setSourceFile(const QString& filePath, const QString& content) {
    m_activeFile = filePath;
    m_fileContents[filePath] = content;
    if (m_codeViewer) {
        CoverageReport fileRep = m_report;
        QString cleanTarget = QDir::cleanPath(filePath);
        if (m_report.fileReports.contains(cleanTarget)) {
            fileRep = m_report.fileReports.value(cleanTarget);
        }
        m_codeViewer->setSourceAndCoverage(content, fileRep);
    }
    if (m_cmbFiles) {
        int idx = m_cmbFiles->findData(QDir::cleanPath(filePath));
        if (idx == -1) idx = m_cmbFiles->findData(filePath);
        if (idx != -1) {
            m_cmbFiles->blockSignals(true);
            m_cmbFiles->setCurrentIndex(idx);
            m_cmbFiles->blockSignals(false);
        } else {
            m_cmbFiles->blockSignals(true);
            m_cmbFiles->addItem(QFileInfo(filePath).fileName(), filePath);
            m_cmbFiles->setCurrentIndex(m_cmbFiles->count() - 1);
            m_cmbFiles->blockSignals(false);
        }
    }
}

void CoverageView::clear() {
    m_report = CoverageReport();
    m_functions.clear();
    m_fileContents.clear();
    m_activeFile.clear();

    m_listItems->clear();
    m_tableCoverage->setRowCount(0);
    m_chartWidget->setData({});
    m_codeViewer->clear();
    updateKpiBadges();
}

void CoverageView::populateList() {
    m_listItems->clear();
    QString filter = m_editSearch->text().trimmed();

    if (m_radioFuncMode->isChecked()) {
        QString activeBaseName = QFileInfo(m_activeFile).fileName();
        QString cleanActive = QDir::cleanPath(m_activeFile);

        int count = 0;
        QSet<QString> addedFuncs;

        for (const auto& fc : m_report.functionCoverages) {
            bool matchFile = m_activeFile.isEmpty() ||
                             fc.fileName.isEmpty() ||
                             (QDir::cleanPath(fc.fileName) == cleanActive) ||
                             (QFileInfo(fc.fileName).fileName().compare(activeBaseName, Qt::CaseInsensitive) == 0);
            if (!matchFile) continue;

            if (filter.isEmpty() || fc.functionName.contains(filter, Qt::CaseInsensitive)) {
                auto* item = new QListWidgetItem(fc.functionName, m_listItems);
                item->setData(Qt::UserRole, fc.functionName);
                item->setData(Qt::UserRole + 1, fc.fileName);
                item->setData(Qt::UserRole + 2, fc.startLine);
                item->setIcon(QIcon(QStringLiteral(":/icons/static_analysis.svg")));
                addedFuncs.insert(fc.functionName);
                count++;
            }
        }

        // 若当前选定文件在 m_functions 中存在但未在 m_report.functionCoverages 中出现，也一并补充展示
        if (!m_activeFile.isEmpty()) {
            for (const auto& fn : m_functions) {
                bool matchFile = (QDir::cleanPath(fn.filePath) == cleanActive) ||
                                 (QFileInfo(fn.filePath).fileName().compare(activeBaseName, Qt::CaseInsensitive) == 0);
                if (matchFile && !addedFuncs.contains(fn.name)) {
                    if (filter.isEmpty() || fn.name.contains(filter, Qt::CaseInsensitive)) {
                        auto* item = new QListWidgetItem(fn.name, m_listItems);
                        item->setData(Qt::UserRole, fn.name);
                        item->setData(Qt::UserRole + 1, fn.filePath);
                        item->setData(Qt::UserRole + 2, fn.startLine);
                        item->setIcon(QIcon(QStringLiteral(":/icons/static_analysis.svg")));
                        addedFuncs.insert(fn.name);
                        count++;
                    }
                }
            }
        }

        // 若当前文件没有过滤出特定函数且用户未在搜索栏输入，回退展示所有函数，防止列表完全空白
        if (count == 0 && filter.isEmpty()) {
            for (const auto& fc : m_report.functionCoverages) {
                if (!addedFuncs.contains(fc.functionName)) {
                    auto* item = new QListWidgetItem(fc.functionName, m_listItems);
                    item->setData(Qt::UserRole, fc.functionName);
                    item->setData(Qt::UserRole + 1, fc.fileName);
                    item->setData(Qt::UserRole + 2, fc.startLine);
                    item->setIcon(QIcon(QStringLiteral(":/icons/static_analysis.svg")));
                    addedFuncs.insert(fc.functionName);
                }
            }
        }
    } else {
        // 文件模式：加载当前项目下的所有源文件
        QSet<QString> uniqueFiles;
        if (ProjectManager::instance().hasActiveProject()) {
            const auto& proj = ProjectManager::instance().currentProject();
            for (const QString& sf : proj.sourceFiles) {
                if (!sf.isEmpty()) uniqueFiles.insert(QDir::cleanPath(sf));
            }
            if (!proj.rootDir.isEmpty()) {
                QDir srcDir(proj.rootDir + QStringLiteral("/Source"));
                if (!srcDir.exists()) srcDir = QDir(proj.rootDir + QStringLiteral("/source"));
                if (srcDir.exists()) {
                    for (const auto& fi : srcDir.entryInfoList({QStringLiteral("*.c"), QStringLiteral("*.cpp")}, QDir::Files)) {
                        uniqueFiles.insert(QDir::cleanPath(fi.absoluteFilePath()));
                    }
                }
                QDir hdrDir(proj.rootDir + QStringLiteral("/Header"));
                if (!hdrDir.exists()) hdrDir = QDir(proj.rootDir + QStringLiteral("/header"));
                if (hdrDir.exists()) {
                    for (const auto& fi : hdrDir.entryInfoList({QStringLiteral("*.h"), QStringLiteral("*.hpp")}, QDir::Files)) {
                        uniqueFiles.insert(QDir::cleanPath(fi.absoluteFilePath()));
                    }
                }
            }
        }
        for (const auto& fc : m_report.functionCoverages) {
            if (!fc.fileName.isEmpty()) uniqueFiles.insert(QDir::cleanPath(fc.fileName));
        }
        if (!m_activeFile.isEmpty()) uniqueFiles.insert(QDir::cleanPath(m_activeFile));

        QStringList sortedFiles = uniqueFiles.values();
        std::sort(sortedFiles.begin(), sortedFiles.end(), [](const QString& a, const QString& b) {
            return QFileInfo(a).fileName().compare(QFileInfo(b).fileName(), Qt::CaseInsensitive) < 0;
        });

        for (const QString& fp : sortedFiles) {
            QString name = QFileInfo(fp).fileName();
            if (filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive) || fp.contains(filter, Qt::CaseInsensitive)) {
                auto* item = new QListWidgetItem(name, m_listItems);
                item->setData(Qt::UserRole, fp);
                item->setData(Qt::UserRole + 1, fp);
                item->setToolTip(fp);
                if (name.endsWith(QStringLiteral(".h"), Qt::CaseInsensitive) || name.endsWith(QStringLiteral(".hpp"), Qt::CaseInsensitive)) {
                    item->setIcon(QIcon(QStringLiteral(":/icons/h_file.svg")));
                } else {
                    item->setIcon(QIcon(QStringLiteral(":/icons/c_file.svg")));
                }
            }
        }
    }
}

void CoverageView::populateTable() {
    m_tableCoverage->setRowCount(0);
    int row = 0;

    for (const auto& fc : m_report.functionCoverages) {
        m_tableCoverage->insertRow(row);

        auto* itemIdx = new QTableWidgetItem(QString::number(fc.index));
        itemIdx->setTextAlignment(Qt::AlignCenter);

        auto* itemFunc = new QTableWidgetItem(fc.functionName);
        itemFunc->setData(Qt::UserRole, fc.functionName);

        auto* itemFile = new QTableWidgetItem(fc.fileName);

        auto* itemStmtTotal = new QTableWidgetItem(QString::number(fc.statementCount));
        itemStmtTotal->setTextAlignment(Qt::AlignCenter);

        auto* itemStmtCovered = new QTableWidgetItem(QString::number(fc.coveredStatements));
        itemStmtCovered->setTextAlignment(Qt::AlignCenter);

        auto* itemStmtRate = new QTableWidgetItem(QString::number(fc.statementCoveragePercent, 'f', 2));
        itemStmtRate->setTextAlignment(Qt::AlignCenter);

        auto* itemBranchTotal = new QTableWidgetItem(QString::number(fc.branchCount));
        itemBranchTotal->setTextAlignment(Qt::AlignCenter);

        auto* itemBranchCovered = new QTableWidgetItem(QString::number(fc.coveredBranches));
        itemBranchCovered->setTextAlignment(Qt::AlignCenter);

        auto* itemBranchRate = new QTableWidgetItem(QString::number(fc.branchCoveragePercent, 'f', 2));
        itemBranchRate->setTextAlignment(Qt::AlignCenter);

        // 着色高亮
        bool isLight = (m_currentTheme == ThemeType::LightModern);
        QColor okColor = isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#a6e3a1"));
        QColor warnColor = isLight ? QColor(QStringLiteral("#d97706")) : QColor(QStringLiteral("#f9e2af"));
        QColor critColor = isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8"));

        QColor stmtC = (fc.statementCoveragePercent >= 80.0) ? okColor : (fc.statementCoveragePercent >= 50.0) ? warnColor : critColor;
        itemStmtRate->setForeground(stmtC);

        QColor branchC = (fc.branchCoveragePercent >= 80.0) ? okColor : (fc.branchCoveragePercent >= 50.0) ? warnColor : critColor;
        itemBranchRate->setForeground(branchC);

        m_tableCoverage->setItem(row, 0, itemIdx);
        m_tableCoverage->setItem(row, 1, itemFunc);
        m_tableCoverage->setItem(row, 2, itemFile);
        m_tableCoverage->setItem(row, 3, itemStmtTotal);
        m_tableCoverage->setItem(row, 4, itemStmtCovered);
        m_tableCoverage->setItem(row, 5, itemStmtRate);
        m_tableCoverage->setItem(row, 6, itemBranchTotal);
        m_tableCoverage->setItem(row, 7, itemBranchCovered);
        m_tableCoverage->setItem(row, 8, itemBranchRate);

        row++;
    }
}

void CoverageView::updateKpiBadges() {
    double stmtRate = m_report.statementCoveragePercent();
    double branchRate = m_report.branchCoveragePercent();
    double funcRate = m_report.functionCoveragePercent();

    m_lblStmtBadge->setText(QString("语句覆盖: %1% (%2/%3)")
                               .arg(QString::number(stmtRate, 'f', 1))
                               .arg(m_report.coveredLines)
                               .arg(m_report.executableLines));

    m_lblBranchBadge->setText(QString("分支覆盖: %1% (%2/%3)")
                                 .arg(QString::number(branchRate, 'f', 1))
                                 .arg(m_report.coveredBranches)
                                 .arg(m_report.totalBranches));

    m_lblFuncBadge->setText(QString("函数覆盖: %1% (%2/%3)")
                               .arg(QString::number(funcRate, 'f', 1))
                               .arg(m_report.coveredFunctions)
                               .arg(m_report.totalFunctions));
}

void CoverageView::onSearchTextChanged(const QString& /* text */) {
    populateList();
}

void CoverageView::onModeRadioToggled() {
    populateList();
}

void CoverageView::onListItemClicked(QListWidgetItem* item) {
    if (!item) return;

    if (m_radioFuncMode->isChecked()) {
        QString funcName = item->data(Qt::UserRole).toString();
        m_chartWidget->setSelectedFunction(funcName);

        // 同步在表格中定位选中
        for (int r = 0; r < m_tableCoverage->rowCount(); ++r) {
            if (m_tableCoverage->item(r, 1)->text() == funcName) {
                m_tableCoverage->selectRow(r);
                m_tableCoverage->scrollToItem(m_tableCoverage->item(r, 1));
                break;
            }
        }

        // 若处于代码覆盖 Tab，跳至该函数首行
        int line = item->data(Qt::UserRole + 2).toInt();
        if (line > 0 && m_codeViewer) {
            m_codeViewer->gotoLine(line);
        }
    } else {
        // 文件模式，切换代码查看器中的文件
        QString filePath = item->data(Qt::UserRole).toString();
        int idx = m_cmbFiles->findData(filePath);
        if (idx != -1) {
            m_cmbFiles->setCurrentIndex(idx);
        }
    }
}

void CoverageView::onListItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;

    if (m_radioFileMode->isChecked()) {
        // 1. 获取选中的文件路径
        QString filePath = item->data(Qt::UserRole).toString();
        if (filePath.isEmpty()) return;

        m_activeFile = filePath;

        // 2. 加载文件内容到代码查看器
        if (!m_fileContents.contains(filePath) && QFile::exists(filePath)) {
            QFile f(filePath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                m_fileContents[filePath] = QString::fromUtf8(f.readAll());
                f.close();
            }
        }
        if (m_fileContents.contains(filePath) && m_codeViewer) {
            CoverageReport fileRep = m_report;
            QString cleanTarget = QDir::cleanPath(filePath);
            if (m_report.fileReports.contains(cleanTarget)) {
                fileRep = m_report.fileReports.value(cleanTarget);
            } else if (QDir::cleanPath(m_report.filePath) != cleanTarget) {
                Instrumenter inst;
                auto res = inst.instrumentSource(m_fileContents[filePath], filePath);
                CoverageEngine eng;
                eng.setMetadata(res.metadata);
                fileRep = eng.generateReport(m_fileContents[filePath]);
                m_report.fileReports[cleanTarget] = fileRep;
            }
            m_codeViewer->setSourceAndCoverage(m_fileContents[filePath], fileRep);
        }

        // 3. 同步更新下拉框
        m_cmbFiles->blockSignals(true);
        int idx = m_cmbFiles->findData(QDir::cleanPath(filePath));
        if (idx == -1) idx = m_cmbFiles->findData(filePath);
        if (idx != -1) {
            m_cmbFiles->setCurrentIndex(idx);
        }
        m_cmbFiles->blockSignals(false);

        // 4. 自动跳转到函数模式，并展示对应源码文件函数列表
        m_radioFuncMode->setChecked(true);
        populateList();

        // 5. 切换到代码覆盖 Tab 查看文件
        m_tabWidget->setCurrentIndex(1);
    } else {
        // 函数模式下双击：切换至 Tab 2: 代码覆盖并跳至该函数首行
        QString funcName = item->data(Qt::UserRole).toString();
        m_tabWidget->setCurrentIndex(1);
        int line = item->data(Qt::UserRole + 2).toInt();
        if (line > 0 && m_codeViewer) {
            m_codeViewer->gotoLine(line);
        }
    }
}

void CoverageView::onTableSelectionChanged() {
    int row = m_tableCoverage->currentRow();
    if (row >= 0 && row < m_tableCoverage->rowCount()) {
        QString funcName = m_tableCoverage->item(row, 1)->text();
        m_chartWidget->setSelectedFunction(funcName);
    }
}

void CoverageView::onTableDoubleClicked(int row, int /* col */) {
    if (row >= 0 && row < m_tableCoverage->rowCount()) {
        QString funcName = m_tableCoverage->item(row, 1)->text();
        // 切换至 Tab 2: 代码覆盖
        m_tabWidget->setCurrentIndex(1);

        for (const auto& fc : m_report.functionCoverages) {
            if (fc.functionName == funcName) {
                if (fc.startLine > 0 && m_codeViewer) {
                    m_codeViewer->gotoLine(fc.startLine);
                }
                break;
            }
        }
    }
}

void CoverageView::onChartFunctionClicked(const QString& funcName) {
    for (int r = 0; r < m_tableCoverage->rowCount(); ++r) {
        if (m_tableCoverage->item(r, 1)->text() == funcName) {
            m_tableCoverage->selectRow(r);
            m_tableCoverage->scrollToItem(m_tableCoverage->item(r, 1));
            break;
        }
    }
}

void CoverageView::onFileComboIndexChanged(int index) {
    if (index < 0) return;
    QString filePath = m_cmbFiles->itemData(index).toString();
    if (filePath.isEmpty()) return;

    m_activeFile = filePath;
    if (!m_fileContents.contains(filePath) && QFile::exists(filePath)) {
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_fileContents[filePath] = QString::fromUtf8(f.readAll());
            f.close();
        }
    }

    if (m_fileContents.contains(filePath)) {
        CoverageReport fileRep = m_report;
        QString cleanTarget = QDir::cleanPath(filePath);
        if (m_report.fileReports.contains(cleanTarget)) {
            fileRep = m_report.fileReports.value(cleanTarget);
        } else if (QDir::cleanPath(m_report.filePath) != cleanTarget) {
            Instrumenter inst;
            auto res = inst.instrumentSource(m_fileContents[filePath], filePath);
            CoverageEngine eng;
            eng.setMetadata(res.metadata);
            fileRep = eng.generateReport(m_fileContents[filePath]);
            m_report.fileReports[cleanTarget] = fileRep;
        }
        m_codeViewer->setSourceAndCoverage(m_fileContents[filePath], fileRep);
    }

    // 自动跳转到函数模式，并展示对应源码文件函数列表
    m_radioFuncMode->setChecked(true);
    populateList();
}

void CoverageView::applyTheme(ThemeType type) {
    m_currentTheme = type;
    m_chartWidget->applyTheme(type);
    m_codeViewer->applyTheme(type);

    bool isLight = (type == ThemeType::LightModern);
    if (isLight) {
        setStyleSheet(QStringLiteral(
            "QTableWidget { background-color: #ffffff; color: #202124; gridline-color: #e8eaed; border: 1px solid #dadce0; selection-background-color: #e8f0fe; selection-color: #1a73e8; }"
            "QHeaderView::section { background-color: #f1f3f4; color: #202124; font-weight: bold; border: 1px solid #e0e0e0; padding: 4px; }"
            "QListWidget { background-color: #ffffff; color: #202124; border: 1px solid #dadce0; }"
            "QLineEdit { background-color: #ffffff; color: #202124; border: 1px solid #dadce0; border-radius: 4px; padding: 4px; }"
            "QTabWidget::pane { border: 1px solid #dadce0; background: #ffffff; }"
            "QTabBar::tab { background: #f1f3f4; color: #5f6368; padding: 6px 16px; border: 1px solid #dadce0; margin-right: 2px; font-weight: bold; }"
            "QTabBar::tab:selected { background: #ffffff; color: #1a73e8; border-bottom: 2px solid #1a73e8; }"
        ));
    } else {
        setStyleSheet(QStringLiteral(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; border: 1px solid #313244; selection-background-color: #45475a; selection-color: #cdd6f4; }"
            "QHeaderView::section { background-color: #181825; color: #cdd6f4; font-weight: bold; border: 1px solid #313244; padding: 4px; }"
            "QListWidget { background-color: #1e1e2e; color: #cdd6f4; border: 1px solid #313244; }"
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 4px; padding: 4px; }"
            "QTabWidget::pane { border: 1px solid #313244; background: #1e1e2e; }"
            "QTabBar::tab { background: #181825; color: #a6adc8; padding: 6px 16px; border: 1px solid #313244; margin-right: 2px; font-weight: bold; }"
            "QTabBar::tab:selected { background: #1e1e2e; color: #89b4fa; border-top: 2px solid #89b4fa; }"
        ));
    }

    populateTable();
}

void CoverageView::retranslateUi() {
    bool isZh = (LocalizationManager::instance().currentLanguage() == Language::Chinese);
    m_radioFileMode->setText(isZh ? QStringLiteral("文件") : QStringLiteral("File"));
    m_radioFuncMode->setText(isZh ? QStringLiteral("函数") : QStringLiteral("Function"));
    m_tabWidget->setTabText(0, isZh ? QStringLiteral("覆盖统计") : QStringLiteral("Coverage Stats"));
    m_tabWidget->setTabText(1, isZh ? QStringLiteral("代码覆盖") : QStringLiteral("Code Coverage"));
}

} // namespace Coverage
