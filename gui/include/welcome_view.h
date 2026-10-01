#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include "theme_manager.h"

namespace Coverage {

/**
 * @brief 软件启动主欢迎界面 (WelcomeView)
 * 启动默认展示，提供最近打开工程的双击快速加载、工程新建与打开向导、
 * 以及车规级嵌入式 C 示例工程与使用说明的一键直达。
 */
class WelcomeView : public QWidget {
    Q_OBJECT
public:
    explicit WelcomeView(QWidget* parent = nullptr);
    ~WelcomeView() override = default;

    /**
     * @brief 刷新最近工程列表
     */
    void refreshRecentProjects();

    /**
     * @brief 应用主题样式 (适配深浅主题色彩规范)
     */
    void applyTheme(ThemeType type);

signals:
    void projectSelected(const QString& covprojPath);
    void newProjectRequested();
    void openProjectRequested();
    void openFileRequested();
    void openSampleMotorRequested();
    void openSampleSensorRequested();
    void openManualRequested();

private slots:
    void onRecentItemDoubleClicked(int row, int col);
    void onBtnOpenRecentClicked();
    void onBtnRemoveRecentClicked();
    void onBtnClearRecentClicked();

private:
    void setupUi();

    QWidget*      m_bannerWidget = nullptr;
    QLabel*       m_lblTitle = nullptr;
    QLabel*       m_lblSub = nullptr;
    QGroupBox*    m_leftBox = nullptr;
    QGroupBox*    m_rightBox = nullptr;
    QVector<QPushButton*> m_quickCards;
    QVector<QLabel*>      m_cardTitles;
    QVector<QLabel*>      m_cardDescs;

    QTableWidget* m_tableRecent = nullptr;
    QPushButton*  m_btnOpenRecent = nullptr;
    QPushButton*  m_btnRemoveRecent = nullptr;
    QPushButton*  m_btnClearRecent = nullptr;
    QLabel*       m_lblRecentCount = nullptr;
};

} // namespace Coverage
