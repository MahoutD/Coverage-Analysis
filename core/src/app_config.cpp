#include "coverage/app_config.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

namespace Coverage {

AppConfig& AppConfig::instance() {
    static AppConfig s_instance;
    return s_instance;
}

AppConfig::AppConfig() {
    load();
}

void AppConfig::load(const QString& configPath) {
    if (!configPath.isEmpty()) {
        m_filePath = configPath;
    } else {
        // 查找配置文件候选路径
        const QStringList candidates = {
            QDir::currentPath() + QStringLiteral("/config/app_config.ini"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/config/app_config.ini"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/../../config/app_config.ini"),
            QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis/config/app_config.ini")
        };

        for (const QString& p : candidates) {
            if (QFile::exists(p)) {
                m_filePath = QDir::toNativeSeparators(p);
                break;
            }
        }

        if (m_filePath.isEmpty()) {
            m_filePath = QDir::toNativeSeparators(QDir::currentPath() + QStringLiteral("/config/app_config.ini"));
        }
    }

    if (!QFile::exists(m_filePath)) {
        // 文件不存在则立即根据默认值保存一份，方便用户用记事本随时编辑修改
        save(m_filePath);
        return;
    }

    QSettings settings(m_filePath, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    settings.setIniCodec("UTF-8");
#endif

    // [Product]
    settings.beginGroup(QStringLiteral("Product"));
    productTitle = settings.value(QStringLiteral("Title"), productTitle).toString();
    productSubtitle = settings.value(QStringLiteral("Subtitle"), productSubtitle).toString();
    productVersion = settings.value(QStringLiteral("Version"), productVersion).toString();
    productFooter = settings.value(QStringLiteral("Footer"), productFooter).toString();
    settings.endGroup();

    // [SplashScreen]
    settings.beginGroup(QStringLiteral("SplashScreen"));
    splashMinTimeMs = settings.value(QStringLiteral("MinDisplayTimeMs"), splashMinTimeMs).toInt();
    if (splashMinTimeMs < 500) splashMinTimeMs = 500;

    card1Title = settings.value(QStringLiteral("Card1_Title"), card1Title).toString();
    card1Desc = settings.value(QStringLiteral("Card1_Desc"), card1Desc).toString();
    card1Color = settings.value(QStringLiteral("Card1_Color"), card1Color).toString();

    card2Title = settings.value(QStringLiteral("Card2_Title"), card2Title).toString();
    card2Desc = settings.value(QStringLiteral("Card2_Desc"), card2Desc).toString();
    card2Color = settings.value(QStringLiteral("Card2_Color"), card2Color).toString();

    card3Title = settings.value(QStringLiteral("Card3_Title"), card3Title).toString();
    card3Desc = settings.value(QStringLiteral("Card3_Desc"), card3Desc).toString();
    card3Color = settings.value(QStringLiteral("Card3_Color"), card3Color).toString();
    settings.endGroup();

    // [Project]
    settings.beginGroup(QStringLiteral("Project"));
    autoOpenRecentProject = settings.value(QStringLiteral("AutoOpenRecentProject"), autoOpenRecentProject).toBool();
    lastProjectPath = settings.value(QStringLiteral("LastProjectPath"), lastProjectPath).toString();
    settings.endGroup();
}

void AppConfig::save(const QString& configPath) {
    if (!configPath.isEmpty()) {
        m_filePath = configPath;
    }

    QFileInfo fi(m_filePath);
    QDir().mkpath(fi.absolutePath());

    QSettings settings(m_filePath, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    settings.setIniCodec("UTF-8");
#endif

    // [Product]
    settings.beginGroup(QStringLiteral("Product"));
    settings.setValue(QStringLiteral("Title"), productTitle);
    settings.setValue(QStringLiteral("Subtitle"), productSubtitle);
    settings.setValue(QStringLiteral("Version"), productVersion);
    settings.setValue(QStringLiteral("Footer"), productFooter);
    settings.endGroup();

    // [SplashScreen]
    settings.beginGroup(QStringLiteral("SplashScreen"));
    settings.setValue(QStringLiteral("MinDisplayTimeMs"), splashMinTimeMs);
    settings.setValue(QStringLiteral("Card1_Title"), card1Title);
    settings.setValue(QStringLiteral("Card1_Desc"), card1Desc);
    settings.setValue(QStringLiteral("Card1_Color"), card1Color);

    settings.setValue(QStringLiteral("Card2_Title"), card2Title);
    settings.setValue(QStringLiteral("Card2_Desc"), card2Desc);
    settings.setValue(QStringLiteral("Card2_Color"), card2Color);

    settings.setValue(QStringLiteral("Card3_Title"), card3Title);
    settings.setValue(QStringLiteral("Card3_Desc"), card3Desc);
    settings.setValue(QStringLiteral("Card3_Color"), card3Color);
    settings.endGroup();

    // [Project]
    settings.beginGroup(QStringLiteral("Project"));
    settings.setValue(QStringLiteral("AutoOpenRecentProject"), autoOpenRecentProject);
    settings.setValue(QStringLiteral("LastProjectPath"), lastProjectPath);
    settings.endGroup();

    settings.sync();
}

} // namespace Coverage
