#include "theme_manager.h"
#include <QPalette>
#include <QFont>
#include <QColor>
#include <QSettings>
#include <QStyleHints>
#include <QGuiApplication>

namespace Coverage {

ThemeManager& ThemeManager::instance() {
    static ThemeManager s_instance;
    return s_instance;
}

ThemeType ThemeManager::currentTheme() {
    return instance().m_currentTheme;
}

ThemeType ThemeManager::detectSystemTheme() {
#ifdef Q_OS_WIN
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
                       QSettings::NativeFormat);
    QVariant val = settings.value(QStringLiteral("AppsUseLightTheme"));
    if (val.isValid()) {
        return (val.toInt() == 1) ? ThemeType::LightModern : ThemeType::DarkCatppuccin;
    }
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (QGuiApplication::styleHints() && QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light) {
        return ThemeType::LightModern;
    }
#endif
    return ThemeType::DarkCatppuccin;
}

QString ThemeManager::themeName(ThemeType type) {
    switch (type) {
    case ThemeType::DarkCatppuccin:
        return QStringLiteral("深色科技风 (Catppuccin)");
    case ThemeType::LightModern:
        return QStringLiteral("现代清新白 (Modern Light)");
    case ThemeType::SolarizedDark:
        return QStringLiteral("Solarized 暗夜 (Solarized Dark)");
    case ThemeType::MonokaiPro:
        return QStringLiteral("Monokai Pro 专业暗色 (Monokai Pro)");
    }
    return QStringLiteral("默认深色");
}

static QString getCatppuccinStyle() {
    return QStringLiteral(
R"(
* {
    outline: none;
}
*:focus {
    outline: none;
}
QPushButton:focus, QToolButton:focus, QTabBar::tab:focus, QTreeView:focus, QTreeWidget:focus, QListView:focus, QTableView:focus {
    outline: none;
}
QMainWindow, QDialog {
    background-color: #1e1e2e;
    color: #cdd6f4;
}
QWidget {
    background-color: #1e1e2e;
    color: #cdd6f4;
    selection-background-color: #45475a;
    selection-color: #cdd6f4;
}
QMenuBar {
    background-color: #181825;
    color: #cdd6f4;
    border-bottom: 1px solid #313244;
}
QMenuBar::item {
    background: transparent;
    padding: 6px 12px;
}
QMenuBar::item:selected {
    background: #313244;
    border-radius: 4px;
}
QMenu {
    background-color: #181825;
    color: #cdd6f4;
    border: 1px solid #45475a;
    border-radius: 6px;
    padding: 4px;
}
QMenu::item {
    padding: 6px 24px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #313244;
}
QToolBar {
    background-color: #181825;
    border-bottom: 1px solid #313244;
    padding: 2px 4px;
    spacing: 3px;
}
QToolBar::separator {
    width: 1px;
    margin: 3px 4px;
    background-color: #313244;
}
QToolButton {
    background-color: transparent;
    color: #cdd6f4;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 3px 5px;
    margin: 0px 1px;
    font-weight: 500;
}
QToolButton:hover {
    background-color: #313244;
    border: 1px solid #45475a;
}
QToolButton:pressed {
    background-color: #45475a;
}
QStatusBar {
    background-color: #181825;
    color: #a6adc8;
    border-top: 1px solid #313244;
}
QDockWidget {
    color: #cdd6f4;
    font-weight: bold;
}
QDockWidget::title {
    background: #181825;
    padding: 6px;
    border: 1px solid #313244;
    border-radius: 4px;
}
QTabWidget::pane {
    border: 1px solid #313244;
    background: #1e1e2e;
    border-radius: 4px;
}
QTabBar::tab {
    background: #181825;
    color: #a6adc8;
    padding: 8px 16px;
    border: 1px solid #313244;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 2px;
}
QTabBar::tab:selected {
    background: #1e1e2e;
    color: #89b4fa;
    border-bottom: 2px solid #89b4fa;
    font-weight: bold;
}
QTabBar::tab:hover {
    background: #313244;
}
QHeaderView::section {
    background-color: #181825;
    color: #89b4fa;
    padding: 6px;
    border: 1px solid #313244;
    font-weight: bold;
}
QTreeWidget, QTableWidget, QListView, QTextEdit, QPlainTextEdit, QLineEdit, QComboBox {
    background-color: #11111b;
    color: #cdd6f4;
    border: 1px solid #313244;
    border-radius: 6px;
    padding: 4px;
}
QComboBox QAbstractItemView {
    background-color: #181825;
    color: #cdd6f4;
    selection-background-color: #313244;
    border: 1px solid #45475a;
}
QTreeWidget::item:selected, QTableWidget::item:selected {
    background-color: #313244;
    color: #89b4fa;
}
QPushButton {
    background-color: #313244;
    color: #cdd6f4;
    border: 1px solid #45475a;
    border-radius: 6px;
    padding: 6px 14px;
    font-weight: 600;
}
QPushButton:hover {
    background-color: #45475a;
    border: 1px solid #585b70;
}
QPushButton:pressed {
    background-color: #585b70;
}
QPushButton#btnPrimary {
    background-color: #89b4fa;
    color: #11111b;
    border: none;
}
QPushButton#btnPrimary:hover {
    background-color: #b4befe;
}
QProgressBar {
    background-color: #181825;
    border: 1px solid #313244;
    border-radius: 4px;
    text-align: center;
    color: #cdd6f4;
    font-weight: bold;
}
QProgressBar::chunk {
    background-color: #a6e3a1;
    border-radius: 3px;
}
QScrollBar:vertical {
    background: #181825;
    width: 12px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background: #45475a;
    min-height: 20px;
    border-radius: 6px;
}
QScrollBar::handle:vertical:hover {
    background: #585b70;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}
)");
}

static QString getLightModernStyle() {
    return QStringLiteral(
R"(
* {
    outline: none;
}
*:focus {
    outline: none;
}
QPushButton:focus, QToolButton:focus, QTabBar::tab:focus, QTreeView:focus, QTreeWidget:focus, QListView:focus, QTableView:focus {
    outline: none;
}
QMainWindow, QDialog {
    background-color: #f8f9fa;
    color: #212529;
}
QWidget {
    background-color: #f8f9fa;
    color: #212529;
    selection-background-color: #d0e2ff;
    selection-color: #0f62fe;
}
QMenuBar {
    background-color: #ffffff;
    color: #212529;
    border-bottom: 1px solid #e0e0e0;
}
QMenuBar::item {
    background: transparent;
    padding: 6px 12px;
}
QMenuBar::item:selected {
    background: #e8eaed;
    border-radius: 4px;
}
QMenu {
    background-color: #ffffff;
    color: #212529;
    border: 1px solid #dadce0;
    border-radius: 6px;
    padding: 4px;
}
QMenu::item {
    padding: 6px 24px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #e8f0fe;
    color: #1a73e8;
}
QToolBar {
    background-color: #ffffff;
    border-bottom: 1px solid #e0e0e0;
    padding: 2px 4px;
    spacing: 3px;
}
QToolBar::separator {
    width: 1px;
    margin: 3px 4px;
    background-color: #dadce0;
}
QToolButton {
    background-color: transparent;
    color: #3c4043;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 3px 5px;
    margin: 0px 1px;
    font-weight: 500;
}
QToolButton:hover {
    background-color: #f1f3f4;
    border: 1px solid #dadce0;
}
QToolButton:pressed {
    background-color: #e8eaed;
}
QStatusBar {
    background-color: #ffffff;
    color: #5f6368;
    border-top: 1px solid #e0e0e0;
}
QDockWidget {
    color: #202124;
    font-weight: bold;
}
QDockWidget::title {
    background: #f1f3f4;
    padding: 6px;
    border: 1px solid #dadce0;
    border-radius: 4px;
}
QTabWidget::pane {
    border: 1px solid #dadce0;
    background: #ffffff;
    border-radius: 4px;
}
QTabBar::tab {
    background: #f1f3f4;
    color: #5f6368;
    padding: 8px 16px;
    border: 1px solid #dadce0;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 2px;
}
QTabBar::tab:selected {
    background: #ffffff;
    color: #1a73e8;
    border-bottom: 2px solid #1a73e8;
    font-weight: bold;
}
QTabBar::tab:hover {
    background: #e8eaed;
}
QHeaderView::section {
    background-color: #f1f3f4;
    color: #1a73e8;
    padding: 6px;
    border: 1px solid #dadce0;
    font-weight: bold;
}
QTreeWidget, QTableWidget, QListView, QTextEdit, QPlainTextEdit, QLineEdit, QComboBox {
    background-color: #ffffff;
    color: #202124;
    border: 1px solid #dadce0;
    border-radius: 6px;
    padding: 4px;
}
QComboBox QAbstractItemView {
    background-color: #ffffff;
    color: #202124;
    selection-background-color: #e8f0fe;
    selection-color: #1a73e8;
    border: 1px solid #dadce0;
}
QTreeWidget::item:selected, QTableWidget::item:selected {
    background-color: #e8f0fe;
    color: #1a73e8;
}
QPushButton {
    background-color: #f1f3f4;
    color: #202124;
    border: 1px solid #dadce0;
    border-radius: 6px;
    padding: 6px 14px;
    font-weight: 600;
}
QPushButton:hover {
    background-color: #e8eaed;
    border: 1px solid #bdc1c6;
}
QPushButton:pressed {
    background-color: #dadce0;
}
QPushButton#btnPrimary {
    background-color: #1a73e8;
    color: #ffffff;
    border: none;
}
QPushButton#btnPrimary:hover {
    background-color: #174ea6;
}
QProgressBar {
    background-color: #f1f3f4;
    border: 1px solid #dadce0;
    border-radius: 4px;
    text-align: center;
    color: #202124;
    font-weight: bold;
}
QProgressBar::chunk {
    background-color: #34a853;
    border-radius: 3px;
}
QScrollBar:vertical {
    background: #f1f3f4;
    width: 12px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background: #dadce0;
    min-height: 20px;
    border-radius: 6px;
}
QScrollBar::handle:vertical:hover {
    background: #bdc1c6;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}
)");
}

static QString getSolarizedDarkStyle() {
    return QStringLiteral(
R"(
* {
    outline: none;
}
*:focus {
    outline: none;
}
QPushButton:focus, QToolButton:focus, QTabBar::tab:focus, QTreeView:focus, QTreeWidget:focus, QListView:focus, QTableView:focus {
    outline: none;
}
QMainWindow, QDialog {
    background-color: #002b36;
    color: #839496;
}
QWidget {
    background-color: #002b36;
    color: #839496;
    selection-background-color: #073642;
    selection-color: #2aa198;
}
QMenuBar {
    background-color: #073642;
    color: #93a1a1;
    border-bottom: 1px solid #00212b;
}
QMenuBar::item {
    background: transparent;
    padding: 6px 12px;
}
QMenuBar::item:selected {
    background: #00212b;
    border-radius: 4px;
}
QMenu {
    background-color: #073642;
    color: #93a1a1;
    border: 1px solid #586e75;
    border-radius: 6px;
    padding: 4px;
}
QMenu::item {
    padding: 6px 24px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #00212b;
    color: #2aa198;
}
QToolBar {
    background-color: #073642;
    border-bottom: 1px solid #00212b;
    padding: 2px 4px;
    spacing: 3px;
}
QToolBar::separator {
    width: 1px;
    margin: 3px 4px;
    background-color: #586e75;
}
QToolButton {
    background-color: transparent;
    color: #93a1a1;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 3px 5px;
    margin: 0px 1px;
    font-weight: 500;
}
QToolButton:hover {
    background-color: #00212b;
    border: 1px solid #586e75;
}
QStatusBar {
    background-color: #073642;
    color: #586e75;
    border-top: 1px solid #00212b;
}
QDockWidget {
    color: #93a1a1;
    font-weight: bold;
}
QDockWidget::title {
    background: #073642;
    padding: 6px;
    border: 1px solid #586e75;
    border-radius: 4px;
}
QTabWidget::pane {
    border: 1px solid #073642;
    background: #002b36;
    border-radius: 4px;
}
QTabBar::tab {
    background: #073642;
    color: #657b83;
    padding: 8px 16px;
    border: 1px solid #00212b;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 2px;
}
QTabBar::tab:selected {
    background: #002b36;
    color: #2aa198;
    border-bottom: 2px solid #2aa198;
    font-weight: bold;
}
QTabBar::tab:hover {
    background: #00212b;
}
QHeaderView::section {
    background-color: #073642;
    color: #2aa198;
    padding: 6px;
    border: 1px solid #00212b;
    font-weight: bold;
}
QTreeWidget, QTableWidget, QListView, QTextEdit, QPlainTextEdit, QLineEdit, QComboBox {
    background-color: #00212b;
    color: #93a1a1;
    border: 1px solid #073642;
    border-radius: 6px;
    padding: 4px;
}
QComboBox QAbstractItemView {
    background-color: #073642;
    color: #93a1a1;
    selection-background-color: #00212b;
    selection-color: #2aa198;
    border: 1px solid #586e75;
}
QTreeWidget::item:selected, QTableWidget::item:selected {
    background-color: #073642;
    color: #2aa198;
}
QPushButton {
    background-color: #073642;
    color: #93a1a1;
    border: 1px solid #586e75;
    border-radius: 6px;
    padding: 6px 14px;
    font-weight: 600;
}
QPushButton:hover {
    background-color: #00212b;
    color: #2aa198;
}
QPushButton#btnPrimary {
    background-color: #268bd2;
    color: #002b36;
    border: none;
}
QPushButton#btnPrimary:hover {
    background-color: #2aa198;
}
QProgressBar {
    background-color: #073642;
    border: 1px solid #586e75;
    border-radius: 4px;
    text-align: center;
    color: #93a1a1;
    font-weight: bold;
}
QProgressBar::chunk {
    background-color: #859900;
    border-radius: 3px;
}
QScrollBar:vertical {
    background: #073642;
    width: 12px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background: #586e75;
    min-height: 20px;
    border-radius: 6px;
}
QScrollBar::handle:vertical:hover {
    background: #657b83;
}
)");
}

static QString getMonokaiProStyle() {
    return QStringLiteral(
R"(
* {
    outline: none;
}
*:focus {
    outline: none;
}
QPushButton:focus, QToolButton:focus, QTabBar::tab:focus, QTreeView:focus, QTreeWidget:focus, QListView:focus, QTableView:focus {
    outline: none;
}
QMainWindow, QDialog {
    background-color: #2d2a2e;
    color: #fcfcfa;
}
QWidget {
    background-color: #2d2a2e;
    color: #fcfcfa;
    selection-background-color: #403e41;
    selection-color: #ffd866;
}
QMenuBar {
    background-color: #221f22;
    color: #fcfcfa;
    border-bottom: 1px solid #403e41;
}
QMenuBar::item {
    background: transparent;
    padding: 6px 12px;
}
QMenuBar::item:selected {
    background: #403e41;
    border-radius: 4px;
}
QMenu {
    background-color: #221f22;
    color: #fcfcfa;
    border: 1px solid #403e41;
    border-radius: 6px;
    padding: 4px;
}
QMenu::item {
    padding: 6px 24px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #403e41;
    color: #ffd866;
}
QToolBar {
    background-color: #221f22;
    border-bottom: 1px solid #403e41;
    padding: 2px 4px;
    spacing: 3px;
}
QToolBar::separator {
    width: 1px;
    margin: 3px 4px;
    background-color: #403e41;
}
QToolButton {
    background-color: transparent;
    color: #fcfcfa;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 3px 5px;
    margin: 0px 1px;
    font-weight: 500;
}
QToolButton:hover {
    background-color: #403e41;
}
QStatusBar {
    background-color: #221f22;
    color: #939293;
    border-top: 1px solid #403e41;
}
QDockWidget {
    color: #fcfcfa;
    font-weight: bold;
}
QDockWidget::title {
    background: #221f22;
    padding: 6px;
    border: 1px solid #403e41;
    border-radius: 4px;
}
QTabWidget::pane {
    border: 1px solid #403e41;
    background: #2d2a2e;
    border-radius: 4px;
}
QTabBar::tab {
    background: #221f22;
    color: #939293;
    padding: 8px 16px;
    border: 1px solid #403e41;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 2px;
}
QTabBar::tab:selected {
    background: #2d2a2e;
    color: #ffd866;
    border-bottom: 2px solid #ffd866;
    font-weight: bold;
}
QTabBar::tab:hover {
    background: #403e41;
}
QHeaderView::section {
    background-color: #221f22;
    color: #ffd866;
    padding: 6px;
    border: 1px solid #403e41;
    font-weight: bold;
}
QTreeWidget, QTableWidget, QListView, QTextEdit, QPlainTextEdit, QLineEdit, QComboBox {
    background-color: #19181a;
    color: #fcfcfa;
    border: 1px solid #403e41;
    border-radius: 6px;
    padding: 4px;
}
QComboBox QAbstractItemView {
    background-color: #221f22;
    color: #fcfcfa;
    selection-background-color: #403e41;
    selection-color: #ffd866;
    border: 1px solid #403e41;
}
QTreeWidget::item:selected, QTableWidget::item:selected {
    background-color: #403e41;
    color: #ffd866;
}
QPushButton {
    background-color: #403e41;
    color: #fcfcfa;
    border: 1px solid #727072;
    border-radius: 6px;
    padding: 6px 14px;
    font-weight: 600;
}
QPushButton:hover {
    background-color: #5b595c;
}
QPushButton#btnPrimary {
    background-color: #ffd866;
    color: #2d2a2e;
    border: none;
}
QPushButton#btnPrimary:hover {
    background-color: #ffe082;
}
QProgressBar {
    background-color: #221f22;
    border: 1px solid #403e41;
    border-radius: 4px;
    text-align: center;
    color: #fcfcfa;
    font-weight: bold;
}
QProgressBar::chunk {
    background-color: #a9dc76;
    border-radius: 3px;
}
QScrollBar:vertical {
    background: #221f22;
    width: 12px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background: #403e41;
    min-height: 20px;
    border-radius: 6px;
}
QScrollBar::handle:vertical:hover {
    background: #5b595c;
}
)");
}

void ThemeManager::applyTheme(ThemeType type) {
    instance().m_currentTheme = type;

    auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;

    QFont font(QStringLiteral("Segoe UI"), 9);
    app->setFont(font);

    QString qss;
    switch (type) {
    case ThemeType::DarkCatppuccin:
        qss = getCatppuccinStyle();
        break;
    case ThemeType::LightModern:
        qss = getLightModernStyle();
        break;
    case ThemeType::SolarizedDark:
        qss = getSolarizedDarkStyle();
        break;
    case ThemeType::MonokaiPro:
        qss = getMonokaiProStyle();
        break;
    }

    app->setStyleSheet(qss);
    emit instance().themeChanged(type);
}

QString ThemeManager::getEditorStyleSheet() {
    return getEditorStyleSheet(currentTheme());
}

QString ThemeManager::getEditorStyleSheet(ThemeType type) {
    switch (type) {
    case ThemeType::DarkCatppuccin:
        return QStringLiteral(
            "background-color: #11111b;"
            "color: #cdd6f4;"
            "border: none;"
            "font-family: 'Consolas', 'Courier New', monospace;"
            "font-size: 13px;"
        );
    case ThemeType::LightModern:
        return QStringLiteral(
            "background-color: #ffffff;"
            "color: #1f2328;"
            "border: none;"
            "font-family: 'Consolas', 'Courier New', monospace;"
            "font-size: 13px;"
        );
    case ThemeType::SolarizedDark:
        return QStringLiteral(
            "background-color: #00212b;"
            "color: #93a1a1;"
            "border: none;"
            "font-family: 'Consolas', 'Courier New', monospace;"
            "font-size: 13px;"
        );
    case ThemeType::MonokaiPro:
        return QStringLiteral(
            "background-color: #19181a;"
            "color: #fcfcfa;"
            "border: none;"
            "font-family: 'Consolas', 'Courier New', monospace;"
            "font-size: 13px;"
        );
    }
    return QString();
}

QString ThemeManager::getGraphBgColor(ThemeType type) {
    switch (type) {
    case ThemeType::DarkCatppuccin:
        return QStringLiteral("#1e1e2e");
    case ThemeType::LightModern:
        return QStringLiteral("#f8f9fa");
    case ThemeType::SolarizedDark:
        return QStringLiteral("#002b36");
    case ThemeType::MonokaiPro:
        return QStringLiteral("#2d2a2e");
    }
    return QStringLiteral("#1e1e2e");
}

} // namespace Coverage
