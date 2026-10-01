#pragma once

#include "coverage/models.h"
#include "theme_manager.h"
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>

namespace Coverage {

/**
 * @brief Python 激励注入视图
 * 支持双模式注入：
 * 1. 外部 .py 文件选择与自动执行
 * 2. 在线输入框编写 Python 脚本，支持前置语法语义检查，防止代码不可用
 */
class StimulusView : public QWidget {
    Q_OBJECT
public:
    explicit StimulusView(QWidget* parent = nullptr);

    void setPythonExecutable(const QString& path);
    QString pythonExecutable() const;

    void setStimulusPlan(const StimulusPlan& plan);
    void loadScriptFile(const QString& filePath);
    void reloadProjectScripts();
    void appendLog(const QString& text);
    void clearLog();
    void applyTheme(ThemeType type = ThemeType::DarkCatppuccin);

signals:
    void runScriptRequested(const QString& scriptPath, const QStringList& args);
    void runCodeContentRequested(const QString& pythonCode, const QStringList& args);
    void simulateCoverageRequested();
    void pythonPathChanged(const QString& path);

private slots:
    void onModeToggled();
    void onBrowseScript();
    void onAutoDetectPython();
    void onScriptSelected(int index);
    void onLoadTemplateToEditor(int index);
    void onCheckSyntaxClicked();
    void onRunClicked();
    void onSimulateClicked();

private:
    void setupUi();
    void loadPresetScripts();
    void updateVectorsTable(const StimulusPlan& plan);

    // 运行模式切换
    QRadioButton* m_radioFileMode;
    QRadioButton* m_radioEditorMode;
    QButtonGroup* m_modeGroup;

    // Python 环境配置
    QLineEdit* m_txtPythonPath;
    QPushButton* m_btnAutoDetectPython;

    // 模式1：文件选择控件
    QWidget* m_widgetFileMode;
    QComboBox* m_cmbScriptPresets;
    QLineEdit* m_txtCustomScript;
    QPushButton* m_btnBrowseScript;

    // 模式2：在线代码编辑器控件
    QWidget* m_widgetEditorMode;
    QComboBox* m_cmbEditorTemplates;
    QPushButton* m_btnLoadTemplate;
    QPushButton* m_btnCheckSyntax;
    QLabel* m_lblSyntaxResult;
    QPlainTextEdit* m_txtCodeEditor;

    // 公共执行参数与控制按钮
    QLineEdit* m_txtArgs;
    QPushButton* m_btnRun;
    QPushButton* m_btnSimulate;

    // 结果展示选项卡 (测试向量表 / 控制台输出)
    QTableWidget* m_tableVectors = nullptr;
    QTextEdit* m_txtLog = nullptr;
    QLabel* m_lblStatus = nullptr;
    QTabWidget* m_tabs = nullptr;
    ThemeType m_currentTheme = ThemeType::DarkCatppuccin;
};

} // namespace Coverage
