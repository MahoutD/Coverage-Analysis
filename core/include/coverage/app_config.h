#pragma once

#include <QString>

namespace Coverage {

/**
 * @brief 应用程序与启动欢迎界面配置管理器
 * 读取并持久化 config/app_config.ini 文件，
 * 支持动态定制产品主副标题、版本徽章、特性卡片文案与颜色、加载动画时长及工程自启动偏好。
 */
class AppConfig {
public:
    static AppConfig& instance();

    void load(const QString& configPath = QString());
    void save(const QString& configPath = QString());

    // 产品品牌信息
    QString productTitle = QStringLiteral("Embedded C Coverage Studio");
    QString productSubtitle = QStringLiteral("嵌入式 C 覆盖分析与静态分析套件");
    QString productVersion = QStringLiteral("v1.5.0 专业版");
    QString productFooter = QStringLiteral("基于 Qt 6.8 架构 • 严格前后端解耦 • 完全免外部运行时环境");

    // 启动欢迎画面配置
    int splashMinTimeMs = 2500; // 启动画面持续展示时长 (毫秒)，可配置

    QString card1Title = QStringLiteral("MISRA-C 检测");
    QString card1Desc = QStringLiteral("C++17 纯算法内核");
    QString card1Color = QStringLiteral("#f38ba8");

    QString card2Title = QStringLiteral("Graphviz 原生图元");
    QString card2Desc = QStringLiteral("零失真无级矢量拓扑");
    QString card2Color = QStringLiteral("#89b4fa");

    QString card3Title = QStringLiteral("路径探索与着色");
    QString card3Desc = QStringLiteral("独立基路径联动渲染");
    QString card3Color = QStringLiteral("#a6e3a1");

    // 工程相关配置
    bool autoOpenRecentProject = true;
    QString lastProjectPath;

    QString currentConfigFilePath() const { return m_filePath; }

private:
    AppConfig();
    QString m_filePath;
};

} // namespace Coverage
