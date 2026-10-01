#include "project_hub_dialog.h"
#include "coverage/app_config.h"
#include "coverage/logger.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QSplitter>

namespace Coverage {

ProjectHubDialog::ProjectHubDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("嵌入式工程管理中心 (Project Management Hub)"));
    resize(920, 620);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.svg")));

    setupUi();
    refreshRecentList();
}

void ProjectHubDialog::setupUi() {
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(12);

    // 1. 左侧导航侧边栏
    auto* leftBar = new QWidget(this);
    leftBar->setFixedWidth(210);
    auto* leftLayout = new QVBoxLayout(leftBar);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);

    auto* lblAppTitle = new QLabel(QStringLiteral("📦 工程管理中心"), leftBar);
    lblAppTitle->setStyleSheet(isLight ?
        QStringLiteral("font-weight: bold; font-size: 15px; color: #1a73e8; padding: 6px 2px;") :
        QStringLiteral("font-weight: bold; font-size: 15px; color: #89b4fa; padding: 6px 2px;"));
    leftLayout->addWidget(lblAppTitle);

    m_navList = new QListWidget(leftBar);
    if (isLight) {
        m_navList->setStyleSheet(QStringLiteral(
            "QListWidget {"
            "  background-color: #ffffff;"
            "  border: 1px solid #dadce0;"
            "  border-radius: 8px;"
            "  outline: none;"
            "  padding: 4px;"
            "}"
            "QListWidget::item {"
            "  height: 38px;"
            "  padding-left: 10px;"
            "  border-radius: 6px;"
            "  color: #202124;"
            "  font-size: 13px;"
            "}"
            "QListWidget::item:selected {"
            "  background-color: #e8f0fe;"
            "  color: #1a73e8;"
            "  font-weight: bold;"
            "}"
            "QListWidget::item:hover:!selected {"
            "  background-color: #f1f3f4;"
            "}"
        ));
    } else {
        m_navList->setStyleSheet(QStringLiteral(
            "QListWidget {"
            "  background-color: #1e1e2e;"
            "  border: 1px solid #313244;"
            "  border-radius: 8px;"
            "  outline: none;"
            "  padding: 4px;"
            "}"
            "QListWidget::item {"
            "  height: 38px;"
            "  padding-left: 10px;"
            "  border-radius: 6px;"
            "  color: #cdd6f4;"
            "  font-size: 13px;"
            "}"
            "QListWidget::item:selected {"
            "  background-color: #313244;"
            "  color: #89b4fa;"
            "  font-weight: bold;"
            "}"
            "QListWidget::item:hover:!selected {"
            "  background-color: #262738;"
            "}"
        ));
    }

    m_navList->addItem(QStringLiteral("🌟 欢迎与快捷向导"));
    m_navList->addItem(QStringLiteral("➕ 创建新工程"));
    m_navList->addItem(QStringLiteral("📂 打开已有工程"));
    m_navList->addItem(QStringLiteral("🚀 体验示例工程"));
    m_navList->addItem(QStringLiteral("🕒 最近打开工程"));
    m_navList->setCurrentRow(0);

    leftLayout->addWidget(m_navList, 1);

    m_chkAutoOpenLast = new QCheckBox(QStringLiteral("下次启动自动载入最近工程"), leftBar);
    m_chkAutoOpenLast->setChecked(AppConfig::instance().autoOpenRecentProject);
    m_chkAutoOpenLast->setStyleSheet(isLight ? QStringLiteral("color: #5f6368; font-size: 11px;") : QStringLiteral("color: #a6adc8; font-size: 11px;"));
    connect(m_chkAutoOpenLast, &QCheckBox::toggled, this, &ProjectHubDialog::onAutoOpenPrefToggled);
    leftLayout->addWidget(m_chkAutoOpenLast);

    rootLayout->addWidget(leftBar);

    // 2. 右侧多页面栈 (TabWidget 无 TabBar)
    m_stackPages = new QTabWidget(this);
    m_stackPages->tabBar()->hide();
    m_stackPages->setStyleSheet(isLight ?
        QStringLiteral(
            "QTabWidget::pane {"
            "  background-color: #ffffff;"
            "  border: 1px solid #dadce0;"
            "  border-radius: 8px;"
            "}"
        ) :
        QStringLiteral(
            "QTabWidget::pane {"
            "  background-color: #181825;"
            "  border: 1px solid #313244;"
            "  border-radius: 8px;"
            "}"
        ));

    setupWelcomePage();
    setupCreatePage();
    setupOpenPage();
    setupSamplesPage();
    setupRecentPage();

    rootLayout->addWidget(m_stackPages, 1);

    connect(m_navList, &QListWidget::currentRowChanged, this, &ProjectHubDialog::onNavIndexChanged);
}

void ProjectHubDialog::onNavIndexChanged(int index) {
    if (index >= 0 && index < m_stackPages->count()) {
        m_stackPages->setCurrentIndex(index);
        if (index == 4) {
            refreshRecentList();
        }
    }
}

// -----------------------------------------------------------------------------
// Page 1: 欢迎与快捷向导
// -----------------------------------------------------------------------------
void ProjectHubDialog::setupWelcomePage() {
    auto* page = new QWidget(m_stackPages);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);

    auto* lblHero = new QLabel(QStringLiteral("欢迎使用 Embedded C Coverage Studio"), page);
    lblHero->setStyleSheet(isLight ?
        QStringLiteral("font-size: 19px; font-weight: bold; color: #1a73e8;") :
        QStringLiteral("font-size: 19px; font-weight: bold; color: #cdd6f4;"));
    layout->addWidget(lblHero);

    auto* lblDesc = new QLabel(QStringLiteral(
        "本套件专为高可靠性嵌入式 C 软件（车载控制器、航天姿态传感、电机驱动、工业 PLC）提供代码覆盖度量、\n"
        "MISRA-C 静态缺陷扫描、Graphviz 零失真拓扑分析与 Python 自动化激励注入能力。每次分析以工程 (.covproj) 为单位。"
    ), page);
    lblDesc->setStyleSheet(isLight ?
        QStringLiteral("color: #5f6368; font-size: 12px; line-height: 1.5;") :
        QStringLiteral("color: #a6adc8; font-size: 12px; line-height: 1.5;"));
    layout->addWidget(lblDesc);

    layout->addSpacing(8);

    auto* gridCards = new QGridLayout();
    gridCards->setSpacing(12);

    auto createActionCard = [&](const QString& icon, const QString& title, const QString& desc, const QString& btnText, auto slot) {
        auto* box = new QFrame(page);
        if (isLight) {
            box->setStyleSheet(QStringLiteral(
                "QFrame {"
                "  background-color: #f8f9fa;"
                "  border: 1px solid #dadce0;"
                "  border-radius: 8px;"
                "  padding: 12px;"
                "}"
                "QFrame:hover {"
                "  border: 1px solid #1a73e8;"
                "}"
            ));
        } else {
            box->setStyleSheet(QStringLiteral(
                "QFrame {"
                "  background-color: #1e1e2e;"
                "  border: 1px solid #45475a;"
                "  border-radius: 8px;"
                "  padding: 12px;"
                "}"
                "QFrame:hover {"
                "  border: 1px solid #89b4fa;"
                "}"
            ));
        }
        auto* v = new QVBoxLayout(box);
        v->setContentsMargins(8, 8, 8, 8);
        v->setSpacing(6);

        auto* t = new QLabel(icon + QStringLiteral(" ") + title, box);
        t->setStyleSheet(isLight ?
            QStringLiteral("font-weight: bold; font-size: 14px; color: #1a73e8;") :
            QStringLiteral("font-weight: bold; font-size: 14px; color: #89b4fa;"));
        v->addWidget(t);

        auto* d = new QLabel(desc, box);
        d->setStyleSheet(isLight ?
            QStringLiteral("color: #5f6368; font-size: 11px;") :
            QStringLiteral("color: #a6adc8; font-size: 11px;"));
        d->setWordWrap(true);
        v->addWidget(d, 1);

        auto* btn = new QPushButton(btnText, box);
        if (isLight) {
            btn->setStyleSheet(QStringLiteral(
                "QPushButton {"
                "  background-color: #1a73e8;"
                "  color: #ffffff;"
                "  font-weight: bold;"
                "  padding: 6px 12px;"
                "  border-radius: 4px;"
                "}"
                "QPushButton:hover { background-color: #1557b0; }"
            ));
        } else {
            btn->setStyleSheet(QStringLiteral(
                "QPushButton {"
                "  background-color: #313244;"
                "  color: #cdd6f4;"
                "  font-weight: bold;"
                "  padding: 6px 12px;"
                "  border-radius: 4px;"
                "}"
                "QPushButton:hover { background-color: #89b4fa; color: #11111b; }"
            ));
        }
        connect(btn, &QPushButton::clicked, this, slot);
        v->addWidget(btn);

        return box;
    };

    gridCards->addWidget(createActionCard(
        QStringLiteral("➕"), QStringLiteral("新建工程"),
        QStringLiteral("创建全新的嵌入式分析工程，配置目标硬件芯片架构、C/C++ 源码与 Python 激励脚本。"),
        QStringLiteral("立即新建..."), [this]() { m_navList->setCurrentRow(1); }
    ), 0, 0);

    gridCards->addWidget(createActionCard(
        QStringLiteral("📂"), QStringLiteral("打开已有工程"),
        QStringLiteral("从本地磁盘载入已有的 .covproj 工程配置文件并恢复工作区状态。"),
        QStringLiteral("浏览打开..."), [this]() { m_navList->setCurrentRow(2); }
    ), 0, 1);

    gridCards->addWidget(createActionCard(
        QStringLiteral("🚗"), QStringLiteral("电机驱动示例工程"),
        QStringLiteral("车载无刷电机矢量驱动与 ASIL-B 故障安全控制工程，包含 CAN 报文与模糊测试激励。"),
        QStringLiteral("载入电机示例"), &ProjectHubDialog::onOpenSampleMotor
    ), 1, 0);

    gridCards->addWidget(createActionCard(
        QStringLiteral("🛰️"), QStringLiteral("传感器融合工程"),
        QStringLiteral("姿态航向参考 (AHRS) 互补滤波算法，包含六轴 IMU 与气压计注入脚本。"),
        QStringLiteral("载入传感示例"), &ProjectHubDialog::onOpenSampleSensor
    ), 1, 1);

    layout->addLayout(gridCards, 1);

    m_stackPages->addTab(page, QStringLiteral("Welcome"));
}

// -----------------------------------------------------------------------------
// Page 2: 创建新工程
// -----------------------------------------------------------------------------
void ProjectHubDialog::setupCreatePage() {
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    auto* page = new QWidget(m_stackPages);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("➕ 创建新的嵌入式分析工程 (.covproj)"), page);
    title->setStyleSheet(isLight ?
        QStringLiteral("font-size: 16px; font-weight: bold; color: #1a73e8;") :
        QStringLiteral("font-size: 16px; font-weight: bold; color: #cdd6f4;"));
    layout->addWidget(title);

    auto* grid = new QGridLayout();
    grid->setSpacing(10);

    // 1. 工程名称
    grid->addWidget(new QLabel(QStringLiteral("工程名称:"), page), 0, 0);
    m_txtNewName = new QLineEdit(page);
    m_txtNewName->setPlaceholderText(QStringLiteral("例如：BMS_Battery_Safety_System"));
    m_txtNewName->setText(QStringLiteral("Embedded_Target_Project"));
    grid->addWidget(m_txtNewName, 0, 1, 1, 2);

    // 2. 存储路径
    grid->addWidget(new QLabel(QStringLiteral("工程根目录:"), page), 1, 0);
    m_txtNewDir = new QLineEdit(page);
    m_txtNewDir->setText(QDir::toNativeSeparators(QDir::currentPath() + QStringLiteral("/projects")));
    grid->addWidget(m_txtNewDir, 1, 1);
    auto* btnBrowseDir = new QPushButton(QStringLiteral("浏览..."), page);
    connect(btnBrowseDir, &QPushButton::clicked, this, &ProjectHubDialog::onCreateBrowseDir);
    grid->addWidget(btnBrowseDir, 1, 2);

    // 3. 目标芯片硬件架构
    grid->addWidget(new QLabel(QStringLiteral("目标硬件架构:"), page), 2, 0);
    m_cmbTargetArch = new QComboBox(page);
    m_cmbTargetArch->addItems({
        QStringLiteral("ARM Cortex-M4 (STM32F4/F3, NXP LPC)"),
        QStringLiteral("ARM Cortex-M7 (STM32H7, i.MX RT)"),
        QStringLiteral("RISC-V 32-bit (GD32VF103, ESP32-C3)"),
        QStringLiteral("AutoSAR Classic Platform (车规级)"),
        QStringLiteral("TI C2000 DSP (TMS320F28379D)"),
        QStringLiteral("通用嵌入式 C99/C11 裸机内核")
    });
    grid->addWidget(m_cmbTargetArch, 2, 1, 1, 2);

    // 4. 描述说明
    grid->addWidget(new QLabel(QStringLiteral("工程简要说明:"), page), 3, 0, Qt::AlignTop);
    m_txtNewDesc = new QTextEdit(page);
    m_txtNewDesc->setPlaceholderText(QStringLiteral("简要记录此工程的嵌入式软件模块定位、安全完整性等级 (如 ASIL-B / SIL-2) 等..."));
    m_txtNewDesc->setMaximumHeight(90);
    grid->addWidget(m_txtNewDesc, 3, 1, 1, 2);

    layout->addLayout(grid);

    auto* tipBox = new QLabel(QStringLiteral("💡 提示：创建成功后，将自动生成 .covproj 工程配置文件并立即加载进主工作区。后续您可随时向左侧工程树添加更多源文件或激励脚本。"), page);
    tipBox->setStyleSheet(isLight ?
        QStringLiteral("color: #5f6368; font-size: 11px; padding: 6px; background: #f1f3f4; border: 1px solid #dadce0; border-radius: 4px;") :
        QStringLiteral("color: #a6adc8; font-size: 11px; padding: 6px; background: #1e1e2e; border: 1px solid #313244; border-radius: 4px;"));
    tipBox->setWordWrap(true);
    layout->addWidget(tipBox);

    layout->addStretch();

    auto* btnSubmit = new QPushButton(QStringLiteral("🚀 立即创建并打开工程"), page);
    if (isLight) {
        btnSubmit->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #1a73e8;"
            "  color: #ffffff;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "  padding: 10px 20px;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:hover { background-color: #1557b0; }"
        ));
    } else {
        btnSubmit->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #a6e3a1;"
            "  color: #11111b;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "  padding: 10px 20px;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:hover { background-color: #94e2d5; }"
        ));
    }
    connect(btnSubmit, &QPushButton::clicked, this, &ProjectHubDialog::onCreateProjectSubmit);
    layout->addWidget(btnSubmit, 0, Qt::AlignRight);

    m_stackPages->addTab(page, QStringLiteral("Create"));
}

void ProjectHubDialog::onCreateBrowseDir() {
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择工程根目录"), m_txtNewDir->text());
    if (!dir.isEmpty()) {
        m_txtNewDir->setText(QDir::toNativeSeparators(dir));
    }
}

void ProjectHubDialog::onCreateProjectSubmit() {
    QString name = m_txtNewName->text().trimmed();
    QString dir = m_txtNewDir->text().trimmed();
    if (name.isEmpty() || dir.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("输入不完整"), QStringLiteral("请输入工程名称与目标目录。"));
        return;
    }

    QString arch = m_cmbTargetArch->currentText();
    bool ok = ProjectManager::instance().createNewProject(name, dir, arch);
    if (ok) {
        CoverageProject& proj = ProjectManager::instance().currentProject();
        proj.description = m_txtNewDesc->toPlainText().trimmed();
        proj.saveToFile();

        m_selectedProjectPath = proj.filePath;
        emit projectOpened(m_selectedProjectPath);
        accept();
    } else {
        QMessageBox::critical(this, QStringLiteral("创建工程失败"), QStringLiteral("无法在指定目录创建工程文件，请检查磁盘权限。"));
    }
}

// -----------------------------------------------------------------------------
// Page 3: 打开已有工程
// -----------------------------------------------------------------------------
void ProjectHubDialog::setupOpenPage() {
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    auto* page = new QWidget(m_stackPages);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("📂 打开已有的嵌入式工程 (.covproj)"), page);
    title->setStyleSheet(isLight ?
        QStringLiteral("font-size: 16px; font-weight: bold; color: #1a73e8;") :
        QStringLiteral("font-size: 16px; font-weight: bold; color: #cdd6f4;"));
    layout->addWidget(title);

    auto* hPath = new QHBoxLayout();
    m_txtOpenPath = new QLineEdit(page);
    m_txtOpenPath->setPlaceholderText(QStringLiteral("请选择或输入 .covproj 文件绝对路径..."));
    hPath->addWidget(m_txtOpenPath, 1);

    auto* btnBrowse = new QPushButton(QStringLiteral("浏览工程..."), page);
    connect(btnBrowse, &QPushButton::clicked, this, &ProjectHubDialog::onOpenBrowseFile);
    hPath->addWidget(btnBrowse);
    layout->addLayout(hPath);

    m_lblOpenPreview = new QLabel(page);
    if (isLight) {
        m_lblOpenPreview->setStyleSheet(QStringLiteral(
            "QLabel {"
            "  background-color: #ffffff;"
            "  border: 1px solid #dadce0;"
            "  border-radius: 6px;"
            "  padding: 16px;"
            "  color: #202124;"
            "  font-size: 12px;"
            "}"
        ));
    } else {
        m_lblOpenPreview->setStyleSheet(QStringLiteral(
            "QLabel {"
            "  background-color: #1e1e2e;"
            "  border: 1px solid #45475a;"
            "  border-radius: 6px;"
            "  padding: 16px;"
            "  color: #cdd6f4;"
            "  font-size: 12px;"
            "}"
        ));
    }
    m_lblOpenPreview->setText(QStringLiteral("请选择一个有效的 .covproj 工程文件以预览元数据。"));
    m_lblOpenPreview->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(m_lblOpenPreview, 1);

    auto* btnOpen = new QPushButton(QStringLiteral("📂 打开所选工程"), page);
    if (isLight) {
        btnOpen->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #1a73e8;"
            "  color: #ffffff;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "  padding: 8px 18px;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:hover { background-color: #1557b0; }"
        ));
    } else {
        btnOpen->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #89b4fa;"
            "  color: #11111b;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "  padding: 8px 18px;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:hover { background-color: #b4befe; }"
        ));
    }
    connect(btnOpen, &QPushButton::clicked, this, &ProjectHubDialog::onOpenProjectSubmit);
    layout->addWidget(btnOpen, 0, Qt::AlignRight);

    m_stackPages->addTab(page, QStringLiteral("Open"));
}

void ProjectHubDialog::onOpenBrowseFile() {
    QString p = QFileDialog::getOpenFileName(this, QStringLiteral("选择分析工程文件"),
                                            QDir::currentPath(),
                                            QStringLiteral("Coverage Project (*.covproj);;All Files (*.*)"));
    if (!p.isEmpty()) {
        m_txtOpenPath->setText(QDir::toNativeSeparators(p));

        bool ok = false;
        CoverageProject info = CoverageProject::loadFromFile(p, &ok);
        if (ok) {
            QString preview = QString(
                "<b>工程名称:</b> %1<br/>"
                "<b>目标硬件:</b> %2<br/>"
                "<b>源码文件:</b> %3 个<br/>"
                "<b>激励脚本:</b> %4 个<br/>"
                "<b>最后修改:</b> %5<br/>"
                "<b>工程路径:</b> %6<br/><br/>"
                "<b>说明:</b> %7"
            ).arg(info.name, info.targetArch)
             .arg(info.sourceFiles.size()).arg(info.scriptFiles.size())
             .arg(info.lastModifiedTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")))
             .arg(info.filePath, info.description.isEmpty() ? QStringLiteral("无详细说明") : info.description);
            m_lblOpenPreview->setText(preview);
        } else {
            m_lblOpenPreview->setText(QStringLiteral("⚠️ 所选文件不是合法的 .covproj 工程 JSON 格式！"));
        }
    }
}

void ProjectHubDialog::onOpenProjectSubmit() {
    QString p = m_txtOpenPath->text().trimmed();
    if (!QFile::exists(p)) {
        QMessageBox::warning(this, QStringLiteral("文件不存在"), QStringLiteral("指定的 .covproj 工程文件不存在。"));
        return;
    }

    if (ProjectManager::instance().openProject(p)) {
        m_selectedProjectPath = p;
        emit projectOpened(p);
        accept();
    } else {
        QMessageBox::critical(this, QStringLiteral("打开工程失败"), QStringLiteral("无法解析该工程文件。"));
    }
}

// -----------------------------------------------------------------------------
// Page 4: 体验示例工程
// -----------------------------------------------------------------------------
void ProjectHubDialog::setupSamplesPage() {
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    auto* page = new QWidget(m_stackPages);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("🚀 开箱即用内置示例工程 (Sample Projects)"), page);
    title->setStyleSheet(isLight ?
        QStringLiteral("font-size: 16px; font-weight: bold; color: #1a73e8;") :
        QStringLiteral("font-size: 16px; font-weight: bold; color: #cdd6f4;"));
    layout->addWidget(title);

    auto* sampleBox1 = new QGroupBox(QStringLiteral("示例 1: 车载无刷电机驱动与故障安全系统 (Motor Controller)"), page);
    auto* v1 = new QVBoxLayout(sampleBox1);
    auto* d1 = new QLabel(QStringLiteral(
        "• <b>行业定位:</b> 车载车身动力与底盘驱动 (ASIL-B 级安全架构)\n"
        "• <b>核心算法:</b> 三相 BLDC 电机 FOC 矢量控制、速度/电流闭环 PID、短路/过温双重闭锁保护\n"
        "• <b>配套激励:</b> CAN 报文实时注入、极限边界与毛刺注入模糊测试脚本\n"
        "• <b>包含文件:</b> motor_controller.c, motor_controller.h, can_frame_injector.py, boundary_fuzzer.py"
    ), sampleBox1);
    d1->setStyleSheet(isLight ?
        QStringLiteral("color: #5f6368; font-size: 12px; line-height: 1.4;") :
        QStringLiteral("color: #a6adc8; font-size: 12px; line-height: 1.4;"));
    v1->addWidget(d1);

    auto* btnLoadMotor = new QPushButton(QStringLiteral("⚡ 立即载入电机控制器工程"), sampleBox1);
    if (isLight) {
        btnLoadMotor->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #1a73e8; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px;"
            "}"
            "QPushButton:hover { background-color: #1557b0; }"
        ));
    } else {
        btnLoadMotor->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #89b4fa; color: #11111b; font-weight: bold; padding: 6px 14px; border-radius: 4px;"
            "}"
            "QPushButton:hover { background-color: #b4befe; }"
        ));
    }
    connect(btnLoadMotor, &QPushButton::clicked, this, &ProjectHubDialog::onOpenSampleMotor);
    v1->addWidget(btnLoadMotor, 0, Qt::AlignRight);
    layout->addWidget(sampleBox1);

    auto* sampleBox2 = new QGroupBox(QStringLiteral("示例 2: 航姿参考多传感器融合与位姿滤波系统 (Sensor Fusion)"), page);
    auto* v2 = new QVBoxLayout(sampleBox2);
    auto* d2 = new QLabel(QStringLiteral(
        "• <b>行业定位:</b> 无人机航姿参考系统 (AHRS) 与无人移动底盘位姿估计\n"
        "• <b>核心算法:</b> 六轴陀螺仪/加速度计与电子罗盘互补融合滤波、四元数转欧拉角、气压计卡尔曼高度估计\n"
        "• <b>配套激励:</b> 传感器正弦振动波形注入与异常突变数据注入脚本\n"
        "• <b>包含文件:</b> sensor_fusion.c, sensor_fusion.h, sensor_waveform_injector.py"
    ), sampleBox2);
    d2->setStyleSheet(isLight ?
        QStringLiteral("color: #5f6368; font-size: 12px; line-height: 1.4;") :
        QStringLiteral("color: #a6adc8; font-size: 12px; line-height: 1.4;"));
    v2->addWidget(d2);

    auto* btnLoadSensor = new QPushButton(QStringLiteral("⚡ 立即载入传感器融合工程"), sampleBox2);
    if (isLight) {
        btnLoadSensor->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #16a34a; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px;"
            "}"
            "QPushButton:hover { background-color: #15803d; }"
        ));
    } else {
        btnLoadSensor->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #a6e3a1; color: #11111b; font-weight: bold; padding: 6px 14px; border-radius: 4px;"
            "}"
            "QPushButton:hover { background-color: #94e2d5; }"
        ));
    }
    connect(btnLoadSensor, &QPushButton::clicked, this, &ProjectHubDialog::onOpenSampleSensor);
    v2->addWidget(btnLoadSensor, 0, Qt::AlignRight);
    layout->addWidget(sampleBox2);

    layout->addStretch();

    m_stackPages->addTab(page, QStringLiteral("Samples"));
}

void ProjectHubDialog::onOpenSampleMotor() {
    if (ProjectManager::instance().openBuiltinSample(QStringLiteral("motor_controller"))) {
        m_selectedProjectPath = ProjectManager::instance().currentProject().filePath;
        emit projectOpened(m_selectedProjectPath);
        accept();
    }
}

void ProjectHubDialog::onOpenSampleSensor() {
    if (ProjectManager::instance().openBuiltinSample(QStringLiteral("sensor_fusion"))) {
        m_selectedProjectPath = ProjectManager::instance().currentProject().filePath;
        emit projectOpened(m_selectedProjectPath);
        accept();
    }
}

// -----------------------------------------------------------------------------
// Page 5: 最近打开工程
// -----------------------------------------------------------------------------
void ProjectHubDialog::setupRecentPage() {
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    auto* page = new QWidget(m_stackPages);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("🕒 最近打开的历史工程 (Recent Projects)"), page);
    title->setStyleSheet(isLight ?
        QStringLiteral("font-size: 16px; font-weight: bold; color: #1a73e8;") :
        QStringLiteral("font-size: 16px; font-weight: bold; color: #cdd6f4;"));
    layout->addWidget(title);

    m_tableRecent = new QTableWidget(page);
    m_tableRecent->setColumnCount(3);
    m_tableRecent->setHorizontalHeaderLabels({
        QStringLiteral("工程名称"),
        QStringLiteral("完整路径"),
        QStringLiteral("文件状态")
    });
    m_tableRecent->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableRecent->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableRecent->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableRecent->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableRecent->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableRecent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    if (isLight) {
        m_tableRecent->setStyleSheet(QStringLiteral(
            "QTableWidget { background-color: #ffffff; alternate-background-color: #f8f9fa; border: 1px solid #dadce0; color: #202124; gridline-color: #e8eaed; }"
            "QHeaderView::section { background-color: #f1f3f4; color: #1a73e8; font-weight: bold; border: 1px solid #dadce0; padding: 6px; }"
            "QTableWidget::item:selected { background-color: #e8f0fe; color: #1a73e8; font-weight: 500; }"
        ));
    } else {
        m_tableRecent->setStyleSheet(QStringLiteral(
            "QTableWidget { background-color: #1e1e2e; alternate-background-color: #181825; border: 1px solid #313244; color: #cdd6f4; gridline-color: #313244; }"
            "QHeaderView::section { background-color: #181825; color: #89b4fa; font-weight: bold; border: 1px solid #313244; padding: 6px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; font-weight: 500; }"
        ));
    }
    connect(m_tableRecent, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        onOpenRecentItem();
    });
    layout->addWidget(m_tableRecent, 1);

    auto* hBtns = new QHBoxLayout();
    m_btnOpenRecent = new QPushButton(QStringLiteral("📂 打开所选工程"), page);
    if (isLight) {
        m_btnOpenRecent->setStyleSheet(QStringLiteral("background-color: #1a73e8; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px;"));
    } else {
        m_btnOpenRecent->setStyleSheet(QStringLiteral("background-color: #89b4fa; color: #11111b; font-weight: bold; padding: 6px 14px; border-radius: 4px;"));
    }
    connect(m_btnOpenRecent, &QPushButton::clicked, this, &ProjectHubDialog::onOpenRecentItem);

    m_btnRemoveRecent = new QPushButton(QStringLiteral("从列表移除"), page);
    connect(m_btnRemoveRecent, &QPushButton::clicked, this, &ProjectHubDialog::onRemoveRecentItem);

    m_btnClearRecent = new QPushButton(QStringLiteral("清空历史记录"), page);
    connect(m_btnClearRecent, &QPushButton::clicked, this, &ProjectHubDialog::onClearRecentList);

    hBtns->addWidget(m_btnOpenRecent);
    hBtns->addWidget(m_btnRemoveRecent);
    hBtns->addStretch();
    hBtns->addWidget(m_btnClearRecent);
    layout->addLayout(hBtns);

    m_stackPages->addTab(page, QStringLiteral("Recent"));
}

void ProjectHubDialog::refreshRecentList() {
    if (!m_tableRecent) return;
    m_tableRecent->setRowCount(0);

    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    const QStringList recents = ProjectManager::instance().recentProjects();
    for (const QString& path : recents) {
        int row = m_tableRecent->rowCount();
        m_tableRecent->insertRow(row);

        QFileInfo fi(path);
        bool exists = fi.exists();

        auto* itemTitle = new QTableWidgetItem(fi.baseName());
        auto* itemPath = new QTableWidgetItem(path);
        auto* itemStatus = new QTableWidgetItem(exists ? QStringLiteral("就绪") : QStringLiteral("已丢失"));

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

void ProjectHubDialog::onOpenRecentItem() {
    int row = m_tableRecent->currentRow();
    if (row < 0) return;

    QString p = m_tableRecent->item(row, 1)->text();
    if (!QFile::exists(p)) {
        QMessageBox::warning(this, QStringLiteral("文件不存在"), QStringLiteral("工程文件已在磁盘中被移动或删除。"));
        return;
    }

    if (ProjectManager::instance().openProject(p)) {
        m_selectedProjectPath = p;
        emit projectOpened(p);
        accept();
    }
}

void ProjectHubDialog::onRemoveRecentItem() {
    int row = m_tableRecent->currentRow();
    if (row < 0) return;
    QString p = m_tableRecent->item(row, 1)->text();
    ProjectManager::instance().removeRecentProject(p);
    refreshRecentList();
}

void ProjectHubDialog::onClearRecentList() {
    ProjectManager::instance().clearRecentProjects();
    refreshRecentList();
}

void ProjectHubDialog::onAutoOpenPrefToggled(bool checked) {
    AppConfig::instance().autoOpenRecentProject = checked;
    AppConfig::instance().save();
}

} // namespace Coverage
