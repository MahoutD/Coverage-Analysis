#include "splash_screen.h"
#include "coverage/app_config.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>
#include <QFontMetrics>

namespace Coverage {

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(580, 340);

    // 屏幕居中显示
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        QRect scrGeom = screen->geometry();
        move((scrGeom.width() - width()) / 2, (scrGeom.height() - height()) / 2);
    }
}

void SplashScreen::setProgress(int percent, const QString& statusText) {
    m_progress = qBound(0, percent, 100);
    m_statusText = statusText;
    update();
}

void SplashScreen::finish(QWidget* mainWin) {
    if (mainWin) {
        mainWin->raise();
        mainWin->activateWindow();
    }
    close();
}

void SplashScreen::paintEvent(QPaintEvent* /* event */) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QRect rect = this->rect();

    // 1. 绘制外部柔和半透明大阴影与主窗口圆角底板 (Dark Modern Tech Style)
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect.adjusted(6, 6, -6, -6), 16, 16);

    // 主底色线性渐变 (Catppuccin Mocha Mantle -> Base)
    QLinearGradient bgGrad(0, 0, width(), height());
    bgGrad.setColorAt(0.0, QColor(24, 24, 37, 252));
    bgGrad.setColorAt(0.5, QColor(30, 30, 46, 252));
    bgGrad.setColorAt(1.0, QColor(17, 17, 27, 252));
    p.fillPath(bgPath, bgGrad);

    // 细致高质感边框
    p.setPen(QPen(QColor(88, 91, 112, 180), 1.5));
    p.drawPath(bgPath);

    // 2. 绘制微弱科技感背景点阵网格
    p.setPen(QColor(147, 153, 178, 25));
    for (int x = 20; x < width() - 20; x += 22) {
        for (int y = 20; y < height() - 20; y += 22) {
            p.drawPoint(x, y);
        }
    }

    // 3. 绘制专属芯片与代码覆盖率矢量 Logo 徽标
    int logoX = 40;
    int logoY = 42;
    int logoSize = 64;

    // 徽标发光渐变外框
    QPainterPath logoPath;
    logoPath.addRoundedRect(logoX, logoY, logoSize, logoSize, 14, 14);
    QLinearGradient logoGrad(logoX, logoY, logoX + logoSize, logoY + logoSize);
    logoGrad.setColorAt(0.0, QColor(137, 180, 250)); // Sky Blue
    logoGrad.setColorAt(1.0, QColor(166, 227, 161)); // Emerald Green
    p.fillPath(logoPath, logoGrad);

    // 芯片中央黑色核心
    QPainterPath corePath;
    corePath.addRoundedRect(logoX + 10, logoY + 10, logoSize - 20, logoSize - 20, 8, 8);
    p.fillPath(corePath, QColor(24, 24, 37));

    // 芯片引脚金色线条 (左右上下)
    p.setPen(QPen(QColor(249, 226, 175), 2));
    p.drawLine(logoX + 2, logoY + 22, logoX + 8, logoY + 22);
    p.drawLine(logoX + 2, logoY + 32, logoX + 8, logoY + 32);
    p.drawLine(logoX + 2, logoY + 42, logoX + 8, logoY + 42);

    p.drawLine(logoX + logoSize - 8, logoY + 22, logoX + logoSize - 2, logoY + 22);
    p.drawLine(logoX + logoSize - 8, logoY + 32, logoX + logoSize - 2, logoY + 32);
    p.drawLine(logoX + logoSize - 8, logoY + 42, logoX + logoSize - 2, logoY + 42);

    // 芯片内部绿色代码覆盖雷达脉冲小圆
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(166, 227, 161));
    p.drawEllipse(logoX + logoSize / 2 - 5, logoY + logoSize / 2 - 5, 10, 10);
    p.setPen(QPen(QColor(166, 227, 161, 160), 1.5, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(logoX + logoSize / 2 - 12, logoY + logoSize / 2 - 12, 24, 24);

    const auto& cfg = AppConfig::instance();

    // 4. 软件主副标题与版本胶囊徽章
    int textLeft = logoX + logoSize + 22;

    // 主标题
    QFont fontTitle(QStringLiteral("Segoe UI"), 16, QFont::Bold);
    p.setFont(fontTitle);
    p.setPen(QColor(245, 245, 250));
    p.drawText(textLeft, logoY + 26, cfg.productTitle);

    // 中文副标题
    QFont fontSub(QStringLiteral("Microsoft YaHei"), 11, QFont::Normal);
    p.setFont(fontSub);
    p.setPen(QColor(186, 194, 222));
    p.drawText(textLeft, logoY + 52, cfg.productSubtitle);

    // 版本徽章 (自适应宽度)
    QFont fontBadge(QStringLiteral("Segoe UI"), 8, QFont::Bold);
    QFontMetrics fmBadge(fontBadge);
    int badgeTextW = fmBadge.horizontalAdvance(cfg.productVersion);
    int badgeW = qMax(78, badgeTextW + 16);
    int badgeX = textLeft + 250;
    if (badgeX + badgeW > width() - 25) {
        badgeX = width() - 25 - badgeW;
    }
    int badgeY = logoY + 37;
    QPainterPath badgePath;
    badgePath.addRoundedRect(badgeX, badgeY, badgeW, 20, 10, 10);
    p.fillPath(badgePath, QColor(49, 50, 68));
    p.setPen(QPen(QColor(137, 180, 250), 1));
    p.drawPath(badgePath);

    p.setFont(fontBadge);
    p.setPen(QColor(166, 227, 161));
    p.drawText(QRect(badgeX, badgeY, badgeW, 20), Qt::AlignCenter, cfg.productVersion);

    // 装饰性渐变分隔线
    QLinearGradient lineGrad(36, 130, width() - 36, 130);
    lineGrad.setColorAt(0.0, QColor(137, 180, 250, 120));
    lineGrad.setColorAt(0.5, QColor(166, 227, 161, 180));
    lineGrad.setColorAt(1.0, QColor(203, 166, 247, 80));
    p.setPen(QPen(lineGrad, 1.5));
    p.drawLine(36, 130, width() - 36, 130);

    // 5. 核心特性芯片卡片小图标栏 (根据配置文件动态读取)
    QFont fontCard(QStringLiteral("Microsoft YaHei"), 9);
    p.setFont(fontCard);
    int colW = 160;
    int cardY = 155;

    auto drawFeatureBadge = [&](int x, const QString& title, const QString& desc, const QColor& color) {
        QPainterPath cp;
        cp.addRoundedRect(x, cardY, colW, 46, 8, 8);
        p.fillPath(cp, QColor(30, 30, 46, 180));
        p.setPen(QPen(QColor(69, 71, 90), 1));
        p.drawPath(cp);

        // 小彩条指示
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(x + 8, cardY + 12, 4, 22, 2, 2);

        p.setPen(QColor(235, 237, 245));
        p.drawText(x + 18, cardY + 22, title);
        p.setPen(QColor(166, 173, 200));
        QFont fSmall(QStringLiteral("Segoe UI"), 8);
        p.setFont(fSmall);
        p.drawText(x + 18, cardY + 38, desc);
        p.setFont(fontCard);
    };

    drawFeatureBadge(36, cfg.card1Title, cfg.card1Desc, QColor(cfg.card1Color));
    drawFeatureBadge(36 + colW + 14, cfg.card2Title, cfg.card2Desc, QColor(cfg.card2Color));
    drawFeatureBadge(36 + (colW + 14) * 2, cfg.card3Title, cfg.card3Desc, QColor(cfg.card3Color));

    // 6. 底部进度条与状态文案
    int pbX = 36;
    int pbY = 236;
    int pbW = width() - 72;
    int pbH = 10;

    // 进度条背景槽
    QPainterPath pbBg;
    pbBg.addRoundedRect(pbX, pbY, pbW, pbH, 5, 5);
    p.fillPath(pbBg, QColor(49, 50, 68));

    // 进度条活动填充 (翡翠绿到天空蓝霓虹渐变)
    if (m_progress > 0) {
        int fillW = qMax(10, pbW * m_progress / 100);
        QPainterPath pbFill;
        pbFill.addRoundedRect(pbX, pbY, fillW, pbH, 5, 5);

        QLinearGradient pGrad(pbX, pbY, pbX + fillW, pbY);
        pGrad.setColorAt(0.0, QColor(137, 180, 250));
        pGrad.setColorAt(1.0, QColor(166, 227, 161));
        p.fillPath(pbFill, pGrad);
    }

    // 状态文字显示
    QFont fontStatus(QStringLiteral("Microsoft YaHei"), 9);
    p.setFont(fontStatus);
    p.setPen(QColor(205, 214, 244));
    QString statusText = m_statusText.isEmpty() ? QStringLiteral("正在初始化系统组件...") : m_statusText;
    p.drawText(pbX, pbY + 28, statusText);

    // 进度百分比
    QFont fontPct(QStringLiteral("Segoe UI"), 10, QFont::Bold);
    p.setFont(fontPct);
    p.setPen(QColor(166, 227, 161));
    p.drawText(QRect(pbX, pbY + 12, pbW, 24), Qt::AlignRight | Qt::AlignVCenter, QString("%1%").arg(m_progress));

    // 7. 底部版权与架构提示 (配置化)
    QFont fontFoot(QStringLiteral("Segoe UI"), 8);
    p.setFont(fontFoot);
    p.setPen(QColor(108, 112, 134));
    p.drawText(QRect(0, height() - 32, width(), 20), Qt::AlignCenter, cfg.productFooter);
}

} // namespace Coverage
