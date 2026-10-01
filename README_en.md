# Embedded C Coverage Analysis & Static Analysis Suite (C++ Qt)

An industrial-grade, safety-critical software engineering platform tailored for automotive electronics, aerospace avionics, and industrial automation. It provides integrated **Static Code Analysis**, **Software Metrics (LOC, Statements, Branches, Comment Ratio, MC/DC, Fan-In/Fan-Out, Function & Call Depth)**, **Independent Function Path Extraction with Synchronized Green Highlight Exploration**, **Static Stack & Worst-Case Call Depth Analysis (Stack Analyzer)**, **Source-Level Coverage Probe Instrumentation (Excluding Variable Declarations)**, **Dedicated Test Coverage Analysis Studio (Tables, Charts, Code Highlighting & Legends)**, **Multi-Scenario Python Stimulus Injection (Interactive Environment Scanner)**, **Embedded Read-Only Report Viewer (HTML/PDF/Word/JSON)**, **Lifecycle Audit Logging**, and **Clean Standardized Project Directory Physical Isolation**.

---

## 🌟 Core Features & System Architecture (v2.7.0 Enterprise Edition)

### 1. Unopened Project State Mutual Exclusion & Welcome Wizard
* **Clean Startup & Unopened Project Protection**: When launched without an active project, the left project tree remains empty, the central workspace **displays only the Welcome Wizard and the bottom system log**, while workspace editors, static analyzers, graph visualizers, and stack analyzers remain hidden to prevent accidental clicks or invalid calculations.
* **Mutual Exclusion Protection for Menus and Toolbar**: When no project is active, all operational menu items (Save Project, Close Project, Save File, Export Report, Run Static Analysis, Instrument Code, Stack Analysis, Run Simulation, Run Stimulus, etc.) are **strictly disabled**; the main control toolbar **displays only "Project Hub" and "Open Project"**, while all other action buttons, separators, and the file switcher combobox are automatically hidden.
* **Seamless Workspace Activation**: Opening or creating a project from the Welcome Wizard, Project Hub, or menu automatically closes the Welcome Wizard and reveals all primary workspace panels, fully restoring all toolbar buttons and menus.
* **Double-Click Recent Project Loading**: The Welcome Wizard displays recently opened projects with target hardware architecture, absolute path, and file readiness. **Double-click any entry to immediately restore the full analysis workspace**.

### 2. OS Appearance Mode Auto-Detection & Adaptive Theming
* **Operating System Theme Auto-Detection**: The suite automatically queries Windows registry settings on startup to detect the active system appearance mode:
  - Windows Light Mode: Automatically defaults to **"Modern Light" (现代清新白)**;
  - Windows Dark Mode: Automatically defaults to **"Dark Catppuccin" (深色科技风)**.
* **Pixel-Perfect Widget Harmonization**: All dialogs, cards, tables, tree views, and editors are fully styled to match the active theme, eliminating mismatched light/dark contrast boxes. Furthermore, the white focus outline around buttons upon clicking has been completely eliminated.
* **Four Refined Themes**: Seamless runtime switching between Dark Catppuccin Mocha, Modern Clean Light, Solarized Dark, and Monokai Pro.

### 3. Strict Project Directory Physical Isolation
* Enforces standard industrial embedded project hierarchy where each project is isolated in its own dedicated directory with 4 standardized subfolders:
  - `<ProjectName>/<ProjectName>.covproj`: Core project metadata and configuration file;
  - `Source/`: Dedicated embedded C/C++ implementation source files (*.c, *.cpp);
  - `Header/`: Dedicated header files (*.h);
  - `Script/`: Dedicated Python stimulus and test injection scripts (*.py);
  - `Report/`: Dedicated quality and coverage analysis export reports (*.html, *.docx, *.pdf, *.json);
* Completely eliminates cross-contamination between different project workspaces.

### 4. Static Stack & Worst-Case Call Depth Analysis (Static Stack Analyzer - Zero 3rd-Party EXE Dependency)
* **Safety Standard Background**: Tailored for **ISO 26262 ASIL-D** (automotive) and **DO-178C Level A** (avionics), providing non-intrusive static peak stack depth evaluation and call chain visualization with zero runtime instrumentation overhead.
* **100% In-Process Native C++ Engine (Zero External Tool Dependency)**:
  - Built-in native C++ ELF (32/64-bit) and Windows PE/COFF parser without invoking any external command-line binaries (zero dependency on `objdump.exe`);
  - In-memory extraction of symbols, sections, code segments, and exported routines;
  - Fuses GCC `-fstack-usage` `.su` static stack metrics; when `.su` is unavailable, the in-process instruction decoder automatically analyzes machine opcodes (`push`, `sub esp/rsp`, `stp`, `str`, etc.) and AST call topologies to derive accurate frame bounds.
* **DFS Directed Acyclic Graph (DAG) Worst-Case Solver**:
  - Traverses global call topologies via depth-first search to determine worst-case peak call stack depths and exact critical call chains;
  - Features an 8-column function metrics table: Index, Function Name, Source File, Line Number, Local Stack (Bytes), Max Call Stack (Bytes), Frame Type, and MISRA 17.2 Recursion Risk;
  - Clicking any function displays its assembly disassembly and stack allocation instructions.

### 5. Advanced Static Analysis Metrics & Path Exploration Studio
* **Comprehensive Function Metrics Expansion**:
  - Full support for calculating and displaying embedded code quality indicators: **Fan-In**, **Fan-Out**, **Function Nesting Depth**, and **Call Depth**;
  - **Tab 1: Function Metrics Overview**: Top KPI scoreboard (Statements, Branches, Comment Ratio, MC/DC Conditions, Cyclomatic Complexity, Defects). The comprehensive function table presents 11 core metrics: **Index, Function Name, Location, Statements, Branches, Cyclomatic Complexity, Fan-In, Fan-Out, Function Depth, Call Depth, MC/DC Count**.
* **Tab 2: Control Flow & Independent Path Exploration**:
  - Full list of linearly independent basis paths per function. Selecting a path dynamically highlights control flow edges (**bold emerald #10b981, 2.8px**) and active node borders (**bold emerald #10b981**);
  - The right-hand source viewer dynamically highlights lines traversed by the selected path in **bold green text (#10b981)** with a soft emerald translucent background.

### 6. Dedicated Test Coverage Analysis Studio
* **Dedicated Entrypoint & Distinct Vector Icon**:
  - Features a dedicated vector icon (`coverage_analysis.svg`) to distinguish it from the coverage simulation runner;
  - Accessible via keyboard shortcut `F8`, menu item "Analysis -> Test Coverage Analysis" (测试覆盖分析), and main toolbar;
* **Robust Statement, Branch & Function Coverage Calculations (No False Comment/Branch Coloring)**:
  - **Isolated Multi-File Coverage Reports**: Each project source file maintains its own isolated coverage report and line mapping, eliminating cross-file line index misalignment so comments and regular statements are never erroneously tinted as branch conditions;
  - **Strict Comment and Non-Executable Exclusion**: Block comments (`/* ... */`), single-line comments (`//`), leading comment asterisks, and preprocessor directives are stripped prior to instrumentation and strictly ignored during highlight rendering;
  - **Branch Keyword Validation**: Only code lines containing actual condition control keywords (`if`, `switch`, `while`, `for`, `?`) are eligible for branch status; normal statements are strictly categorized as Covered (Green) or Uncovered (Red), never Partial Branch (Yellow);
  - **Variable Declarations Excluded from Probes**: Pure variable declarations (e.g. `int a;`, `float v;`, `uint32_t flags;`, `static MotorState st;`) are identified and excluded from executable instrumentation probes and coverage hit counts;
* **Clear Coverage Color Legend (Legend Chips)**:
  - Prominent legend chips displayed directly above the code view:
    - `🟩 Covered Executable Statement (Covered)`
    - `🟨 Partial Branch Evaluation (Partial)`
    - `🟥 Uncovered Executable Line (Uncovered)`
    - `⬜ Non-Executable / Declaration / Comment (Ignored)`
* **Rapid Uncovered Code Navigation with Prominent Focus & Cursor Indicators**:
  - Clicking "Previous Uncovered" (◀ 上一未覆盖) or "Next Uncovered" (下一未覆盖 ▶) cycles through uncovered lines instantly;
  - **Prominent Line Selection**: The target line is fully selected with keyboard focus set, decorated with a high-contrast outline pen border (`#dc2626` / `#f38ba8`) and distinct tinted background;
  - **Gutter Direction Pointer Arrow**: A bright red arrow badge (`▶`) is drawn in the line-number gutter directly beside the navigated line;
  - **Real-Time Status Tooltip**: Displays feedback toasts such as `📍 Located uncovered/partial branch line: Line X` or `🎉 All lines covered!`.
* **Dual-Mode Filtering Panel (Left) & Automatic Mode Switching**:
  - Toggle between "File" and "Function" coverage views;
  - Double-clicking any file in "File" mode automatically tints coverage, **switches to "Function" mode, and displays all functions belonging to that source file**;
* **Dual-Tab Professional Views**:
  - **Tab 1: Coverage Statistics (覆盖统计)**: Full 9-column function coverage metrics table + vector chart with right-click menu to switch between **📊 Bar Chart**, **📈 Line Chart**, and **🥧 Pie Chart**;
  - **Tab 2: Code Coverage (代码覆盖)**: Multi-color line highlights, line status gutter badges, and "Previous / Next Uncovered" review buttons.
* **Double-Click Edit Lockout on Read-Only Widgets**:
  - Prevents unintended inline editing across the project tree, coverage tables, function lists, metrics views, and stack chain trees.

### 7. Interactive Python Environment Scanner & User Selector
* Clicking the "Environment Detection" (环境检测) button initiates a full computer scan:
  1. System `PATH` (`where.exe python`);
  2. Workspace bundled portable Python (`tools/python/python.exe`);
  3. User directories (`AppData/Local/Programs/Python`);
  4. System root `C:\Python*` and `Program Files`;
  5. Anaconda / Miniconda virtual environments and bases.
* Displays an interactive dialog listing detected Python versions and locations (e.g., `Python 3.10.11 - C:\...`) along with a manual file browse option, giving users full control over which Python interpreter executes their test stimuli.

### 8. Project-Driven Stimulus Script Configuration
* Generic preset template dropdowns have been replaced with a **dynamic project script loader** that scans the active project's `Script/` directory for all `*.py` files;
* Users can easily select from the project's real stimulus scripts and execute them with a single click.

### 9. Embedded Read-Only Report Viewer (Pure Preview, No External Apps)
* The project tree's "Report Files" group dynamically loads all report formats generated in `Report/`: `*.html`, `*.htm`, `*.pdf`, `*.docx`, `*.doc`, `*.json`, `*.txt`;
* **Double-clicking any report opens it directly inside the internal `ReportPreviewDialog`**;
* Operates strictly in read-only view mode (export buttons hidden) for safe, distraction-free document examination.

### 10. Reveal in Explorer via Project Tree Context Menu
* Right-clicking any valid file item in the Project Tree (project configuration, C/C++ source and headers, Python scripts, exported reports) offers **"📂 Reveal in Explorer" (打开文件所在路径并选中)**;
* Opens Windows Explorer directly at the containing folder and **automatically highlights and selects the corresponding file**.

### 11. Automotive-Grade Sample Project & Multi-Scenario Test Stimuli
* **Real-World Motor Controller Application**:
  - Closed-loop PID controller (`pid_controller.c/.h`) featuring anti-windup clamping, derivative low-pass filtering, and deadband thresholds;
  - Automotive diagnostic freeze-frame recorder (`fault_recorder.c/.h`) capturing Diagnostic Trouble Codes (DTCs), timestamps, and environmental snapshots;
  - Multi-state motor control FSM with overtemperature trip, undervoltage/overvoltage lockout, and overspeed shutdown.
* **4 Dedicated Scenario Stimulus Scripts**:
  - `normal_cruise.py`: Smooth acceleration and closed-loop cruise control branch;
  - `overtemp_fault.py`: High-temperature excursion (>105°C) triggering thermal protection and DTC freeze-frame logging;
  - `voltage_fluctuation.py`: Voltage dip (<2.2V) and surge (>4.2V) triggering supply fault branches;
  - `emergency_estop.py`: Hardware emergency stop button press triggering dynamic braking.

---

## 📁 Project Directory Structure

```text
Coverage Analysis/
├── build/                                 # 【CMake Build Output Directory】
│   └── bin/                               # 【Standalone Executables】
│       ├── coverage_gui.exe               # Main graphical desktop studio
│       └── coverage_cli.exe               # Headless command-line CLI
├── config/                                # Configuration (app_config.ini, history, etc.)
├── core/                                  # 【Core Engine Library】libcoverage_core (No GUI)
│   ├── include/coverage/
│   │   ├── types.h                        # Core enums
│   │   ├── models.h                       # DTO models: StaticIssue, FunctionMetric, CoverageReport
│   │   ├── static_analyzer.h              # Static analysis and path exploration engine
│   │   ├── stack_analyzer.h               # Static stack and call depth analyzer
│   │   ├── instrumenter.h                 # Source probe instrumenter (filters variable declarations)
│   │   ├── coverage_engine.h              # Coverage evaluation engine with stimulus matching
│   │   ├── stimulus_engine.h              # Python stimulus execution engine
│   │   ├── graph_generator.h              # Graphviz DOT generator
│   │   ├── logger.h                       # Thread-safe logging subsystem
│   │   ├── report_generator.h             # Native OpenXML docx & multi-format report engine
│   │   └── backend_service.h              # Central backend singleton
│   └── src/
├── gui/                                   # 【Presentation Layer】Qt Desktop Studio
│   ├── resources/                         # SVG icons (includes coverage_analysis.svg)
│   ├── include/
│   │   ├── main_window.h                  # Main window (docking, mutual exclusion, file switcher)
│   │   ├── welcome_view.h                 # Welcome wizard and recent projects
│   │   ├── coverage_view.h                # Coverage studio (tables, charts, syntax highlighting)
│   │   ├── stack_analysis_view.h          # Stack analysis and call depth visualizer
│   │   ├── static_analysis_explorer_view.h# Static analysis & path exploration view
│   │   ├── report_preview_dialog.h        # Embedded pure read-only report viewer
│   │   ├── user_manual_dialog.h           # User manual document reader
│   │   └── theme_manager.h                # Theme manager with auto dark/light mode detection
│   └── src/
├── cli/                                   # Headless CLI implementation
├── docs/                                  # Documentation
│   └── user_manual.md                     # Software user manual
└── examples/                              # 【Industrial Embedded C Sample Suite】
    ├── motor_controller/                  # Automotive BLDC FOC motor drive project
    │   ├── motor_controller.covproj       # Project configuration file
    │   ├── Source/                        # Sources: motor_controller.c, pid_controller.c, fault_recorder.c, etc.
    │   ├── Header/                        # Headers: motor_controller.h, pid_controller.h, fault_recorder.h, etc.
    │   ├── Script/                        # 4 Scenario Python scripts (normal, overtemp, voltage, estop)
    │   └── Report/                        # Project reports
    └── sensor_fusion/                     # Attitude sensor fusion project
```

---

## 🚀 Running the Suite

### 1. Graphical User Interface (GUI)
Run the binary generated in `build/bin/`:
```powershell
.\build\bin\coverage_gui.exe
```

### 2. Command Line Interface (CLI)
```powershell
# 1. Static code quality analysis
.\build\bin\coverage_cli.exe -f examples\motor_controller\Source\motor_controller.c --analyze

# 2. Export native Word (.docx) quality report
.\build\bin\coverage_cli.exe -f examples\motor_controller\Source\motor_controller.c -w reports\motor_report.docx

# 3. Export standalone HTML report
.\build\bin\coverage_cli.exe -f examples\motor_controller\Source\motor_controller.c -m reports\motor_report.html
```

---

## 🧭 Keyboard Shortcuts & Quick Reference

| Shortcut | Action | Description |
| :--- | :--- | :--- |
| **F1** | User Manual | Opens the embedded documentation viewer (pure user guide) |
| **Ctrl + P** | Project Hub | Create, open, and manage projects and history |
| **Ctrl + Shift + N** | Create New Project | Wizard-based creation of isolated project structures |
| **Ctrl + Shift + O** | Open Existing Project | Load `.covproj` and activate the full workspace |
| **Ctrl + Shift + S** | Save Current Project | Save project metadata and file lists |
| **Ctrl + K** | Static Stack Analysis | Launch the binary static stack and worst-case call depth analyzer |
| **Ctrl + O** | Open Single C Source | Open source file into editor tab |
| **Ctrl + I** | Import Directory | Recursively scan and add files to the active project |
| **Ctrl + S** | Save File | Save the active code editor content |
| **Ctrl + E** | Export Quality Report... | Export comprehensive Word/PDF/HTML/JSON reports |
| **Ctrl + L** | View Runtime Log | Open the active persistent session log |
| **F5** | Run Static Analysis | Scan for MISRA/embedded defects, cyclomatic complexity & basis paths |
| **F6** | Instrument Source Code | Insert coverage probes (excluding variable declarations) |
| **F7** | Run Python Stimulus | Execute the selected project stimulus script |
| **F8** | Test Coverage Studio | Open coverage studio (tables, vector charts, syntax highlighting) |
| **Ctrl + F8** | Run Coverage Simulation | Drive simulation kernel with stimulus vectors |
