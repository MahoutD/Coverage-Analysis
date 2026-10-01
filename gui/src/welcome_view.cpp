#include "welcome_view.h"
#include "coverage/project.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QSplitter>

namespace Coverage {

WelcomeView::WelcomeView(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refreshRecentProjects();
    applyTheme(ThemeManager::currentTheme());
}

void WelcomeView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // 1. 顶部 Header / Banner
    m_bannerWidget = new QWidget(this);
    auto* bannerLayout = new QHBoxLayout(m_bannerWidget);
    bannerLayout->setContentsMargins(12, 8, 12, 8);

    auto* lblLogo = new QLabel(m_bannerWidget);
    lblLogo->setPixmap(QIcon(QStringLiteral(":/icons/app_icon.svg")).pixmap(54, 54));
    bannerLayout->addWidget(lblLogo);

    auto* bannerTextLayout = new QVBoxLayout();
    bannerTextLayout->setSpacing(4);

    m_lblTitle = new QLabel(QStringLiteral("嵌入式 C 覆盖率分析与静态测试套件"), m_bannerWidget);
    m_lblSub = new QLabel(QStringLiteral("v2.5.0 Enterprise · 面向 ISO 26262 ASIL-B / DO-178C 高可靠性嵌入式系统的语句、分支、MCDC 覆盖分析与拓扑探索平台"), m_bannerWidget);

    bannerTextLayout->addWidget(m_lblTitle);
    bannerTextLayout->addWidget(m_lblSub);
    bannerLayout->addLayout(bannerTextLayout, 1);

    mainLayout->addWidget(m_bannerWidget);

    // 2. 主体左右分栏
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // -------------------------------------------------------------
    // 左侧面板: 最近打开的项目 (Recent Projects) - 双击快速打开
    // -------------------------------------------------------------
    m_leftBox = new QGroupBox(QStringLiteral("🕒 最近打开的项目 (双击快速打开)"), splitter);
    auto* leftLayout = new QVBoxLayout(m_leftBox);
    leftLayout->setContentsMargins(12, 14, 12, 12);
    leftLayout->setSpacing(10);

    m_tableRecent = new QTableWidget(m_leftBox);
    m_tableRecent->setColumnCount(3);
    m_tableRecent->setHorizontalHeaderLabels({
        QStringLiteral("工程名称"),
        QStringLiteral("完整路径"),
        QStringLiteral("状态")
    });
    m_tableRecent->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableRecent->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableRecent->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableRecent->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableRecent->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableRecent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableRecent->verticalHeader()->setVisible(false);

    // 双击项目列表行即可直接打开对应工程
    connect(m_tableRecent, &QTableWidget::cellDoubleClicked,
            this, &WelcomeView::onRecentItemDoubleClicked);

    leftLayout->addWidget(m_tableRecent, 1);

    auto* leftBtnLayout = new QHBoxLayout();
    m_btnOpenRecent = new QPushButton(QIcon(QStringLiteral(":/icons/open_file.svg")), QStringLiteral("打开所选工程"), m_leftBox);
    connect(m_btnOpenRecent, &QPushButton::clicked, this, &WelcomeView::onBtnOpenRecentClicked);

    m_btnRemoveRecent = new QPushButton(QIcon(QStringLiteral(":/icons/delete.svg")), QStringLiteral("从历史中移除"), m_leftBox);
    connect(m_btnRemoveRecent, &QPushButton::clicked, this, &WelcomeView::onBtnRemoveRecentClicked);

    m_btnClearRecent = new QPushButton(QStringLiteral("清空历史记录"), m_leftBox);
    connect(m_btnClearRecent, &QPushButton::clicked, this, &WelcomeView::onBtnClearRecentClicked);

    leftBtnLayout->addWidget(m_btnOpenRecent);
    leftBtnLayout->addWidget(m_btnRemoveRecent);
    leftBtnLayout->addStretch();
    leftBtnLayout->addWidget(m_btnClearRecent);
    leftLayout->addLayout(leftBtnLayout);

    splitter->addWidget(m_leftBox);

    // -------------------------------------------------------------
    // 右侧面板: 快速启动与内置示例 (Quick Start & Samples)
    // -------------------------------------------------------------
    m_rightBox = new QGroupBox(QStringLiteral("🚀 快速开始与示例 (Quick Start)"), splitter);
    auto* rightLayout = new QVBoxLayout(m_rightBox);
    rightLayout->setContentsMargins(12, 14, 12, 12);
    rightLayout->setSpacing(12);

    auto createCard = [this](const QString& iconPath, const QString& title, const QString& desc, auto slotFunc) -> QPushButton* {
        auto* btn = new QPushButton(m_rightBox);
        btn->setCursor(Qt::PointingHandCursor);
        m_quickCards.append(btn);

        auto* h = new QHBoxLayout(btn);
        h->setContentsMargins(4, 4, 4, 4);
        h->setSpacing(12);

        auto* lblIcon = new QLabel(btn);
        lblIcon->setPixmap(QIcon(iconPath).pixmap(26, 26));
        lblIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
        h->addWidget(lblIcon);

        auto* v = new QVBoxLayout();
        v->setSpacing(2);
        auto* lblT = new QLabel(title, btn);
        lblT->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px; color: #cdd6f4;"));
        lblT->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto* lblD = new QLabel(desc, btn);
        lblD->setStyleSheet(QStringLiteral("font-size: 11px; color: #a6adc8;"));
        lblD->setAttribute(Qt::WA_TransparentForMouseEvents);

        v->addWidget(lblT);
        v->addWidget(lblD);
        h->addLayout(v, 1);

        m_cardTitles.append(lblT);
        m_cardDescs.append(lblD);

        connect(btn, &QPushButton::clicked, this, slotFunc);
        return btn;
    };

    rightLayout->addWidget(createCard(
        QStringLiteral(":/icons/app_icon.svg"),
        QStringLiteral("🌟 新建分析工程 (New Project)"),
        QStringLiteral("向导式创建全新的 .covproj 分析工程规范"),
        [this]() { emit newProjectRequested(); }
    ));

    rightLayout->addWidget(createCard(
        QStringLiteral(":/icons/open_file.svg"),
        QStringLiteral("📂 打开已有工程 (Open Project)"),
        QStringLiteral("浏览磁盘并加载现有的 .covproj 工程文件"),
        [this]() { emit openProjectRequested(); }
    ));

    rightLayout->addWidget(createCard(
        QStringLiteral(":/icons/import_dir.svg"),
        QStringLiteral("📄 导入源码文件 / 目录 (Import Files)"),
        QStringLiteral("直接载入嵌入式 C/C++ 源文件或扫描整个工程源码树"),
        [this]() { emit openFileRequested(); }
    ));

    rightLayout->addWidget(createCard(
        QStringLiteral(":/icons/project.svg"),
        QStringLiteral("🚗 体验: 车载电机控制器完整工程 (Motor Controller)"),
        QStringLiteral("FOC 矢量驱动、PWM 死区、ASIL-B 功能安全闭锁与 CAN 激励"),
        [this]() { emit openSampleMotorRequested(); }
    ));

    rightLayout->addWidget(createCard(
        QStringLiteral(":/icons/project.svg"),
        QStringLiteral("🛰️ 体验: 姿态多传感器融合完整工程 (Sensor Fusion)"),
        QStringLiteral("扩展卡尔曼滤波 (EKF)、9 轴 IMU 冗余融合与异常剔除"),
        [this]() { emit openSampleSensorRequested(); }
    ));

    rightLayout->addWidget(createCard(
        QStringLiteral(":/icons/user_manual.svg"),
        QStringLiteral("📖 查看系统使用说明书 (User Manual)"),
        QStringLiteral("查阅覆盖率理论、MISRA-C 规则体系与快捷键说明"),
        [this]() { emit openManualRequested(); }
    ));

    rightLayout->addStretch();
    splitter->addWidget(m_rightBox);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);
}

void WelcomeView::refreshRecentProjects() {
    if (!m_tableRecent) return;
    m_tableRecent->setRowCount(0);

    const QStringList recents = ProjectManager::instance().recentProjects();
    for (const QString& path : recents) {
        int row = m_tableRecent->rowCount();
        m_tableRecent->insertRow(row);

        QFileInfo fi(path);
        bool exists = fi.exists();

        auto* itemTitle = new QTableWidgetItem(QIcon(QStringLiteral(":/icons/project.svg")), fi.baseName());
        auto* itemPath = new QTableWidgetItem(QDir::toNativeSeparators(path));
        itemPath->setToolTip(path);

        auto* itemStatus = new QTableWidgetItem(exists ? QStringLiteral("就绪") : QStringLiteral("文件丢失"));
        bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
        if (!exists) {
            itemStatus->setForeground(isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8")));
        } else {
            itemStatus->setForeground(isLight ? QColor(QStringLiteral("#16a34a")) : QColor(QStringLiteral("#a6e3a1")));
        }

        m_tableRecent->setItem(row, 0, itemTitle);
        m_tableRecent->setItem(row, 1, itemPath);
        m_tableRecent->setItem(row, 2, itemStatus);
    }
}

void WelcomeView::onRecentItemDoubleClicked(int row, int /* col */) {
    if (row < 0 || row >= m_tableRecent->rowCount()) return;

    QString path = m_tableRecent->item(row, 1)->text();
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("文件不存在"),
                             QStringLiteral("所选工程文件在磁盘中已不存在或已被移动:\n") + path);
        ProjectManager::instance().removeRecentProject(path);
        refreshRecentProjects();
        return;
    }

    emit projectSelected(path);
}

void WelcomeView::onBtnOpenRecentClicked() {
    int row = m_tableRecent->currentRow();
    if (row >= 0) {
        onRecentItemDoubleClicked(row, 0);
    }
}

void WelcomeView::onBtnRemoveRecentClicked() {
    int row = m_tableRecent->currentRow();
    if (row < 0) return;

    QString path = m_tableRecent->item(row, 1)->text();
    ProjectManager::instance().removeRecentProject(path);
    refreshRecentProjects();
}

void WelcomeView::onBtnClearRecentClicked() {
    if (QMessageBox::question(this, QStringLiteral("清空历史记录"),
                              QStringLiteral("确定要清空所有最近打开的工程记录吗？"),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        ProjectManager::instance().clearRecentProjects();
        refreshRecentProjects();
    }
}

void WelcomeView::applyTheme(ThemeType type) {
    bool isLight = (type == ThemeType::LightModern);

    if (m_bannerWidget) {
        if (isLight) {
            m_bannerWidget->setStyleSheet(QStringLiteral(
                "QWidget { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ffffff, stop:1 #f8f9fa); "
                "border: 1px solid #dadce0; border-radius: 8px; padding: 12px 18px; }"
            ));
            if (m_lblTitle) m_lblTitle->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #1a73e8; background: transparent; border: none;"));
            if (m_lblSub) m_lblSub->setStyleSheet(QStringLiteral("font-size: 13px; color: #5f6368; background: transparent; border: none;"));
        } else {
            m_bannerWidget->setStyleSheet(QStringLiteral(
                "QWidget { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #181825, stop:1 #1e1e2e); "
                "border: 1px solid #313244; border-radius: 8px; padding: 12px 18px; }"
            ));
            if (m_lblTitle) m_lblTitle->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #89b4fa; background: transparent; border: none;"));
            if (m_lblSub) m_lblSub->setStyleSheet(QStringLiteral("font-size: 13px; color: #a6adc8; background: transparent; border: none;"));
        }
    }

    QString groupStyle = isLight ?
        QStringLiteral("QGroupBox { font-weight: bold; color: #1a73e8; border: 1px solid #dadce0; border-radius: 6px; margin-top: 6px; padding-top: 14px; background-color: #ffffff; }") :
        QStringLiteral("QGroupBox { font-weight: bold; color: #89b4fa; border: 1px solid #313244; border-radius: 6px; margin-top: 6px; padding-top: 14px; background-color: #181825; }");

    if (m_leftBox) m_leftBox->setStyleSheet(groupStyle);
    if (m_rightBox) m_rightBox->setStyleSheet(groupStyle);

    if (m_tableRecent) {
        if (isLight) {
            m_tableRecent->setStyleSheet(QStringLiteral(
                "QTableWidget { background-color: #ffffff; border: 1px solid #dadce0; border-radius: 6px; gridline-color: #e8eaed; color: #202124; }"
                "QHeaderView::section { background-color: #f1f3f4; color: #1a73e8; font-weight: bold; border: 1px solid #dadce0; padding: 4px; }"
                "QTableWidget::item { padding: 6px 8px; }"
                "QTableWidget::item:selected { background-color: #e8f0fe; color: #1a73e8; font-weight: bold; }"
            ));
        } else {
            m_tableRecent->setStyleSheet(QStringLiteral(
                "QTableWidget { background-color: #11111b; border: 1px solid #313244; border-radius: 6px; gridline-color: #262738; color: #cdd6f4; }"
                "QHeaderView::section { background-color: #181825; color: #89b4fa; font-weight: bold; border: 1px solid #313244; padding: 4px; }"
                "QTableWidget::item { padding: 6px 8px; }"
                "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; font-weight: bold; }"
            ));
        }
    }

    if (m_btnOpenRecent) {
        if (isLight) {
            m_btnOpenRecent->setStyleSheet(QStringLiteral("background-color: #1a73e8; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px; outline: none;"));
        } else {
            m_btnOpenRecent->setStyleSheet(QStringLiteral("background-color: #89b4fa; color: #11111b; font-weight: bold; padding: 6px 14px; border-radius: 4px; outline: none;"));
        }
    }

    for (auto* btn : m_quickCards) {
        if (!btn) continue;
        if (isLight) {
            btn->setStyleSheet(QStringLiteral(
                "QPushButton { text-align: left; padding: 10px 14px; background-color: #f8f9fa; border: 1px solid #dadce0; border-radius: 6px; outline: none; }"
                "QPushButton:hover { background-color: #e8f0fe; border-color: #1a73e8; }"
                "QPushButton:pressed { background-color: #d2e3fc; }"
            ));
        } else {
            btn->setStyleSheet(QStringLiteral(
                "QPushButton { text-align: left; padding: 10px 14px; background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; outline: none; }"
                "QPushButton:hover { background-color: #2b2c3f; border-color: #89b4fa; }"
                "QPushButton:pressed { background-color: #181825; }"
            ));
        }
    }

    for (auto* lbl : m_cardTitles) {
        if (!lbl) continue;
        lbl->setStyleSheet(isLight ?
            QStringLiteral("font-weight: bold; font-size: 13px; color: #1e293b; background: transparent;") :
            QStringLiteral("font-weight: bold; font-size: 13px; color: #cdd6f4; background: transparent;"));
    }

    for (auto* lbl : m_cardDescs) {
        if (!lbl) continue;
        lbl->setStyleSheet(isLight ?
            QStringLiteral("font-size: 11px; color: #64748b; background: transparent;") :
            QStringLiteral("font-size: 11px; color: #a6adc8; background: transparent;"));
    }

    refreshRecentProjects();
}

} // namespace Coverage
