#include "stimulus_view.h"
#include "coverage/backend_service.h"
#include "coverage/project.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QTabWidget>
#include <QDir>
#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>
#include <QInputDialog>
#include <QRegularExpression>

namespace Coverage {

StimulusView::StimulusView(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    reloadProjectScripts();
}

void StimulusView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // 1. Python 解释器环境配置
    auto* grpEnv = new QGroupBox(QStringLiteral("Python 解释器环境"), this);
    auto* envLayout = new QHBoxLayout(grpEnv);
    m_txtPythonPath = new QLineEdit(this);
    m_txtPythonPath->setPlaceholderText(QStringLiteral("Path to python.exe"));
    m_txtPythonPath->setText(StimulusEngine::autoDetectPython());

    m_btnAutoDetectPython = new QPushButton(QStringLiteral("环境检测"), this);
    envLayout->addWidget(new QLabel(QStringLiteral("Python 路径:"), this));
    envLayout->addWidget(m_txtPythonPath);
    envLayout->addWidget(m_btnAutoDetectPython);
    mainLayout->addWidget(grpEnv);

    // 2. 激励注入模式选择 (文件执行 vs 在线编辑)
    auto* grpMode = new QGroupBox(QStringLiteral("激励注入源配置"), this);
    auto* modeLayout = new QVBoxLayout(grpMode);

    auto* radioLayout = new QVBoxLayout();
    radioLayout->setContentsMargins(0, 0, 0, 0);
    radioLayout->setSpacing(4);
    m_radioFileMode = new QRadioButton(QStringLiteral("模式 1: 选择项目 Python 激励脚本执行"), this);
    m_radioEditorMode = new QRadioButton(QStringLiteral("模式 2: 在线编写 Python 脚本 (语法检查与在线调试)"), this);
    m_radioFileMode->setChecked(true);

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->addButton(m_radioFileMode);
    m_modeGroup->addButton(m_radioEditorMode);

    radioLayout->addWidget(m_radioFileMode);
    radioLayout->addWidget(m_radioEditorMode);
    modeLayout->addLayout(radioLayout);

    // 模式 1 容器：文件选择
    m_widgetFileMode = new QWidget(this);
    auto* fileLayout = new QGridLayout(m_widgetFileMode);
    fileLayout->setContentsMargins(0, 4, 0, 0);

    m_cmbScriptPresets = new QComboBox(this);
    m_txtCustomScript = new QLineEdit(this);
    m_txtCustomScript->setPlaceholderText(QStringLiteral("输入或浏览自定义 .py 脚本绝对路径..."));
    m_btnBrowseScript = new QPushButton(QStringLiteral("浏览..."), this);

    // Requirement 12: 不需要预设模板，只需要读取当前项目路径下的所有 Python 脚本
    fileLayout->addWidget(new QLabel(QStringLiteral("项目激励脚本:"), this), 0, 0);
    fileLayout->addWidget(m_cmbScriptPresets, 0, 1, 1, 2);
    fileLayout->addWidget(new QLabel(QStringLiteral("脚本路径:"), this), 1, 0);
    fileLayout->addWidget(m_txtCustomScript, 1, 1);
    fileLayout->addWidget(m_btnBrowseScript, 1, 2);
    modeLayout->addWidget(m_widgetFileMode);

    // 模式 2 容器：在线代码编辑器与语法语义校验
    m_widgetEditorMode = new QWidget(this);
    m_widgetEditorMode->setVisible(false);
    auto* editorLayout = new QVBoxLayout(m_widgetEditorMode);
    editorLayout->setContentsMargins(0, 4, 0, 0);

    auto* editorBar = new QHBoxLayout();
    editorBar->addWidget(new QLabel(QStringLiteral("代码模板:"), this));
    m_cmbEditorTemplates = new QComboBox(this);
    m_cmbEditorTemplates->addItem(QStringLiteral("传感器模拟与故障注入代码模板"), 0);
    m_cmbEditorTemplates->addItem(QStringLiteral("CAN 总线周期控制报文模板"), 1);
    m_cmbEditorTemplates->addItem(QStringLiteral("边界值模糊覆盖测试代码模板"), 2);
    editorBar->addWidget(m_cmbEditorTemplates);

    m_btnLoadTemplate = new QPushButton(QStringLiteral("载入模板到编辑器"), this);
    m_btnCheckSyntax = new QPushButton(QStringLiteral("🔍 语法与语义检查"), this);
    m_btnCheckSyntax->setStyleSheet(QStringLiteral("color: #89b4fa; font-weight: bold;"));

    editorBar->addWidget(m_btnLoadTemplate);
    editorBar->addWidget(m_btnCheckSyntax);
    editorBar->addStretch();
    editorLayout->addLayout(editorBar);

    // 语法检查状态反馈标签
    m_lblSyntaxResult = new QLabel(QStringLiteral("尚未执行语法检查。"), this);
    m_lblSyntaxResult->setStyleSheet(QStringLiteral("color: #a6adc8; padding: 4px; background: #11111b; border-radius: 4px;"));
    editorLayout->addWidget(m_lblSyntaxResult);

    // 代码编辑器
    m_txtCodeEditor = new QPlainTextEdit(this);
    m_txtCodeEditor->setStyleSheet(ThemeManager::getEditorStyleSheet());
    m_txtCodeEditor->setMinimumHeight(180);
    editorLayout->addWidget(m_txtCodeEditor);

    modeLayout->addWidget(m_widgetEditorMode);

    // 公共命令行参数
    auto* argsLayout = new QHBoxLayout();
    argsLayout->addWidget(new QLabel(QStringLiteral("附加参数:"), this));
    m_txtArgs = new QLineEdit(this);
    m_txtArgs->setPlaceholderText(QStringLiteral("--steps 100 --target motor_controller.c"));
    argsLayout->addWidget(m_txtArgs);

    m_btnRun = new QPushButton(QStringLiteral("▶ 执行激励脚本"), this);
    m_btnRun->setObjectName(QStringLiteral("btnPrimary"));
    m_btnSimulate = new QPushButton(QStringLiteral("⚡ 注入并执行覆盖仿真"), this);
    argsLayout->addWidget(m_btnRun);
    argsLayout->addWidget(m_btnSimulate);

    modeLayout->addLayout(argsLayout);
    mainLayout->addWidget(grpMode);

    // 3. 结果选项卡：激励测试向量表 / 进程控制台输出
    m_tabs = new QTabWidget(this);

    // Tab 1: 向量列表
    m_tableVectors = new QTableWidget(this);
    m_tableVectors->setColumnCount(4);
    m_tableVectors->setHorizontalHeaderLabels({
        QStringLiteral("步骤"),
        QStringLiteral("时标 (ms)"),
        QStringLiteral("激励信号键值对"),
        QStringLiteral("原始报文 Payload")
    });
    m_tableVectors->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableVectors->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tableVectors->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableVectors->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableVectors->setAlternatingRowColors(true);
    m_tabs->addTab(m_tableVectors, QStringLiteral("激励测试向量 (Vectors)"));

    // Tab 2: 控制台输出
    m_txtLog = new QTextEdit(this);
    m_txtLog->setReadOnly(true);
    m_tabs->addTab(m_txtLog, QStringLiteral("Python 控制台输出 (Console)"));

    mainLayout->addWidget(m_tabs);

    // 状态栏
    m_lblStatus = new QLabel(QStringLiteral("就绪。可选择脚本文件或在输入框编写代码后执行。"), this);
    mainLayout->addWidget(m_lblStatus);

    // 默认载入编辑器初始模板
    onLoadTemplateToEditor(0);

    // 信号槽连接
    connect(m_radioFileMode, &QRadioButton::toggled, this, &StimulusView::onModeToggled);
    connect(m_radioEditorMode, &QRadioButton::toggled, this, &StimulusView::onModeToggled);
    connect(m_btnAutoDetectPython, &QPushButton::clicked, this, &StimulusView::onAutoDetectPython);
    connect(m_btnBrowseScript, &QPushButton::clicked, this, &StimulusView::onBrowseScript);
    connect(m_cmbScriptPresets, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StimulusView::onScriptSelected);
    connect(m_btnLoadTemplate, &QPushButton::clicked, this, [this]() {
        onLoadTemplateToEditor(m_cmbEditorTemplates->currentIndex());
    });
    connect(m_btnCheckSyntax, &QPushButton::clicked, this, &StimulusView::onCheckSyntaxClicked);
    connect(m_btnRun, &QPushButton::clicked, this, &StimulusView::onRunClicked);
    connect(m_btnSimulate, &QPushButton::clicked, this, &StimulusView::onSimulateClicked);
    connect(m_txtPythonPath, &QLineEdit::textChanged, this, &StimulusView::pythonPathChanged);

    applyTheme(ThemeManager::currentTheme());
}

void StimulusView::applyTheme(ThemeType type) {
    m_currentTheme = type;
    bool isLight = (type == ThemeType::LightModern);

    if (m_txtCodeEditor) {
        m_txtCodeEditor->setStyleSheet(ThemeManager::getEditorStyleSheet(type));
    }

    if (m_txtLog) {
        if (isLight) {
            m_txtLog->setStyleSheet(QStringLiteral("background: #ffffff; color: #202124; font-family: 'Consolas', monospace; border: 1px solid #dadce0; border-radius: 4px; padding: 4px;"));
        } else {
            m_txtLog->setStyleSheet(QStringLiteral("background: #11111b; color: #a6adc8; font-family: 'Consolas', monospace; border: 1px solid #313244; border-radius: 4px; padding: 4px;"));
        }
    }

    if (m_lblSyntaxResult) {
        if (isLight) {
            m_lblSyntaxResult->setStyleSheet(QStringLiteral("color: #5f6368; padding: 4px; background: #f1f3f4; border: 1px solid #dadce0; border-radius: 4px;"));
        } else {
            m_lblSyntaxResult->setStyleSheet(QStringLiteral("color: #a6adc8; padding: 4px; background: #11111b; border: 1px solid #313244; border-radius: 4px;"));
        }
    }

    if (m_lblStatus) {
        m_lblStatus->setStyleSheet(isLight ? QStringLiteral("color: #5f6368;") : QStringLiteral("color: #a6adc8;"));
    }

    QString tableStyle = isLight ?
        QStringLiteral(
            "QTableWidget { background-color: #ffffff; alternate-background-color: #f8f9fa; border: 1px solid #dadce0; color: #202124; gridline-color: #e8eaed; outline: none; }"
            "QHeaderView::section { background-color: #f1f3f4; color: #1a73e8; font-weight: bold; border: 1px solid #dadce0; padding: 6px; }"
            "QTableWidget::item:selected { background-color: #e8f0fe; color: #1a73e8; font-weight: 500; }"
            "QTableCornerButton::section { background-color: #f1f3f4; border: 1px solid #dadce0; }"
        ) :
        QStringLiteral(
            "QTableWidget { background-color: #1e1e2e; alternate-background-color: #181825; border: 1px solid #313244; color: #cdd6f4; gridline-color: #313244; outline: none; }"
            "QHeaderView::section { background-color: #181825; color: #89b4fa; font-weight: bold; border: 1px solid #313244; padding: 6px; }"
            "QTableWidget::item:selected { background-color: #45475a; color: #a6e3a1; font-weight: 500; }"
            "QTableCornerButton::section { background-color: #181825; border: 1px solid #313244; }"
        );
    if (m_tableVectors) m_tableVectors->setStyleSheet(tableStyle);

    if (m_tabs) {
        if (isLight) {
            m_tabs->setStyleSheet(QStringLiteral(
                "QTabWidget::pane { border: 1px solid #dadce0; background: #ffffff; border-radius: 4px; }"
                "QTabBar::tab { background: #f1f3f4; color: #5f6368; padding: 6px 14px; border: 1px solid #dadce0; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
                "QTabBar::tab:selected { background: #ffffff; color: #1a73e8; font-weight: bold; border-bottom: 2px solid #1a73e8; }"
                "QTabBar::tab:hover:!selected { background: #e8eaed; }"
            ));
        } else {
            m_tabs->setStyleSheet(QStringLiteral(
                "QTabWidget::pane { border: 1px solid #313244; background: #1e1e2e; border-radius: 4px; }"
                "QTabBar::tab { background: #181825; color: #a6adc8; padding: 6px 14px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
                "QTabBar::tab:selected { background: #1e1e2e; color: #89b4fa; font-weight: bold; border-bottom: 2px solid #89b4fa; }"
                "QTabBar::tab:hover:!selected { background: #262738; }"
            ));
        }
    }
}

void StimulusView::onModeToggled() {
    bool isFile = m_radioFileMode->isChecked();
    m_widgetFileMode->setVisible(isFile);
    m_widgetEditorMode->setVisible(!isFile);
}

void StimulusView::loadPresetScripts() {
    reloadProjectScripts();
}

void StimulusView::reloadProjectScripts() {
    m_cmbScriptPresets->blockSignals(true);
    m_cmbScriptPresets->clear();

    QStringList foundScripts;

    if (ProjectManager::instance().hasActiveProject()) {
        const auto& proj = ProjectManager::instance().currentProject();
        for (const QString& sf : proj.scriptFiles) {
            QString clean = QDir::cleanPath(sf);
            if (QFile::exists(clean) && !foundScripts.contains(clean)) {
                foundScripts.append(clean);
            }
        }
        if (!proj.rootDir.isEmpty()) {
            QDir scriptDir(proj.rootDir + QStringLiteral("/Script"));
            if (!scriptDir.exists()) scriptDir = QDir(proj.rootDir + QStringLiteral("/script"));
            if (scriptDir.exists()) {
                for (const auto& fi : scriptDir.entryInfoList({QStringLiteral("*.py")}, QDir::Files)) {
                    QString fp = QDir::cleanPath(fi.absoluteFilePath());
                    if (!foundScripts.contains(fp)) foundScripts.append(fp);
                }
            }
        }
    }

    // Fallback if no project opened or no scripts in project
    if (foundScripts.isEmpty()) {
        QDir exScriptDir(QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis/examples/motor_controller/Script"));
        if (exScriptDir.exists()) {
            for (const auto& fi : exScriptDir.entryInfoList({QStringLiteral("*.py")}, QDir::Files)) {
                QString fp = QDir::cleanPath(fi.absoluteFilePath());
                if (!foundScripts.contains(fp)) foundScripts.append(fp);
            }
        }
    }

    if (foundScripts.isEmpty()) {
        m_cmbScriptPresets->addItem(QStringLiteral("(当前工程目录下暂无 .py 脚本)"), QString());
    } else {
        for (const QString& fp : foundScripts) {
            QFileInfo fi(fp);
            m_cmbScriptPresets->addItem(fi.fileName() + QString(" [%1]").arg(QDir::toNativeSeparators(fp)), fp);
        }
    }
    m_cmbScriptPresets->blockSignals(false);

    if (m_cmbScriptPresets->count() > 0 && !m_cmbScriptPresets->itemData(0).toString().isEmpty()) {
        onScriptSelected(0);
    }
}

void StimulusView::onScriptSelected(int index) {
    if (index >= 0) {
        QString scriptPath = m_cmbScriptPresets->itemData(index).toString();
        m_txtCustomScript->setText(QDir::toNativeSeparators(scriptPath));
    }
}

void StimulusView::loadScriptFile(const QString& filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) return;

    m_radioFileMode->setChecked(true);
    onModeToggled();
    m_txtCustomScript->setText(QDir::toNativeSeparators(filePath));

    // 同时将脚本内容读入在线编辑器供查看和在线调试
    QFile f(filePath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_txtCodeEditor->setPlainText(QString::fromUtf8(f.readAll()));
        f.close();
    }
}

void StimulusView::onLoadTemplateToEditor(int index) {
    QString code;
    if (index == 0) {
        code = QStringLiteral(
R"(#!/usr/bin/env python3
import json, math

def generate_stimulus():
    """ 生成传感器连续正弦波形与过温过压故障激励 """
    vectors = []
    for i in range(50):
        t_ms = i * 20.0
        t_sec = t_ms / 1000.0
        
        voltage = 1.65 + 1.2 * math.sin(2.0 * math.pi * 0.5 * t_sec)
        temp = 25.0 + 3.5 * t_sec
        fault = 0.0
        
        if 20 <= i <= 25:
            temp = 108.5  # 注入过温故障
            fault = 1.0

        vectors.append({
            "timestampMs": round(t_ms, 2),
            "signals": {
                "adc_voltage": round(voltage, 3),
                "temp_celsius": round(temp, 2),
                "fault_flags": fault
            },
            "rawPayload": f"ADC={voltage:.2f}V, Temp={temp:.1f}C"
        })
        
    return {
        "name": "在线传感器故障注入",
        "targetModule": "motor_controller.c",
        "description": "自定义在线编写的传感器测试向量序列",
        "durationMs": 50 * 20.0,
        "signalNames": ["adc_voltage", "temp_celsius", "fault_flags"],
        "steps": vectors
    }

if __name__ == "__main__":
    plan = generate_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
)");
    } else if (index == 1) {
        code = QStringLiteral(
R"(#!/usr/bin/env python3
import json

def generate_stimulus():
    """ 生成车载 CAN 总线周期速度报文与急停注入 """
    vectors = []
    for i in range(40):
        t_ms = i * 25.0
        target_rpm = min(3000, i * 100)
        e_stop = 1.0 if (25 <= i <= 30) else 0.0
        
        vectors.append({
            "timestampMs": float(t_ms),
            "signals": {
                "can_id": 0x120,
                "target_rpm": float(target_rpm),
                "e_stop": float(e_stop)
            },
            "rawPayload": f"CAN_ID=0x120, RPM={target_rpm}, E_STOP={int(e_stop)}"
        })
        
    return {
        "name": "在线 CAN 报文激励",
        "targetModule": "motor_controller.c",
        "description": "周期 CAN 指令与突发刹车信号",
        "durationMs": 40 * 25.0,
        "signalNames": ["can_id", "target_rpm", "e_stop"],
        "steps": vectors
    }

if __name__ == "__main__":
    plan = generate_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
)");
    } else {
        code = QStringLiteral(
R"(#!/usr/bin/env python3
import json

def generate_stimulus():
    """ 边界值模糊测试 (触发极值判断分支) """
    boundary_cases = [
        {"speed": 0.0, "temp": 25.0, "fault": 0.0},
        {"speed": 3500.0, "temp": 45.0, "fault": 0.0},
        {"speed": 1200.0, "temp": 115.0, "fault": 1.0},
        {"speed": -50.0, "temp": 30.0, "fault": 4.0},
    ]
    vectors = []
    for idx, c in enumerate(boundary_cases):
        vectors.append({
            "timestampMs": idx * 50.0,
            "signals": {
                "speed_rpm": c["speed"],
                "temp_celsius": c["temp"],
                "fault_flags": c["fault"]
            },
            "rawPayload": f"FUZZ: RPM={c['speed']}, Temp={c['temp']}"
        })
    return {
        "name": "在线边界值覆盖激励",
        "targetModule": "motor_controller.c",
        "description": "针对边缘条件促成分支全覆盖",
        "durationMs": len(vectors) * 50.0,
        "signalNames": ["speed_rpm", "temp_celsius", "fault_flags"],
        "steps": vectors
    }

if __name__ == "__main__":
    plan = generate_stimulus()
    print("[STIMULUS_START]")
    print(json.dumps(plan, indent=2))
    print("[STIMULUS_END]")
)");
    }

    m_txtCodeEditor->setPlainText(code.trimmed());
    m_lblSyntaxResult->setText(QStringLiteral("已载入模板代码，尚未进行语法检查。"));
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    m_lblSyntaxResult->setStyleSheet(isLight ?
        QStringLiteral("color: #5f6368; padding: 4px; background: #f1f3f4; border: 1px solid #dadce0; border-radius: 4px;") :
        QStringLiteral("color: #a6adc8; padding: 4px; background: #11111b; border: 1px solid #313244; border-radius: 4px;"));
}

void StimulusView::onCheckSyntaxClicked() {
    QString code = m_txtCodeEditor->toPlainText();
    QString errorMsg;
    int errLine = 0, errCol = 0;

    bool ok = BackendService::instance().stimulusEngine().checkPythonSyntax(code, &errorMsg, &errLine, &errCol);
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    if (ok) {
        QString okColor = isLight ? QStringLiteral("#16a34a") : QStringLiteral("#a6e3a1");
        m_lblSyntaxResult->setText(QString("<font color='%1'><b>✔ 语法与语义检查通过 (Valid Python AST)</b> - 代码结构完好，可安全执行</font>").arg(okColor));
    } else {
        QString errColor = isLight ? QStringLiteral("#dc2626") : QStringLiteral("#f38ba8");
        m_lblSyntaxResult->setText(QString("<font color='%1'><b>✖ %2</b></font>").arg(errColor, errorMsg.toHtmlEscaped()));
    }
}

void StimulusView::onBrowseScript() {
    QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择 Python 激励生成脚本"),
                                                QString(), QStringLiteral("Python Scripts (*.py)"));
    if (!file.isEmpty()) {
        m_txtCustomScript->setText(QDir::toNativeSeparators(file));
    }
}

void StimulusView::onAutoDetectPython() {
    QStringList candidates;

    // 1. Workspace bundled / embedded python
    QString localPy = QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/tools/python/python.exe"));
    if (QFile::exists(localPy)) candidates.append(localPy);
    QString wsPy = QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis/tools/python/python.exe");
    if (QFile::exists(wsPy) && !candidates.contains(wsPy)) candidates.append(wsPy);

    // 2. Query where.exe python and where.exe py via QProcess
    QProcess proc;
    proc.start(QStringLiteral("where.exe"), {QStringLiteral("python")});
    if (proc.waitForFinished(2500)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput());
        for (const QString& line : out.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts)) {
            QString clean = QDir::cleanPath(line.trimmed());
            if (QFile::exists(clean) && !clean.contains(QStringLiteral("WindowsApps"), Qt::CaseInsensitive)) {
                if (!candidates.contains(clean)) candidates.append(clean);
            }
        }
    }

    // 3. Scan common Windows paths
    QString userProfile = qEnvironmentVariable("USERPROFILE");
    QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    QString programFiles = qEnvironmentVariable("ProgramFiles");

    QStringList searchDirs = {
        localAppData + QStringLiteral("/Programs/Python"),
        QStringLiteral("C:/"),
        programFiles,
        programFiles + QStringLiteral(" (x86)"),
        QStringLiteral("C:/ProgramData/Anaconda3"),
        QStringLiteral("C:/ProgramData/miniconda3"),
        userProfile + QStringLiteral("/anaconda3"),
        userProfile + QStringLiteral("/miniconda3")
    };

    for (const QString& sd : searchDirs) {
        if (!QDir(sd).exists()) continue;
        QDir dir(sd);
        if (QFile::exists(dir.absoluteFilePath(QStringLiteral("python.exe")))) {
            QString p = QDir::cleanPath(dir.absoluteFilePath(QStringLiteral("python.exe")));
            if (!candidates.contains(p)) candidates.append(p);
        }
        for (const auto& fi : dir.entryInfoList({QStringLiteral("Python*"), QStringLiteral("python*")}, QDir::Dirs)) {
            QString p = QDir::cleanPath(fi.absoluteFilePath() + QStringLiteral("/python.exe"));
            if (QFile::exists(p) && !candidates.contains(p)) {
                candidates.append(p);
            }
        }
    }

    // 4. Test each candidate to retrieve its version string
    QStringList displayItems;
    QStringList validPaths;
    for (const QString& pyPath : candidates) {
        QProcess vProc;
        vProc.start(pyPath, {QStringLiteral("--version")});
        QString ver = QStringLiteral("Python 3.x");
        if (vProc.waitForFinished(1500)) {
            QString verOut = QString::fromUtf8(vProc.readAllStandardOutput()).trimmed();
            if (verOut.isEmpty()) verOut = QString::fromUtf8(vProc.readAllStandardError()).trimmed();
            if (!verOut.isEmpty()) ver = verOut;
        }
        displayItems.append(QString("%1  -  %2").arg(ver, QDir::toNativeSeparators(pyPath)));
        validPaths.append(pyPath);
    }

    displayItems.append(QStringLiteral("📁 [手动浏览指定其他 Python 路径...]"));

    bool ok = false;
    int currentIdx = 0;
    QString currentPath = m_txtPythonPath->text().trimmed();
    for (int i = 0; i < validPaths.size(); ++i) {
        if (validPaths[i].compare(currentPath, Qt::CaseInsensitive) == 0) {
            currentIdx = i;
            break;
        }
    }

    QString selected = QInputDialog::getItem(
        this,
        QStringLiteral("选择 Python 解释器环境"),
        QStringLiteral("在当前电脑中检索到以下可用 Python 解释器环境，请选择需要使用哪一个："),
        displayItems,
        currentIdx,
        false,
        &ok
    );

    if (ok && !selected.isEmpty()) {
        int selIdx = displayItems.indexOf(selected);
        if (selIdx == displayItems.size() - 1) {
            // 手动浏览选择
            QString custom = QFileDialog::getOpenFileName(
                this,
                QStringLiteral("定位 python.exe"),
                QString(),
                QStringLiteral("Python Executable (python.exe);;All Files (*.*)")
            );
            if (!custom.isEmpty()) {
                m_txtPythonPath->setText(QDir::toNativeSeparators(custom));
                emit pythonPathChanged(custom);
            }
        } else if (selIdx >= 0 && selIdx < validPaths.size()) {
            QString chosen = validPaths[selIdx];
            m_txtPythonPath->setText(QDir::toNativeSeparators(chosen));
            emit pythonPathChanged(chosen);
        }
    }
}

void StimulusView::onRunClicked() {
    QStringList args;
    QString rawArgs = m_txtArgs->text().trimmed();
    if (!rawArgs.isEmpty()) {
        args = rawArgs.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    }

    clearLog();

    if (m_radioFileMode->isChecked()) {
        // 模式 1: 执行本地脚本文件
        QString script = m_txtCustomScript->text().trimmed();
        if (script.isEmpty() || !QFile::exists(script)) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("指定的 Python 脚本文件不存在，请重新选择。"));
            return;
        }
        m_lblStatus->setText(QStringLiteral("正在后台运行指定脚本文件..."));
        emit runScriptRequested(script, args);
    } else {
        // 模式 2: 在线编辑执行 (严格前置语法语义检查)
        QString code = m_txtCodeEditor->toPlainText();
        QString syntaxError;
        int errLine = 0, errCol = 0;

        bool syntaxOk = BackendService::instance().stimulusEngine().checkPythonSyntax(code, &syntaxError, &errLine, &errCol);
        if (!syntaxOk) {
            m_lblSyntaxResult->setText(QString("<font color='#f38ba8'><b>✖ %1</b></font>").arg(syntaxError.toHtmlEscaped()));
            QMessageBox::critical(this, QStringLiteral("Python 语法错误，禁止执行"),
                                  QString("编写的 Python 代码存在语法/语义错误，为防止程序崩溃已中止执行：\n\n%1").arg(syntaxError));
            return;
        }

        m_lblSyntaxResult->setText(QStringLiteral("<font color='#a6e3a1'><b>✔ 语法语义校验通过，已启动执行...</b></font>"));
        m_lblStatus->setText(QStringLiteral("正在执行输入框中的 Python 激励代码..."));
        emit runCodeContentRequested(code, args);
    }
}

void StimulusView::onSimulateClicked() {
    emit simulateCoverageRequested();
}

void StimulusView::setPythonExecutable(const QString& path) {
    m_txtPythonPath->setText(path);
}

QString StimulusView::pythonExecutable() const {
    return m_txtPythonPath->text().trimmed();
}

void StimulusView::appendLog(const QString& text) {
    m_txtLog->append(text);
}

void StimulusView::clearLog() {
    m_txtLog->clear();
}

void StimulusView::setStimulusPlan(const StimulusPlan& plan) {
    updateVectorsTable(plan);
    m_lblStatus->setText(QString("激励时序向量已注入: %1 组测试向量 (总时长: %2 ms)")
                             .arg(plan.steps.size()).arg(plan.durationMs));
}

void StimulusView::updateVectorsTable(const StimulusPlan& plan) {
    m_tableVectors->setRowCount(0);

    for (int i = 0; i < plan.steps.size(); ++i) {
        const auto& step = plan.steps[i];
        m_tableVectors->insertRow(i);

        auto* itemStep = new QTableWidgetItem(QString::number(i + 1));
        auto* itemTime = new QTableWidgetItem(QString::number(step.timestampMs, 'f', 1));

        QStringList sigList;
        for (auto it = step.signalValues.constBegin(); it != step.signalValues.constEnd(); ++it) {
            sigList.append(QString("%1: %2").arg(it.key()).arg(it.value()));
        }
        auto* itemSignals = new QTableWidgetItem(sigList.join(QStringLiteral(", ")));
        auto* itemPayload = new QTableWidgetItem(step.rawPayload.isEmpty() ? QStringLiteral("-") : step.rawPayload);

        m_tableVectors->setItem(i, 0, itemStep);
        m_tableVectors->setItem(i, 1, itemTime);
        m_tableVectors->setItem(i, 2, itemSignals);
        m_tableVectors->setItem(i, 3, itemPayload);
    }
}

} // namespace Coverage
