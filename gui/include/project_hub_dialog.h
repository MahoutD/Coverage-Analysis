#pragma once

#include "coverage/project.h"
#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QListWidget>

namespace Coverage {

/**
 * @brief 嵌入式工程管理中心与启动欢迎向导 (ProjectHubDialog)
 * 提供新建工程、打开现有工程 (.covproj)、最近工程列表管理以及内置车规/飞行控制示例工程一键载入。
 */
class ProjectHubDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProjectHubDialog(QWidget* parent = nullptr);
    ~ProjectHubDialog() override = default;

    QString selectedProjectPath() const { return m_selectedProjectPath; }

signals:
    void projectOpened(const QString& covprojPath);

private slots:
    void onNavIndexChanged(int index);
    void onCreateBrowseDir();
    void onCreateProjectSubmit();
    void onOpenBrowseFile();
    void onOpenProjectSubmit();
    void onOpenSampleMotor();
    void onOpenSampleSensor();
    void onOpenRecentItem();
    void onRemoveRecentItem();
    void onClearRecentList();
    void onAutoOpenPrefToggled(bool checked);

private:
    void setupUi();
    void setupWelcomePage();
    void setupCreatePage();
    void setupOpenPage();
    void setupSamplesPage();
    void setupRecentPage();
    void refreshRecentList();

    QString m_selectedProjectPath;

    // 导航与页面
    QListWidget* m_navList = nullptr;
    QTabWidget* m_stackPages = nullptr;

    // 新建工程表单
    QLineEdit* m_txtNewName = nullptr;
    QLineEdit* m_txtNewDir = nullptr;
    QComboBox* m_cmbTargetArch = nullptr;
    QTextEdit* m_txtNewDesc = nullptr;

    // 打开工程表单
    QLineEdit* m_txtOpenPath = nullptr;
    QLabel* m_lblOpenPreview = nullptr;

    // 最近工程列表
    QTableWidget* m_tableRecent = nullptr;
    QPushButton* m_btnOpenRecent = nullptr;
    QPushButton* m_btnRemoveRecent = nullptr;
    QPushButton* m_btnClearRecent = nullptr;

    // 首选项
    QCheckBox* m_chkAutoOpenLast = nullptr;
};

} // namespace Coverage
