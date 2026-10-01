#include "localization_manager.h"

namespace Coverage {

LocalizationManager& LocalizationManager::instance() {
    static LocalizationManager s_instance;
    return s_instance;
}

LocalizationManager::LocalizationManager() {
    initDictionary();
}

void LocalizationManager::setLanguage(Language lang) {
    if (m_currentLanguage != lang) {
        m_currentLanguage = lang;
        emit languageChanged(lang);
    }
}

QString LocalizationManager::text(const QString& key) const {
    if (m_currentLanguage == Language::Chinese) {
        return m_dictZh.value(key, key);
    } else {
        return m_dictEn.value(key, key);
    }
}

void LocalizationManager::initDictionary() {
    // 1. 简体中文词库 (默认)
    m_dictZh[QStringLiteral("app.title")] = QStringLiteral("嵌入式 C 代码覆盖分析与静态分析套件");
    m_dictZh[QStringLiteral("menu.file")] = QStringLiteral("文件 (&F)");
    m_dictZh[QStringLiteral("menu.analysis")] = QStringLiteral("分析 (&A)");
    m_dictZh[QStringLiteral("menu.stimulus")] = QStringLiteral("激励 (&S)");
    m_dictZh[QStringLiteral("menu.view")] = QStringLiteral("视图 (&V)");
    m_dictZh[QStringLiteral("menu.settings")] = QStringLiteral("设置 (&T)");
    m_dictZh[QStringLiteral("menu.language")] = QStringLiteral("软件语言 (Language)");
    m_dictZh[QStringLiteral("menu.theme")] = QStringLiteral("界面风格主题 (Theme)");
    m_dictZh[QStringLiteral("menu.help")] = QStringLiteral("帮助 (&H)");

    m_dictZh[QStringLiteral("act.open_file")] = QStringLiteral("打开单个 C 文件... (&O)");
    m_dictZh[QStringLiteral("act.import_dir")] = QStringLiteral("导入路径下所有源文件... (&I)");
    m_dictZh[QStringLiteral("act.save")] = QStringLiteral("保存文件 (&S)");
    m_dictZh[QStringLiteral("act.export_report")] = QStringLiteral("导出综合分析报告... (&E)");
    m_dictZh[QStringLiteral("act.exit")] = QStringLiteral("退出 (&X)");

    m_dictZh[QStringLiteral("act.run_static")] = QStringLiteral("执行静态代码分析 (F5)");
    m_dictZh[QStringLiteral("act.instrument")] = QStringLiteral("执行源码探针插桩 (F6)");
    m_dictZh[QStringLiteral("act.run_stimulus")] = QStringLiteral("执行 Python 激励生成 (F7)");
    m_dictZh[QStringLiteral("act.run_sim")] = QStringLiteral("执行覆盖仿真内核 (F8)");
    m_dictZh[QStringLiteral("act.user_manual")] = QStringLiteral("软件使用说明文档... (F1)");
    m_dictZh[QStringLiteral("act.view_log")] = QStringLiteral("查看系统运行日志文件... (Ctrl+L)");
    m_dictZh[QStringLiteral("act.open_log_dir")] = QStringLiteral("打开日志存储目录...");
    m_dictZh[QStringLiteral("act.about")] = QStringLiteral("关于此软件 (&A)...");

    m_dictZh[QStringLiteral("tab.source_code")] = QStringLiteral("C 源代码");
    m_dictZh[QStringLiteral("tab.static_explorer")] = QStringLiteral("静态分析与路径探索");
    m_dictZh[QStringLiteral("tab.graph_view")] = QStringLiteral("代码结构拓扑图 (Graphviz)");
    m_dictZh[QStringLiteral("tab.static_issues")] = QStringLiteral("静态分析问题列表");
    m_dictZh[QStringLiteral("tab.system_log")] = QStringLiteral("系统运行控制台日志");
    m_dictZh[QStringLiteral("tab.coverage_dash")] = QStringLiteral("覆盖率仪表盘");
    m_dictZh[QStringLiteral("tab.stimulus_studio")] = QStringLiteral("Python 激励工作台");

    m_dictZh[QStringLiteral("btn.run_static_analysis")] = QStringLiteral("⚡ 执行选定代码静态分析");
    m_dictZh[QStringLiteral("btn.add_file")] = QStringLiteral("添加文件");
    m_dictZh[QStringLiteral("btn.add_dir")] = QStringLiteral("添加目录");
    m_dictZh[QStringLiteral("btn.remove_file")] = QStringLiteral("移除选中");
    m_dictZh[QStringLiteral("act.run_py_script")] = QStringLiteral("▶ 执行此 Python 激励脚本");

    m_dictZh[QStringLiteral("dock.project_files")] = QStringLiteral("工程与源码文件树");
    m_dictZh[QStringLiteral("dock.code_editor")] = QStringLiteral("C 源代码编辑器");
    m_dictZh[QStringLiteral("dock.static_explorer")] = QStringLiteral("静态分析与路径探索");
    m_dictZh[QStringLiteral("dock.graph_view")] = QStringLiteral("代码结构拓扑图 (Graphviz)");
    m_dictZh[QStringLiteral("dock.coverage_stimulus")] = QStringLiteral("覆盖度量与激励注入");
    m_dictZh[QStringLiteral("dock.static_issues")] = QStringLiteral("静态分析缺陷列表");
    m_dictZh[QStringLiteral("dock.system_log")] = QStringLiteral("系统运行实时日志");
    m_dictZh[QStringLiteral("dock.analysis_console")] = QStringLiteral("静态分析与系统日志");
    m_dictZh[QStringLiteral("act.reset_layout")] = QStringLiteral("🔄 恢复默认窗口布局");

    m_dictZh[QStringLiteral("status.no_file")] = QStringLiteral("未加载源文件");
    m_dictZh[QStringLiteral("status.analysis_idle")] = QStringLiteral("静态分析: 空闲");
    m_dictZh[QStringLiteral("status.coverage_na")] = QStringLiteral("覆盖率: N/A");

    m_dictZh[QStringLiteral("lang.zh")] = QStringLiteral("简体中文 (Simplified Chinese)");
    m_dictZh[QStringLiteral("lang.en")] = QStringLiteral("English (英语)");

    m_dictZh[QStringLiteral("theme.catppuccin")] = QStringLiteral("深色科技风 (Catppuccin Mocha)");
    m_dictZh[QStringLiteral("theme.light")] = QStringLiteral("现代清新白 (Modern Light)");
    m_dictZh[QStringLiteral("theme.solarized")] = QStringLiteral("Solarized 暗夜 (Solarized Dark)");
    m_dictZh[QStringLiteral("theme.monokai")] = QStringLiteral("Monokai Pro 专业暗色 (Monokai Pro)");

    m_dictZh[QStringLiteral("lbl.active_file")] = QStringLiteral("活动文件:");
    m_dictZh[QStringLiteral("tree.imported")] = QStringLiteral("已导入源文件目录");
    m_dictZh[QStringLiteral("tree.examples")] = QStringLiteral("内置测试示例集");

    // 2. English Dictionary
    m_dictEn[QStringLiteral("app.title")] = QStringLiteral("Embedded C Coverage & Static Analysis Suite");
    m_dictEn[QStringLiteral("menu.file")] = QStringLiteral("&File");
    m_dictEn[QStringLiteral("menu.analysis")] = QStringLiteral("&Analysis");
    m_dictEn[QStringLiteral("menu.stimulus")] = QStringLiteral("&Stimulus");
    m_dictEn[QStringLiteral("menu.view")] = QStringLiteral("&View");
    m_dictEn[QStringLiteral("menu.settings")] = QStringLiteral("&Settings");
    m_dictEn[QStringLiteral("menu.language")] = QStringLiteral("&Language");
    m_dictEn[QStringLiteral("menu.theme")] = QStringLiteral("&Theme Style");
    m_dictEn[QStringLiteral("menu.help")] = QStringLiteral("&Help");

    m_dictEn[QStringLiteral("act.open_file")] = QStringLiteral("&Open Single C File...");
    m_dictEn[QStringLiteral("act.import_dir")] = QStringLiteral("&Import All Source Files in Directory...");
    m_dictEn[QStringLiteral("act.save")] = QStringLiteral("&Save File");
    m_dictEn[QStringLiteral("act.export_report")] = QStringLiteral("&Export Comprehensive Report...");
    m_dictEn[QStringLiteral("act.exit")] = QStringLiteral("E&xit");

    m_dictEn[QStringLiteral("act.run_static")] = QStringLiteral("Run &Static Analysis (F5)");
    m_dictEn[QStringLiteral("act.instrument")] = QStringLiteral("&Instrument Source Code (F6)");
    m_dictEn[QStringLiteral("act.run_stimulus")] = QStringLiteral("Execute &Python Stimulus (F7)");
    m_dictEn[QStringLiteral("act.run_sim")] = QStringLiteral("Run Coverage &Simulation (F8)");
    m_dictEn[QStringLiteral("act.user_manual")] = QStringLiteral("&User Manual & Guide (F1)...");
    m_dictEn[QStringLiteral("act.view_log")] = QStringLiteral("&View System Runtime Log... (Ctrl+L)");
    m_dictEn[QStringLiteral("act.open_log_dir")] = QStringLiteral("&Open Log Directory...");
    m_dictEn[QStringLiteral("act.about")] = QStringLiteral("&About This Suite...");

    m_dictEn[QStringLiteral("tab.source_code")] = QStringLiteral("C Source Code");
    m_dictEn[QStringLiteral("tab.static_explorer")] = QStringLiteral("Static Analysis & Path Explorer");
    m_dictEn[QStringLiteral("tab.graph_view")] = QStringLiteral("Structure Topology (Graphviz)");
    m_dictEn[QStringLiteral("tab.static_issues")] = QStringLiteral("Static Analysis Issues");
    m_dictEn[QStringLiteral("tab.system_log")] = QStringLiteral("System Console Log");
    m_dictEn[QStringLiteral("tab.coverage_dash")] = QStringLiteral("Coverage Dashboard");
    m_dictEn[QStringLiteral("tab.stimulus_studio")] = QStringLiteral("Python Stimulus Studio");

    m_dictEn[QStringLiteral("btn.run_static_analysis")] = QStringLiteral("⚡ Run Static Code Analysis");
    m_dictEn[QStringLiteral("btn.add_file")] = QStringLiteral("Add File");
    m_dictEn[QStringLiteral("btn.add_dir")] = QStringLiteral("Add Directory");
    m_dictEn[QStringLiteral("btn.remove_file")] = QStringLiteral("Remove Selected");
    m_dictEn[QStringLiteral("act.run_py_script")] = QStringLiteral("▶ Run This Python Stimulus Script");

    m_dictEn[QStringLiteral("dock.project_files")] = QStringLiteral("Project Files Tree");
    m_dictEn[QStringLiteral("dock.code_editor")] = QStringLiteral("C Source Code Editor");
    m_dictEn[QStringLiteral("dock.static_explorer")] = QStringLiteral("Static Analysis & Path Explorer");
    m_dictEn[QStringLiteral("dock.graph_view")] = QStringLiteral("Code Structure Topology (Graphviz)");
    m_dictEn[QStringLiteral("dock.coverage_stimulus")] = QStringLiteral("Coverage & Stimulus Workbench");
    m_dictEn[QStringLiteral("dock.static_issues")] = QStringLiteral("Static Analysis Issues");
    m_dictEn[QStringLiteral("dock.system_log")] = QStringLiteral("System Runtime Log");
    m_dictEn[QStringLiteral("dock.analysis_console")] = QStringLiteral("Analysis Issues & Console");
    m_dictEn[QStringLiteral("act.reset_layout")] = QStringLiteral("🔄 Reset Default Window Layout");

    m_dictEn[QStringLiteral("status.no_file")] = QStringLiteral("No file loaded");
    m_dictEn[QStringLiteral("status.analysis_idle")] = QStringLiteral("Static Analysis: Idle");
    m_dictEn[QStringLiteral("status.coverage_na")] = QStringLiteral("Coverage: N/A");

    m_dictEn[QStringLiteral("lang.zh")] = QStringLiteral("Simplified Chinese (简体中文)");
    m_dictEn[QStringLiteral("lang.en")] = QStringLiteral("English");

    m_dictEn[QStringLiteral("theme.catppuccin")] = QStringLiteral("Dark Tech (Catppuccin Mocha)");
    m_dictEn[QStringLiteral("theme.light")] = QStringLiteral("Modern Light");
    m_dictEn[QStringLiteral("theme.solarized")] = QStringLiteral("Solarized Dark");
    m_dictEn[QStringLiteral("theme.monokai")] = QStringLiteral("Monokai Pro");

    m_dictEn[QStringLiteral("lbl.active_file")] = QStringLiteral("Active File:");
    m_dictEn[QStringLiteral("tree.imported")] = QStringLiteral("Imported Source Directory");
    m_dictEn[QStringLiteral("tree.examples")] = QStringLiteral("Built-in Test Examples");
}

} // namespace Coverage
