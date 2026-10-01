#include "main_window.h"
#include "theme_manager.h"
#include "splash_screen.h"
#include "coverage/backend_service.h"
#include "coverage/logger.h"
#include "coverage/app_config.h"
#include "coverage/project.h"

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QThread>
#include <QElapsedTimer>

int main(int argc, char *argv[]) {
    // High DPI scaling
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("EmbeddedCoverageStudio"));
    app.setApplicationVersion(QStringLiteral("1.5.0"));
    app.setOrganizationName(QStringLiteral("EmbeddedDevTools"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.svg")));

    // 0. 读取应用程序与欢迎界面配置
    auto& cfg = Coverage::AppConfig::instance();
    cfg.load();

    // 1. 初始化系统运行日志记录器
    Coverage::Logger::instance().init();
    Coverage::Logger::instance().installQtMessageHandler();
    Coverage::Logger::instance().log(Coverage::LogLevel::Info, QStringLiteral("App"),
        QStringLiteral("嵌入式 C 覆盖分析与静态分析套件启动中..."));

    // 2. 弹出使用 QPainter 绘制的高科技质感加载启动界面 (配置化展示时长与内容, Requirement 1)
    Coverage::SplashScreen splash;
    splash.show();
    app.processEvents();

    QElapsedTimer timer;
    timer.start();

    int totalDuration = qMax(800, cfg.splashMinTimeMs);

    auto updateSplash = [&](int targetPct, const QString& status) {
        qint64 elapsed = timer.elapsed();
        int expectedTime = (targetPct * totalDuration) / 100;
        if (elapsed < expectedTime) {
            int toWait = expectedTime - elapsed;
            int step = 20;
            while (toWait > 0) {
                int ms = qMin(step, toWait);
                QThread::msleep(ms);
                app.processEvents();
                toWait -= ms;
            }
        }
        splash.setProgress(targetPct, status);
        app.processEvents();
    };

    updateSplash(15, QStringLiteral("正在初始化底层环境与配置参数 (config/app_config.ini)..."));

    // 3. 根据操作系统明暗外观模式自动选择初始主题 (Requirement 8)
    Coverage::ThemeType initialTheme = Coverage::ThemeManager::detectSystemTheme();
    Coverage::ThemeManager::applyTheme(initialTheme);
    updateSplash(35, QStringLiteral("正在载入现代 UI 主题、字体与矢量图元体系..."));

    // 4. 软件默认启动时保持未打开状态 (Requirement 1: 树为空，不预载数据，仅欢迎页与日志可见)
    updateSplash(60, QStringLiteral("正在初始化工作台与工程就绪环境..."));

    // 5. 构建主工作台窗口 (包含 Visual Studio 式自由停靠与 Graphviz 引擎)
    Coverage::MainWindow window;
    updateSplash(85, QStringLiteral("正在构建多文件选项卡编辑器与 Graphviz 矢量图元引擎..."));

    updateSplash(100, QStringLiteral("系统就绪，正在呈现主工作台..."));

    // 补齐最少停留时间以确保用户视觉体验平滑自然
    while (timer.elapsed() < totalDuration) {
        QThread::msleep(30);
        app.processEvents();
    }

    window.show();
    splash.finish(&window);

    return app.exec();
}
