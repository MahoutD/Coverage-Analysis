#pragma once

#include <QWidget>
#include <QString>
#include <QPainter>
#include <QTimer>

namespace Coverage {

/**
 * @brief 使用 QPainter 纯手工绘制的现代化高质感加载界面 (Splash Screen)
 * 具备平滑抗锯齿微弧矩形、芯片拓扑发光徽标、渐变进度条及动态就绪状态显示。
 */
class SplashScreen : public QWidget {
    Q_OBJECT
public:
    explicit SplashScreen(QWidget* parent = nullptr);
    ~SplashScreen() override = default;

    void setProgress(int percent, const QString& statusText);
    int progress() const { return m_progress; }
    QString statusText() const { return m_statusText; }

    void finish(QWidget* mainWin);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_progress = 0;
    QString m_statusText;
};

} // namespace Coverage
