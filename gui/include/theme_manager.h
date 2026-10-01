#pragma once

#include <QString>
#include <QApplication>
#include <QObject>

namespace Coverage {

enum class ThemeType {
    DarkCatppuccin, ///< 深色科技风 (Catppuccin Mocha)
    LightModern,    ///< 现代清新白 (Modern Light)
    SolarizedDark,  ///< Solarized 暗夜 (Solarized Dark)
    MonokaiPro      ///< Monokai Pro 专业暗色 (Monokai Pro)
};

/**
 * @brief 界面主题样式与配色管理器
 * 提供多种现代风格的界面主题，支持运行时动态无缝切换，并适配代码编辑器、图表与控件样式。
 */
class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    static void applyTheme(ThemeType type);
    static ThemeType currentTheme();
    static ThemeType detectSystemTheme();
    static QString themeName(ThemeType type);
    static QString getEditorStyleSheet();
    static QString getEditorStyleSheet(ThemeType type);
    static QString getGraphBgColor(ThemeType type);

signals:
    void themeChanged(ThemeType type);

private:
    ThemeManager() = default;
    ThemeType m_currentTheme = ThemeType::DarkCatppuccin;
};

} // namespace Coverage
