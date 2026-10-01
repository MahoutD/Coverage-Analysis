#include "coverage/static_analyzer.h"
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QStack>

namespace Coverage {

StaticAnalyzer::StaticAnalyzer()
    : m_options()
{
}

StaticAnalyzer::StaticAnalyzer(const Options& options)
    : m_options(options)
{
}

StaticAnalysisReport StaticAnalyzer::analyzeFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        StaticAnalysisReport report;
        report.filePath = filePath;
        report.timestamp = QDateTime::currentDateTime();
        StaticIssue issue;
        issue.file = filePath;
        issue.severity = Severity::Error;
        issue.message = QStringLiteral("Failed to open source file: ") + file.errorString();
        report.issues.append(issue);
        return report;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    return analyzeSource(content, filePath);
}

StaticAnalysisReport StaticAnalyzer::analyzeSource(const QString& sourceCode, const QString& fileName) {
    QElapsedTimer timer;
    timer.start();

    StaticAnalysisReport report;
    report.filePath = fileName;
    report.timestamp = QDateTime::currentDateTime();

    QStringList lines = sourceCode.split(QStringLiteral("\n"));
    report.totalLines = lines.size();

    // 1. Line statistics (code, comment, blank)
    bool inMultiLineComment = false;
    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) {
            report.blankLines++;
            continue;
        }

        if (inMultiLineComment) {
            report.commentLines++;
            if (line.contains(QStringLiteral("*/"))) {
                inMultiLineComment = false;
            }
            continue;
        }

        if (line.startsWith(QStringLiteral("/*"))) {
            report.commentLines++;
            if (!line.contains(QStringLiteral("*/"))) {
                inMultiLineComment = true;
            }
        } else if (line.startsWith(QStringLiteral("//"))) {
            report.commentLines++;
        } else {
            report.codeLines++;
            if (line.contains(QStringLiteral("/*")) && !line.contains(QStringLiteral("*/"))) {
                inMultiLineComment = true;
            }
        }
    }

    // 2. Line by line analysis (MISRA and embedded rules)
    analyzeLineByLine(lines, report);

    // 3. Function & AST level analysis
    analyzeFunctions(sourceCode, lines, report);

    // 4. 统计代码结构度量指标 (语句数、分支数、注释率、MC/DC 等)
    report.metrics = computeCodeMetrics(sourceCode, fileName, report.functions);

    report.elapsedMs = timer.elapsed();
    return report;
}

void StaticAnalyzer::analyzeLineByLine(const QStringList& lines, StaticAnalysisReport& report) {
    bool inMultiLineComment = false;

    // RegEx patterns
    static const QRegularExpression mallocRegex(QStringLiteral(R"(\b(malloc|calloc|realloc|free)\s*\()"));
    static const QRegularExpression gotoRegex(QStringLiteral(R"(\bgoto\s+([A-Za-z0-9_]+);)"));
    static const QRegularExpression floatEqRegex(QStringLiteral(R"(([a-zA-Z0-9_]+)\s*(==|!=)\s*([0-9]+\.[0-9]+[fF]?|[a-zA-Z0-9_]+))"));
    static const QRegularExpression rawHwAddrRegex(QStringLiteral(R"(\(\s*(uint32_t|uint16_t|uint8_t|int|unsigned\s+int)\s*\*\s*\)\s*0x[0-9A-Fa-f]{4,8})"));
    static const QRegularExpression stackBufferRegex(QStringLiteral(R"(\b(uint8_t|char|int|uint16_t|uint32_t)\s+([a-zA-Z0-9_]+)\[\s*([0-9]+)\s*\];)"));
    static const QRegularExpression busyWaitRegex(QStringLiteral(R"(while\s*\(\s*(1|true)\s*\)\s*;|while\s*\(\s*!\s*([a-zA-Z0-9_]+)\s*\)\s*;)"));

    for (int i = 0; i < lines.size(); ++i) {
        int lineNum = i + 1;
        QString line = lines[i];
        QString trimmed = line.trimmed();

        // Handle block comments
        if (inMultiLineComment) {
            if (trimmed.contains(QStringLiteral("*/"))) {
                inMultiLineComment = false;
            }
            continue;
        }
        if (trimmed.startsWith(QStringLiteral("/*"))) {
            if (!trimmed.contains(QStringLiteral("*/"))) {
                inMultiLineComment = true;
            }
            continue;
        }
        if (trimmed.startsWith(QStringLiteral("//"))) {
            continue;
        }

        // Clean out inline comments for safety parsing
        QString cleanLine = line;
        int inlineCommentPos = cleanLine.indexOf(QStringLiteral("//"));
        if (inlineCommentPos != -1) {
            cleanLine = cleanLine.left(inlineCommentPos);
        }

        // MISRA Rule 21.3: Dynamic memory allocation
        if (m_options.checkMemorySafety) {
            auto match = mallocRegex.match(cleanLine);
            if (match.hasMatch()) {
                StaticIssue issue;
                issue.id = QString("ISSUE-%1-%2").arg(lineNum).arg(match.capturedStart());
                issue.file = report.filePath;
                issue.line = lineNum;
                issue.column = match.capturedStart() + 1;
                issue.ruleId = QStringLiteral("MISRA-C-2012-Rule-21.3");
                issue.category = RuleCategory::MemorySafety;
                issue.severity = Severity::Critical;
                issue.messageEn = QString("Dynamic memory allocation function '%1' is prohibited in embedded safety-critical systems.")
                                    .arg(match.captured(1));
                issue.messageZh = QString("在安全关键嵌入式系统中严禁使用动态内存分配函数 '%1'。")
                                    .arg(match.captured(1));
                issue.suggestionEn = QStringLiteral("Use statically allocated buffers or memory pool architectures with deterministic timing.");
                issue.suggestionZh = QStringLiteral("改用具有确定性执行时间的静态预分配缓冲区或内存池架构。");
                issue.message = issue.messageZh;
                issue.suggestion = issue.suggestionZh;
                issue.codeSnippet = trimmed;
                report.issues.append(issue);
            }
        }

        // MISRA Rule 15.1: Goto statement
        if (m_options.checkMisraRules) {
            auto match = gotoRegex.match(cleanLine);
            if (match.hasMatch()) {
                StaticIssue issue;
                issue.id = QString("ISSUE-%1-%2").arg(lineNum).arg(match.capturedStart());
                issue.file = report.filePath;
                issue.line = lineNum;
                issue.column = match.capturedStart() + 1;
                issue.ruleId = QStringLiteral("MISRA-C-2012-Rule-15.1");
                issue.category = RuleCategory::MisraC;
                issue.severity = Severity::Warning;
                issue.messageEn = QString("Unconditional jump 'goto %1' disrupts structured control flow.").arg(match.captured(1));
                issue.messageZh = QString("无条件跳转 'goto %1' 破坏了结构化控制流。").arg(match.captured(1));
                issue.suggestionEn = QStringLiteral("Refactor into structured loops, helper functions, or state machines.");
                issue.suggestionZh = QStringLiteral("重构为结构化循环、辅助函数或状态机。");
                issue.message = issue.messageZh;
                issue.suggestion = issue.suggestionZh;
                issue.codeSnippet = trimmed;
                report.issues.append(issue);
            }
        }

        // MISRA Rule 12.1: Floating point direct comparison
        if (m_options.checkMisraRules) {
            if (cleanLine.contains(QStringLiteral("float")) || cleanLine.contains(QStringLiteral(".f")) || cleanLine.contains(QStringLiteral("0."))) {
                auto match = floatEqRegex.match(cleanLine);
                if (match.hasMatch() && (match.captured(0).contains(QStringLiteral(".")) || match.captured(0).contains(QStringLiteral("f")))) {
                    StaticIssue issue;
                    issue.id = QString("ISSUE-%1-%2").arg(lineNum).arg(match.capturedStart());
                    issue.file = report.filePath;
                    issue.line = lineNum;
                    issue.column = match.capturedStart() + 1;
                    issue.ruleId = QStringLiteral("MISRA-C-2012-Rule-12.1");
                    issue.category = RuleCategory::MisraC;
                    issue.severity = Severity::Warning;
                    issue.messageEn = QString("Floating-point equality test '%1' is vulnerable to precision/rounding discrepancies.")
                                        .arg(match.captured(0).trimmed());
                    issue.messageZh = QString("浮点数恒等比较 '%1' 易受精度与舍入误差影响。")
                                        .arg(match.captured(0).trimmed());
                    issue.suggestionEn = QStringLiteral("Use delta threshold comparison: fabsf(val - target) < EPSILON.");
                    issue.suggestionZh = QStringLiteral("使用误差阈值比较：fabsf(val - target) < EPSILON。");
                    issue.message = issue.messageZh;
                    issue.suggestion = issue.suggestionZh;
                    issue.codeSnippet = trimmed;
                    report.issues.append(issue);
                }
            }
        }

        // Embedded Hardware: Missing volatile on register pointers
        if (m_options.checkHardwareRegisters) {
            auto match = rawHwAddrRegex.match(cleanLine);
            if (match.hasMatch() && !cleanLine.contains(QStringLiteral("volatile"))) {
                StaticIssue issue;
                issue.id = QString("ISSUE-%1-%2").arg(lineNum).arg(match.capturedStart());
                issue.file = report.filePath;
                issue.line = lineNum;
                issue.column = match.capturedStart() + 1;
                issue.ruleId = QStringLiteral("EMB-VOLATILE-01");
                issue.category = RuleCategory::HardwareAndRegisters;
                issue.severity = Severity::Warning;
                issue.messageEn = QString("Direct memory/hardware register access '%1' lacks 'volatile' qualifier; compiler may optimize reads away.")
                                    .arg(match.captured(0));
                issue.messageZh = QString("内存映射硬件寄存器直接访问 '%1' 缺少 'volatile' 修饰符，编译器可能会将读取优化掉。")
                                    .arg(match.captured(0));
                issue.suggestionEn = QStringLiteral("Qualify memory-mapped pointer with volatile, e.g. '*(volatile uint32_t*)0x...'.");
                issue.suggestionZh = QStringLiteral("使用 volatile 限定指针，如 '*(volatile uint32_t*)0x...'。");
                issue.message = issue.messageZh;
                issue.suggestion = issue.suggestionZh;
                issue.codeSnippet = trimmed;
                report.issues.append(issue);
            }
        }

        // Embedded Safety: Large local array on stack
        if (m_options.checkMemorySafety) {
            auto match = stackBufferRegex.match(cleanLine);
            if (match.hasMatch()) {
                int arraySize = match.captured(3).toInt();
                if (arraySize > m_options.maxStackArrayBytes) {
                    StaticIssue issue;
                    issue.id = QString("ISSUE-%1-%2").arg(lineNum).arg(match.capturedStart());
                    issue.file = report.filePath;
                    issue.line = lineNum;
                    issue.column = match.capturedStart() + 1;
                    issue.ruleId = QStringLiteral("EMB-STACK-01");
                    issue.category = RuleCategory::MemorySafety;
                    issue.severity = Severity::Warning;
                    issue.messageEn = QString("Local array '%1[%2]' exceeds safe embedded stack limit (%3 bytes). Risk of stack overflow.")
                                        .arg(match.captured(2)).arg(arraySize).arg(m_options.maxStackArrayBytes);
                    issue.messageZh = QString("函数局部大数组 '%1[%2]' 超过安全栈限制 (%3 字节)，存在栈溢出风险。")
                                        .arg(match.captured(2)).arg(arraySize).arg(m_options.maxStackArrayBytes);
                    issue.suggestionEn = QStringLiteral("Declare buffer with 'static' storage duration or allocate in designated BSS sections.");
                    issue.suggestionZh = QStringLiteral("声明为 static 静态变量或分配在指定的 BSS 内存段中。");
                    issue.message = issue.messageZh;
                    issue.suggestion = issue.suggestionZh;
                    issue.codeSnippet = trimmed;
                    report.issues.append(issue);
                }
            }
        }

        // Embedded Safety: Busy wait loop without timeout
        if (m_options.checkHardwareRegisters) {
            auto match = busyWaitRegex.match(cleanLine);
            if (match.hasMatch()) {
                StaticIssue issue;
                issue.id = QString("ISSUE-%1-%2").arg(lineNum).arg(match.capturedStart());
                issue.file = report.filePath;
                issue.line = lineNum;
                issue.column = match.capturedStart() + 1;
                issue.ruleId = QStringLiteral("EMB-BUSYLOOP-01");
                issue.category = RuleCategory::HardwareAndRegisters;
                issue.severity = Severity::Warning;
                issue.messageEn = QStringLiteral("Potential busy-wait loop without watchdog refresh or timeout mechanism.");
                issue.messageZh = QStringLiteral("检测到没有看门狗喂狗或超时机制的死循环忙等待，可能导致 CPU 饥饿或复位。");
                issue.suggestionEn = QStringLiteral("Add timeout counter, hardware watchdog refresh, or yielding delay in loop body.");
                issue.suggestionZh = QStringLiteral("在循环体中添加超时退出计数器、看门狗喂狗或让出 CPU 调度的延时。");
                issue.message = issue.messageZh;
                issue.suggestion = issue.suggestionZh;
                issue.codeSnippet = trimmed;
                report.issues.append(issue);
            }
        }
    }
}

void StaticAnalyzer::analyzeFunctions(const QString& sourceCode, const QStringList& lines, StaticAnalysisReport& report) {
    // Function definition regex
    // e.g. void motor_update(int speed, float temp) {
    static const QRegularExpression funcRegex(
        QStringLiteral(R"(^(?:static\s+|extern\s+|inline\s+)*([a-zA-Z0-9_]+(?:\s*\*+)?)\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*\{?)"),
        QRegularExpression::MultilineOption
    );

    // Scan lines for function boundaries
    int braceDepth = 0;
    int currentFuncStartLine = -1;
    QString currentFuncName;
    QString currentReturnType;
    QString currentParams;
    QString currentFuncBody;

    for (int i = 0; i < lines.size(); ++i) {
        int lineNum = i + 1;
        QString line = lines[i];
        QString trimmed = line.trimmed();

        if (braceDepth == 0) {
            auto match = funcRegex.match(line);
            if (match.hasMatch() && !trimmed.startsWith(QStringLiteral("//")) && !trimmed.startsWith(QStringLiteral("#"))) {
                currentReturnType = match.captured(1).trimmed();
                currentFuncName = match.captured(2).trimmed();
                currentParams = match.captured(3).trimmed();

                // Exclude keywords like if, while, for, switch
                if (currentFuncName != QStringLiteral("if") &&
                    currentFuncName != QStringLiteral("while") &&
                    currentFuncName != QStringLiteral("for") &&
                    currentFuncName != QStringLiteral("switch"))
                {
                    currentFuncStartLine = lineNum;
                    currentFuncBody.clear();
                }
            }
        }

        // Count braces
        for (QChar c : line) {
            if (c == QLatin1Char('{')) {
                braceDepth++;
            } else if (c == QLatin1Char('}')) {
                braceDepth--;
                if (braceDepth == 0 && currentFuncStartLine != -1) {
                    // Function ended
                    FunctionMetrics fm;
                    fm.name = currentFuncName;
                    fm.returnType = currentReturnType;
                    fm.startLine = currentFuncStartLine;
                    fm.endLine = lineNum;
                    fm.linesOfCode = (lineNum - currentFuncStartLine) + 1;

                    // Parameter count
                    if (currentParams.isEmpty() || currentParams == QStringLiteral("void")) {
                        fm.parameterCount = 0;
                    } else {
                        fm.parameterCount = currentParams.split(QStringLiteral(",")).size();
                    }

                    // Complexity & Nesting
                    fm.cyclomaticComplexity = computeCyclomaticComplexity(currentFuncBody);
                    fm.maxNestingDepth = computeMaxNestingDepth(currentFuncBody);

                    fm.filePath = report.filePath;
                    // Compute function specific statementCount, branchCount, mcdcCount
                    QStringList funcLines = currentFuncBody.split(QStringLiteral("\n"));
                    for (const QString& fRaw : funcLines) {
                        QString fTrim = fRaw.trimmed();
                        if (fTrim.isEmpty() || fTrim.startsWith(QStringLiteral("//")) || fTrim.startsWith(QStringLiteral("/*"))) continue;
                        if (fTrim.contains(QLatin1Char(';')) || fTrim.startsWith(QStringLiteral("if")) ||
                            fTrim.startsWith(QStringLiteral("while")) || fTrim.startsWith(QStringLiteral("for")) ||
                            fTrim.startsWith(QStringLiteral("switch")) || fTrim.startsWith(QStringLiteral("return")) ||
                            fTrim.startsWith(QStringLiteral("case ")) || fTrim.startsWith(QStringLiteral("default:"))) {
                            fm.statementCount++;
                        }
                        if (fTrim.contains(QRegularExpression(QStringLiteral(R"(\b(if|else\s+if|while|for|case\s+|default:)\b)")))) {
                            fm.branchCount++;
                        }
                        if (fTrim.contains(QLatin1Char('?')) && fTrim.contains(QLatin1Char(':'))) {
                            fm.branchCount++;
                        }
                        if (fTrim.contains(QStringLiteral("&&")) || fTrim.contains(QStringLiteral("||"))) {
                            int andC = fTrim.count(QStringLiteral("&&"));
                            int orC = fTrim.count(QStringLiteral("||"));
                            fm.mcdcCount += (andC + orC + 1);
                        } else if (fTrim.contains(QRegularExpression(QStringLiteral(R"(\bif\s*\([^;]+?\))")))) {
                            fm.mcdcCount++;
                        }
                    }
                    if (fm.branchCount == 0 && fm.cyclomaticComplexity > 1) {
                        fm.branchCount = fm.cyclomaticComplexity - 1;
                    }
                    if (fm.statementCount == 0) {
                        fm.statementCount = qMax(1, fm.linesOfCode - 2);
                    }

                    // Check recursion (MISRA Rule 17.2)
                    int openBraceIdx = currentFuncBody.indexOf(QLatin1Char('{'));
                    QString innerBody = (openBraceIdx != -1) ? currentFuncBody.mid(openBraceIdx + 1) : QString();
                    QRegularExpression recursionRegex(QString("\\b%1\\s*\\(").arg(currentFuncName));
                    if (!innerBody.isEmpty() && recursionRegex.match(innerBody).hasMatch()) {
                        fm.isRecursive = true;
                        StaticIssue issue;
                        issue.id = QString("ISSUE-%1-RECURSION").arg(currentFuncStartLine);
                        issue.file = report.filePath;
                        issue.line = currentFuncStartLine;
                        issue.column = 1;
                        issue.ruleId = QStringLiteral("MISRA-C-2012-Rule-17.2");
                        issue.category = RuleCategory::MisraC;
                        issue.severity = Severity::Critical;
                        issue.messageEn = QString("Function '%1' is recursive. Recursion violates embedded deterministic stack depth guidelines.")
                                            .arg(currentFuncName);
                        issue.messageZh = QString("函数 '%1' 包含递归调用。严禁在安全关键嵌入式栈中使用递归以避免不可预测的栈溢出。")
                                            .arg(currentFuncName);
                        issue.suggestionEn = QStringLiteral("Replace recursive algorithm with an iterative approach using static queue/stack.");
                        issue.suggestionZh = QStringLiteral("改用迭代循环或显式状态机，以保证栈深度绝对可预测。");
                        issue.message = issue.messageZh;
                        issue.suggestion = issue.suggestionZh;
                        issue.codeSnippet = lines[currentFuncStartLine - 1].trimmed();
                        report.issues.append(issue);
                    }

                    // Complexity warnings
                    if (m_options.checkComplexity && fm.cyclomaticComplexity > m_options.maxComplexityWarning) {
                        StaticIssue issue;
                        issue.id = QString("ISSUE-%1-COMPLEXITY").arg(currentFuncStartLine);
                        issue.file = report.filePath;
                        issue.line = currentFuncStartLine;
                        issue.column = 1;
                        issue.ruleId = QStringLiteral("METRIC-CYCLOMATIC-COMPLEXITY");
                        issue.category = RuleCategory::ComplexityAndStructure;
                        issue.severity = (fm.cyclomaticComplexity > m_options.maxComplexityError) ? Severity::Error : Severity::Warning;
                        issue.messageEn = QString("Function '%1' has McCabe cyclomatic complexity of %2 (Threshold: %3).")
                                            .arg(currentFuncName).arg(fm.cyclomaticComplexity).arg(m_options.maxComplexityWarning);
                        issue.messageZh = QString("函数 '%1' 圈复杂度为 %2（阈值: %3），分支过多，测试和维护难度极大。")
                                            .arg(currentFuncName).arg(fm.cyclomaticComplexity).arg(m_options.maxComplexityWarning);
                        issue.suggestionEn = QStringLiteral("Decompose function into smaller cohesive functions with singular responsibilities.");
                        issue.suggestionZh = QStringLiteral("将该函数拆解为单一职责的多个子函数或采用表驱动法重构。");
                        issue.message = issue.messageZh;
                        issue.suggestion = issue.suggestionZh;
                        issue.codeSnippet = lines[currentFuncStartLine - 1].trimmed();
                        report.issues.append(issue);
                    }

                    // Nesting depth warning
                    if (m_options.checkComplexity && fm.maxNestingDepth > m_options.maxNestingDepth) {
                        StaticIssue issue;
                        issue.id = QString("ISSUE-%1-NESTING").arg(currentFuncStartLine);
                        issue.file = report.filePath;
                        issue.line = currentFuncStartLine;
                        issue.column = 1;
                        issue.ruleId = QStringLiteral("METRIC-NESTING-DEPTH");
                        issue.category = RuleCategory::ComplexityAndStructure;
                        issue.severity = Severity::Warning;
                        issue.messageEn = QString("Function '%1' exceeds maximum recommended control nesting depth (Depth: %2, Limit: %3).")
                                            .arg(currentFuncName).arg(fm.maxNestingDepth).arg(m_options.maxNestingDepth);
                        issue.messageZh = QString("函数 '%1' 控制流最大嵌套深度达到 %2 层（阈值: %3）。")
                                            .arg(currentFuncName).arg(fm.maxNestingDepth).arg(m_options.maxNestingDepth);
                        issue.suggestionEn = QStringLiteral("Use guard clauses / early returns or extract nested logic into subroutines.");
                        issue.suggestionZh = QStringLiteral("采用保护性断言/提前返回（Early Return）或将深层逻辑提取为独立子函数。");
                        issue.message = issue.messageZh;
                        issue.suggestion = issue.suggestionZh;
                        issue.codeSnippet = lines[currentFuncStartLine - 1].trimmed();
                        report.issues.append(issue);
                    }

                    report.functions.append(fm);
                    currentFuncStartLine = -1;
                }
            }
        }

        if (currentFuncStartLine != -1) {
            currentFuncBody += line + QStringLiteral("\n");
        }
    }

    // -------------------------------------------------------------
    // 计算高级度量指标：扇入 (Fan-In)、扇出 (Fan-Out)、函数深度、调用深度 (Call Depth)
    // -------------------------------------------------------------
    static const QRegularExpression callRegex(QStringLiteral(R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\s*\()"));
    static const QSet<QString> cKeywords = {
        QStringLiteral("if"), QStringLiteral("while"), QStringLiteral("for"),
        QStringLiteral("switch"), QStringLiteral("return"), QStringLiteral("sizeof"),
        QStringLiteral("typeof"), QStringLiteral("alignas"), QStringLiteral("alignof"),
        QStringLiteral("static_assert"), QStringLiteral("case")
    };

    QMap<QString, QSet<QString>> callerToCallees;
    QMap<QString, QSet<QString>> calleeToCallers;
    QSet<QString> internalFuncNames;
    for (const auto& fm : report.functions) {
        internalFuncNames.insert(fm.name);
    }

    for (int i = 0; i < report.functions.size(); ++i) {
        auto& fm = report.functions[i];
        fm.functionDepth = qMax(1, fm.maxNestingDepth);

        // 获取函数体代码
        QString fBody;
        int s = qMax(0, fm.startLine - 1);
        int e = qMin(lines.size() - 1, fm.endLine - 1);
        for (int l = s; l <= e; ++l) {
            fBody += lines[l] + QLatin1Char('\n');
        }

        // 提取被调用函数
        auto it = callRegex.globalMatch(fBody);
        QSet<QString> calledSet;
        while (it.hasNext()) {
            auto m = it.next();
            QString callee = m.captured(1).trimmed();
            if (!cKeywords.contains(callee) && callee != fm.name) {
                calledSet.insert(callee);
            }
        }
        callerToCallees[fm.name] = calledSet;
        for (const QString& callee : calledSet) {
            calleeToCallers[callee].insert(fm.name);
        }
    }

    // 回填扇入和扇出数
    for (auto& fm : report.functions) {
        fm.fanOut = callerToCallees.value(fm.name).size();
        fm.fanIn = calleeToCallers.value(fm.name).size();
    }

    // 计算调用深度 (Call Depth)：自顶向下求解有向调用图的最长调用链
    QMap<QString, int> callDepths;
    for (const auto& fm : report.functions) {
        callDepths[fm.name] = 1;
    }

    bool changed = true;
    for (int iter = 0; iter < report.functions.size() && changed; ++iter) {
        changed = false;
        for (const auto& fm : report.functions) {
            int currentDepth = callDepths.value(fm.name, 1);
            const auto& callees = callerToCallees.value(fm.name);
            for (const QString& callee : callees) {
                if (internalFuncNames.contains(callee)) {
                    int newDepth = currentDepth + 1;
                    if (newDepth > callDepths.value(callee, 1)) {
                        callDepths[callee] = newDepth;
                        changed = true;
                    }
                }
            }
        }
    }

    for (auto& fm : report.functions) {
        fm.callDepth = qMax(1, callDepths.value(fm.name, 1));
    }
}

int StaticAnalyzer::computeCyclomaticComplexity(const QString& functionBody) {
    int complexity = 1; // Base

    static const QRegularExpression branchRegex(
        QStringLiteral(R"(\b(if|while|for|case)\b|&&|\|\||\?)")
    );

    auto iterator = branchRegex.globalMatch(functionBody);
    while (iterator.hasNext()) {
        iterator.next();
        complexity++;
    }

    return complexity;
}

int StaticAnalyzer::computeMaxNestingDepth(const QString& functionBody) {
    int maxDepth = 0;
    int currentDepth = 0;

    for (QChar c : functionBody) {
        if (c == QLatin1Char('{')) {
            currentDepth++;
            if (currentDepth > maxDepth) maxDepth = currentDepth;
        } else if (c == QLatin1Char('}')) {
            if (currentDepth > 0) currentDepth--;
        }
    }

    return maxDepth;
}

CodeMetrics StaticAnalyzer::computeCodeMetrics(const QString& sourceCode, const QString& fileName, const QVector<FunctionMetrics>& functions) {
    CodeMetrics m;
    m.filePath = fileName;
    QStringList lines = sourceCode.split(QStringLiteral("\n"));
    m.totalLines = lines.size();
    m.functionCount = functions.size();

    bool inBlockComment = false;
    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        if (inBlockComment) {
            m.commentLines++;
            if (line.contains(QStringLiteral("*/"))) inBlockComment = false;
            continue;
        }
        if (line.startsWith(QStringLiteral("/*"))) {
            m.commentLines++;
            if (!line.contains(QStringLiteral("*/"))) inBlockComment = true;
            continue;
        }
        if (line.startsWith(QStringLiteral("//"))) {
            m.commentLines++;
            continue;
        }

        // Count statements
        if (line.contains(QLatin1Char(';')) || line.startsWith(QStringLiteral("if")) ||
            line.startsWith(QStringLiteral("while")) || line.startsWith(QStringLiteral("for")) ||
            line.startsWith(QStringLiteral("switch")) || line.startsWith(QStringLiteral("return")) ||
            line.startsWith(QStringLiteral("case ")) || line.startsWith(QStringLiteral("default:"))) {
            m.statementCount++;
        }

        // Count branches
        if (line.contains(QRegularExpression(QStringLiteral(R"(\b(if|else\s+if|while|for|case\s+|default:)\b)")))) {
            m.branchCount++;
        }
        if (line.contains(QLatin1Char('?')) && line.contains(QLatin1Char(':'))) {
            m.branchCount++;
        }

        // Count MC/DC decisions and atomic conditions
        if (line.contains(QStringLiteral("&&")) || line.contains(QStringLiteral("||"))) {
            m.mcdcDecisions++;
            int andCount = line.count(QStringLiteral("&&"));
            int orCount = line.count(QStringLiteral("||"));
            m.mcdcConditions += (andCount + orCount + 1);
        } else if (line.contains(QRegularExpression(QStringLiteral(R"(\bif\s*\([^;]+?\))")))) {
            m.mcdcDecisions++;
            m.mcdcConditions++;
        }
    }

    if (m.totalLines > 0) {
        m.commentRatio = (double)m.commentLines / m.totalLines * 100.0;
    }

    m.totalComplexity = 0;
    m.maxComplexity = 1;
    for (const auto& f : functions) {
        m.totalComplexity += f.cyclomaticComplexity;
        if (f.cyclomaticComplexity > m.maxComplexity) {
            m.maxComplexity = f.cyclomaticComplexity;
        }
    }

    return m;
}

QVector<FunctionPath> StaticAnalyzer::extractFunctionPaths(const FunctionMetrics& func, const QStringList& fileLines) {
    QVector<FunctionPath> paths;
    int start = qMax(0, func.startLine - 1);
    int end = qMin(fileLines.size() - 1, func.endLine - 1);

    struct DecisionInfo {
        int lineNo;
        QString condition;
        int thenStartLine;
        int thenEndLine;
        bool hasReturn = false;
    };
    QVector<DecisionInfo> decisions;
    QVector<int> returnLines;
    QVector<int> normalStmtLines;

    static const QRegularExpression ifReg(QStringLiteral(R"(\bif\s*\(([^;]+?)\))"));
    static const QRegularExpression retReg(QStringLiteral(R"(\breturn\b)"));

    for (int i = start; i <= end; ++i) {
        int lineNo = i + 1;
        QString line = fileLines[i].trimmed();
        if (line.isEmpty() || line.startsWith(QStringLiteral("//")) || line.startsWith(QStringLiteral("/*"))) {
            continue;
        }

        auto match = ifReg.match(line);
        if (match.hasMatch()) {
            DecisionInfo d;
            d.lineNo = lineNo;
            d.condition = match.captured(1).trimmed();
            d.thenStartLine = lineNo + 1;
            int blkEnd = qMin(end, lineNo + 4);
            for (int k = i + 1; k <= end; ++k) {
                if (fileLines[k].contains(QLatin1Char('}')) || fileLines[k].contains(QStringLiteral("return"))) {
                    blkEnd = k + 1;
                    if (fileLines[k].contains(QStringLiteral("return"))) d.hasReturn = true;
                    break;
                }
            }
            d.thenEndLine = blkEnd;
            decisions.append(d);
        } else if (retReg.match(line).hasMatch()) {
            returnLines.append(lineNo);
        } else {
            normalStmtLines.append(lineNo);
        }
    }

    // 路径 1: 标称正常主线路径 (Normal / Baseline Execution Path)
    {
        FunctionPath p1;
        p1.pathId = 1;
        p1.name = QStringLiteral("路径 1: 标称正常主线执行路径 (Normal Baseline Path)");
        p1.conditionDescription = decisions.isEmpty() ? 
            QStringLiteral("顺序执行，无分支阻断 (Sequential Execution)") :
            QStringLiteral("所有条件判定均未触发保护分支 (All Decision Guards Passed)");
        p1.isErrorPath = false;

        p1.executedLines.append(func.startLine);
        for (int l : normalStmtLines) p1.executedLines.append(l);
        for (const auto& d : decisions) p1.executedLines.append(d.lineNo);
        if (!returnLines.isEmpty()) p1.executedLines.append(returnLines.last());
        p1.executedLines.append(func.endLine);
        std::sort(p1.executedLines.begin(), p1.executedLines.end());
        p1.executedLines.erase(std::unique(p1.executedLines.begin(), p1.executedLines.end()), p1.executedLines.end());

        p1.visitedNodeIds.append(QStringLiteral("Entry"));
        for (int i = 0; i < decisions.size(); ++i) {
            p1.visitedNodeIds.append(QString("Decision_%1").arg(i + 1));
        }
        p1.visitedNodeIds.append(QStringLiteral("Exit"));
        paths.append(p1);
    }

    // 分支路径：每个决策点单独命中生成的执行路径
    for (int dIdx = 0; dIdx < decisions.size(); ++dIdx) {
        const auto& d = decisions[dIdx];
        FunctionPath p;
        p.pathId = dIdx + 2;
        p.name = QString("路径 %1: 分支条件命中 [行 %2: %3]")
                     .arg(p.pathId)
                     .arg(d.lineNo)
                     .arg(d.condition.left(24) + (d.condition.length() > 24 ? QStringLiteral("...") : QString()));
        p.conditionDescription = QString("在行 %1 判定 [%2] 为 True，转入处理分支")
                                     .arg(d.lineNo).arg(d.condition);
        p.isErrorPath = d.hasReturn || d.condition.contains(QStringLiteral("ERR")) || 
                        d.condition.contains(QStringLiteral("FAULT")) || 
                        d.condition.contains(QLatin1Char('>')) || 
                        d.condition.contains(QStringLiteral("=="));

        p.executedLines.append(func.startLine);
        for (int l : normalStmtLines) {
            if (l <= d.lineNo) p.executedLines.append(l);
        }
        for (int k = 0; k <= dIdx; ++k) {
            p.executedLines.append(decisions[k].lineNo);
        }
        for (int l = d.thenStartLine; l <= d.thenEndLine; ++l) {
            p.executedLines.append(l);
        }
        p.executedLines.append(func.endLine);
        std::sort(p.executedLines.begin(), p.executedLines.end());
        p.executedLines.erase(std::unique(p.executedLines.begin(), p.executedLines.end()), p.executedLines.end());

        p.visitedNodeIds.append(QStringLiteral("Entry"));
        for (int k = 0; k <= dIdx; ++k) {
            p.visitedNodeIds.append(QString("Decision_%1").arg(k + 1));
        }
        p.visitedNodeIds.append(QString("Then_%1").arg(dIdx + 1));
        p.visitedNodeIds.append(QStringLiteral("Exit"));
        paths.append(p);
    }

    return paths;
}

} // namespace Coverage
