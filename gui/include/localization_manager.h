#pragma once

#include <QString>
#include <QMap>
#include <QObject>

namespace Coverage {

enum class Language {
    Chinese,    ///< 简体中文 (默认)
    English     ///< English
};

/**
 * @brief 全局多语言本地化管理器
 * 统一管理界面多语言文本翻译，默认为简体中文，支持在运行时无重启即时切换。
 */
class LocalizationManager : public QObject {
    Q_OBJECT
public:
    static LocalizationManager& instance();

    Language currentLanguage() const { return m_currentLanguage; }
    void setLanguage(Language lang);

    /**
     * @brief 翻译指定键值的文本
     */
    QString text(const QString& key) const;

signals:
    void languageChanged(Language lang);

private:
    LocalizationManager();
    void initDictionary();

    Language m_currentLanguage = Language::Chinese;
    QMap<QString, QString> m_dictZh;
    QMap<QString, QString> m_dictEn;
};

// 快捷翻译辅助宏
inline QString trText(const QString& key) {
    return LocalizationManager::instance().text(key);
}

} // namespace Coverage
