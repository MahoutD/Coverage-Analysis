#include "coverage/graph_generator.h"
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QCoreApplication>

namespace Coverage {

QString GraphGenerator::findDotBinary() {
    // 1. 优先检查当前程序同级目录的本地部署 Graphviz (完全不依赖系统 PATH)
    QString appLocal = QCoreApplication::applicationDirPath() + QStringLiteral("/graphviz/bin/dot.exe");
    if (QFile::exists(appLocal)) {
        return QDir::toNativeSeparators(appLocal);
    }

    // 2. 检查工程自带的 tools/graphviz 目录
    const QStringList projectPaths = {
        QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis/tools/graphviz/Graphviz-12.2.1-win64/bin/dot.exe"),
        QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis/build/bin/graphviz/bin/dot.exe"),
        QDir::currentPath() + QStringLiteral("/tools/graphviz/Graphviz-12.2.1-win64/bin/dot.exe"),
        QDir::currentPath() + QStringLiteral("/graphviz/bin/dot.exe")
    };
    for (const QString& p : projectPaths) {
        if (QFile::exists(p)) {
            return QDir::toNativeSeparators(p);
        }
    }

    // 3. 检查系统常见安装路径与 PATH
    QString pathBinary = QStandardPaths::findExecutable(QStringLiteral("dot"));
    if (!pathBinary.isEmpty()) {
        return pathBinary;
    }

    const QStringList commonPaths = {
        QStringLiteral("C:/Program Files/Graphviz/bin/dot.exe"),
        QStringLiteral("C:/Program Files (x86)/Graphviz/bin/dot.exe"),
        QStringLiteral("D:/Program Files/Graphviz/bin/dot.exe")
    };
    for (const QString& p : commonPaths) {
        if (QFile::exists(p)) {
            return QDir::toNativeSeparators(p);
        }
    }

    return QStringLiteral("dot");
}

QString GraphGenerator::generateCallGraphDot(const StaticAnalysisReport& report, const QString& sourceCode) {
    QString dot;
    dot += QStringLiteral("digraph CallGraph {\n");
    dot += QStringLiteral("    rankdir=LR;\n");
    dot += QStringLiteral("    bgcolor=\"#1e1e2e\";\n");
    dot += QStringLiteral("    node [shape=box, style=\"rounded,filled\", fontname=\"Segoe UI,Consolas\", fontsize=10, fontcolor=\"#11111b\", penwidth=1.5, margin=\"0.25,0.15\"];\n");
    dot += QStringLiteral("    edge [fontname=\"Segoe UI\", fontsize=9, color=\"#89b4fa\", fontcolor=\"#cdd6f4\", arrowsize=0.8, penwidth=1.2];\n\n");

    // 记录所有函数名称，用于后续快速匹配调用点
    QSet<QString> knownFunctions;
    for (const auto& f : report.functions) {
        knownFunctions.insert(f.name);
    }

    QStringList lines = sourceCode.split(QStringLiteral("\n"));

    // 1. 生成所有函数节点，并根据圈复杂度分配状态颜色
    for (const auto& f : report.functions) {
        QString fillColor = QStringLiteral("#a6e3a1"); // 绿色 (正常复杂度 <= 5)
        if (f.cyclomaticComplexity > 15) {
            fillColor = QStringLiteral("#f38ba8");     // 红色 (严重过高复杂度 > 15)
        } else if (f.cyclomaticComplexity > 10) {
            fillColor = QStringLiteral("#f9e2af");     // 黄色 (警告复杂度 11-15)
        } else if (f.cyclomaticComplexity > 5) {
            fillColor = QStringLiteral("#89b4fa");     // 蓝色 (中等复杂度 6-10)
        }

        QString label = QString("%1()\\n[LOC: %2, Complexity: %3]")
                            .arg(f.name)
                            .arg(f.linesOfCode)
                            .arg(f.cyclomaticComplexity);

        dot += QString("    \"%1\" [label=\"%2\", fillcolor=\"%3\"];\n")
                   .arg(f.name, label, fillColor);
    }

    dot += QStringLiteral("\n");

    // 2. 扫描每个函数体内部对其他函数的调用关系（边）
    for (const auto& caller : report.functions) {
        // 截取函数体代码
        int startIdx = qMax(0, caller.startLine - 1);
        int endIdx = qMin(lines.size() - 1, caller.endLine - 1);

        QString funcBody;
        for (int i = startIdx + 1; i <= endIdx; ++i) {
            funcBody += lines[i] + QStringLiteral("\n");
        }

        QSet<QString> calledCallees;
        for (const QString& calleeName : knownFunctions) {
            if (calleeName == caller.name) {
                // 递归调用自环
                QRegularExpression recReg(QString("\\b%1\\s*\\(").arg(calleeName));
                if (recReg.match(funcBody).hasMatch()) {
                    calledCallees.insert(calleeName);
                }
            } else {
                QRegularExpression callReg(QString("\\b%1\\s*\\(").arg(calleeName));
                if (callReg.match(funcBody).hasMatch()) {
                    calledCallees.insert(calleeName);
                }
            }
        }

        for (const QString& target : calledCallees) {
            if (target == caller.name) {
                dot += QString("    \"%1\" -> \"%2\" [color=\"#f38ba8\", style=dashed, label=\"recursion\"];\n")
                           .arg(caller.name, target);
            } else {
                dot += QString("    \"%1\" -> \"%2\";\n").arg(caller.name, target);
            }
        }
    }

    dot += QStringLiteral("}\n");
    return dot;
}

QString GraphGenerator::generateFunctionCfgDot(const FunctionMetrics& func, const QString& sourceCode) {
    QString dot;
    dot += QString("digraph CFG_%1 {\n").arg(func.name);
    dot += QStringLiteral("    rankdir=TB;\n");
    dot += QStringLiteral("    bgcolor=\"#1e1e2e\";\n");
    dot += QStringLiteral("    node [fontname=\"Consolas,Segoe UI\", fontsize=10, fontcolor=\"#11111b\", penwidth=1.5, margin=\"0.25,0.15\"];\n");
    dot += QStringLiteral("    edge [fontname=\"Segoe UI\", fontsize=9, color=\"#89b4fa\", fontcolor=\"#cdd6f4\", arrowsize=0.8, penwidth=1.3];\n\n");

    // 入口与出口节点
    dot += QString("    Entry [shape=ellipse, style=filled, fillcolor=\"#a6e3a1\", label=\"%1\\n[Entry]\"];\n").arg(func.name);
    dot += QStringLiteral("    Exit [shape=ellipse, style=filled, fillcolor=\"#cba6f7\", label=\"Exit\"];\n\n");

    QStringList lines = sourceCode.split(QStringLiteral("\n"));
    int start = qMax(0, func.startLine - 1);
    int end = qMin(lines.size() - 1, func.endLine - 1);

    int blockIndex = 1;
    QString lastNode = QStringLiteral("Entry");

    static const QRegularExpression ifRegex(QStringLiteral(R"(\bif\s*\(([^;]+?)\))"));
    static const QRegularExpression returnRegex(QStringLiteral(R"(\breturn\b)"));
    static const QRegularExpression whileRegex(QStringLiteral(R"(\bwhile\s*\(([^;]+?)\))"));

    for (int i = start + 1; i < end; ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty() || line.startsWith(QStringLiteral("//")) || line.startsWith(QStringLiteral("/*"))) {
            continue;
        }

        int lineNo = i + 1;

        // 决策分支判定 if
        auto ifMatch = ifRegex.match(line);
        if (ifMatch.hasMatch()) {
            QString decisionNode = QString("Decision_%1").arg(blockIndex++);
            QString conditionText = ifMatch.captured(1).trimmed().toHtmlEscaped();
            conditionText.replace(QLatin1Char('"'), QStringLiteral("\\\""));

            dot += QString("    %1 [shape=diamond, style=filled, fillcolor=\"#f9e2af\", label=\"L%2: if (%3)\"];\n")
                       .arg(decisionNode).arg(lineNo).arg(conditionText);

            dot += QString("    %1 -> %2;\n").arg(lastNode, decisionNode);

            // True 分支
            QString thenNode = QString("Then_%1").arg(blockIndex++);
            dot += QString("    %1 [shape=box, style=\"rounded,filled\", fillcolor=\"#89b4fa\", label=\"L%2: Then Block\"];\n")
                       .arg(thenNode).arg(lineNo);
            dot += QString("    %1 -> %2 [color=\"#a6e3a1\", label=\"True\"];\n").arg(decisionNode, thenNode);

            lastNode = thenNode;
            continue;
        }

        // 循环 while
        auto whileMatch = whileRegex.match(line);
        if (whileMatch.hasMatch()) {
            QString loopNode = QString("Loop_%1").arg(blockIndex++);
            QString cond = whileMatch.captured(1).trimmed().toHtmlEscaped();
            cond.replace(QLatin1Char('"'), QStringLiteral("\\\""));

            dot += QString("    %1 [shape=diamond, style=filled, fillcolor=\"#fab387\", label=\"L%2: while (%3)\"];\n")
                       .arg(loopNode).arg(lineNo).arg(cond);
            dot += QString("    %1 -> %2;\n").arg(lastNode, loopNode);

            QString loopBodyNode = QString("LoopBody_%1").arg(blockIndex++);
            dot += QString("    %1 [shape=box, style=\"rounded,filled\", fillcolor=\"#89b4fa\", label=\"L%2: Loop Body\"];\n")
                       .arg(loopBodyNode).arg(lineNo);
            dot += QString("    %1 -> %2 [color=\"#a6e3a1\", label=\"Repeat\"];\n").arg(loopNode, loopBodyNode);
            dot += QString("    %1 -> %2 [style=dashed, color=\"#fab387\"];\n").arg(loopBodyNode, loopNode);

            lastNode = loopNode;
            continue;
        }

        // return 退出节点
        if (returnRegex.match(line).hasMatch()) {
            QString retNode = QString("Return_%1").arg(blockIndex++);
            QString cleanLine = line.toHtmlEscaped();
            cleanLine.replace(QLatin1Char('"'), QStringLiteral("\\\""));

            dot += QString("    %1 [shape=box, style=\"rounded,filled\", fillcolor=\"#f38ba8\", label=\"L%2: %3\"];\n")
                       .arg(retNode).arg(lineNo).arg(cleanLine);
            dot += QString("    %1 -> %2;\n").arg(lastNode, retNode);
            dot += QString("    %1 -> Exit;\n").arg(retNode);
            continue;
        }

        // 普通关键赋值/调用语句块
        if (line.endsWith(QStringLiteral(";")) && (line.contains(QStringLiteral("=")) || line.contains(QStringLiteral("(")))) {
            if (blockIndex <= 15) { // 限制节点总数，避免控制流图过于臃肿
                QString stmtNode = QString("Stmt_%1").arg(blockIndex++);
                QString clean = line.toHtmlEscaped();
                clean.replace(QLatin1Char('"'), QStringLiteral("\\\""));

                dot += QString("    %1 [shape=box, style=\"rounded,filled\", fillcolor=\"#cdd6f4\", label=\"L%2: %3\"];\n")
                           .arg(stmtNode).arg(lineNo).arg(clean);
                dot += QString("    %1 -> %2;\n").arg(lastNode, stmtNode);
                lastNode = stmtNode;
            }
        }
    }

    if (lastNode != QStringLiteral("Exit")) {
        dot += QString("    %1 -> Exit;\n").arg(lastNode);
    }

    dot += QStringLiteral("}\n");
    return dot;
}

QString GraphGenerator::generateHighlightedPathCfgDot(const FunctionMetrics& func, const QString& sourceCode, const FunctionPath& path) {
    QString dot;
    dot += QString("digraph CFG_PATH_%1_%2 {\n").arg(func.name).arg(path.pathId);
    dot += QStringLiteral("    rankdir=TB;\n");
    dot += QStringLiteral("    bgcolor=\"#181825\";\n");
    dot += QStringLiteral("    node [fontname=\"Consolas,Segoe UI\", fontsize=10, penwidth=1.5, margin=\"0.25,0.15\"];\n");
    dot += QStringLiteral("    edge [fontname=\"Segoe UI\", fontsize=9, arrowsize=0.9, penwidth=1.5];\n\n");

    QSet<QString> activeNodes;
    for (const auto& nid : path.visitedNodeIds) {
        activeNodes.insert(nid);
    }

    // 入口与出口节点
    bool entryActive = activeNodes.contains(QStringLiteral("Entry"));
    dot += QString("    Entry [shape=ellipse, style=filled, fillcolor=\"%1\", color=\"%2\", penwidth=%3, fontcolor=\"%4\", label=\"%5\\n[Entry]\"];\n")
               .arg(entryActive ? QStringLiteral("#10b981") : QStringLiteral("#334155"))
               .arg(entryActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
               .arg(entryActive ? 3.0 : 1.2)
               .arg(entryActive ? QStringLiteral("#ffffff") : QStringLiteral("#94a3b8"))
               .arg(func.name);

    bool exitActive = activeNodes.contains(QStringLiteral("Exit"));
    dot += QString("    Exit [shape=ellipse, style=filled, fillcolor=\"%1\", color=\"%2\", penwidth=%3, fontcolor=\"%4\", label=\"Exit\"];\n\n")
               .arg(exitActive ? QStringLiteral("#a855f7") : QStringLiteral("#334155"))
               .arg(exitActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
               .arg(exitActive ? 3.0 : 1.2)
               .arg(exitActive ? QStringLiteral("#ffffff") : QStringLiteral("#94a3b8"));

    QStringList lines = sourceCode.split(QStringLiteral("\n"));
    int start = qMax(0, func.startLine - 1);
    int end = qMin(lines.size() - 1, func.endLine - 1);

    int blockIndex = 1;
    QString lastNode = QStringLiteral("Entry");

    static const QRegularExpression ifRegex(QStringLiteral(R"(\bif\s*\(([^;]+?)\))"));
    static const QRegularExpression returnRegex(QStringLiteral(R"(\breturn\b)"));
    static const QRegularExpression whileRegex(QStringLiteral(R"(\bwhile\s*\(([^;]+?)\))"));

    for (int i = start + 1; i < end; ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty() || line.startsWith(QStringLiteral("//")) || line.startsWith(QStringLiteral("/*"))) {
            continue;
        }

        int lineNo = i + 1;

        // 决策分支判定 if
        auto ifMatch = ifRegex.match(line);
        if (ifMatch.hasMatch()) {
            QString decisionNode = QString("Decision_%1").arg(blockIndex++);
            bool dActive = activeNodes.contains(decisionNode);
            QString conditionText = ifMatch.captured(1).trimmed().toHtmlEscaped();
            conditionText.replace(QLatin1Char('"'), QStringLiteral("\\\""));

            dot += QString("    %1 [shape=diamond, style=filled, fillcolor=\"%2\", color=\"%3\", penwidth=%4, fontcolor=\"%5\", label=\"L%6: if (%7)\"];\n")
                       .arg(decisionNode)
                       .arg(dActive ? QStringLiteral("#f59e0b") : QStringLiteral("#334155"))
                       .arg(dActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                       .arg(dActive ? 3.0 : 1.2)
                       .arg(dActive ? QStringLiteral("#ffffff") : QStringLiteral("#94a3b8"))
                       .arg(lineNo).arg(conditionText);

            bool edgeActive = activeNodes.contains(lastNode) && dActive;
            dot += QString("    %1 -> %2 [color=\"%3\", penwidth=%4];\n")
                       .arg(lastNode, decisionNode)
                       .arg(edgeActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                       .arg(edgeActive ? 2.5 : 1.0);

            // True 分支
            QString thenNode = QString("Then_%1").arg(blockIndex++);
            bool thenActive = activeNodes.contains(thenNode);
            dot += QString("    %1 [shape=box, style=\"rounded,filled\", fillcolor=\"%2\", color=\"%3\", penwidth=%4, fontcolor=\"%5\", label=\"L%6: Then Block\"];\n")
                       .arg(thenNode)
                       .arg(thenActive ? QStringLiteral("#3b82f6") : QStringLiteral("#334155"))
                       .arg(thenActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                       .arg(thenActive ? 3.0 : 1.2)
                       .arg(thenActive ? QStringLiteral("#ffffff") : QStringLiteral("#94a3b8"))
                       .arg(lineNo);

            bool thenEdgeActive = dActive && thenActive;
            dot += QString("    %1 -> %2 [color=\"%3\", penwidth=%4, label=\"True\"];\n")
                       .arg(decisionNode, thenNode)
                       .arg(thenEdgeActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                       .arg(thenEdgeActive ? 2.5 : 1.0);

            lastNode = thenNode;
            continue;
        }

        // return 退出节点
        if (returnRegex.match(line).hasMatch()) {
            QString retNode = QString("Return_%1").arg(blockIndex++);
            bool retActive = path.executedLines.contains(lineNo);
            QString cleanLine = line.toHtmlEscaped();
            cleanLine.replace(QLatin1Char('"'), QStringLiteral("\\\""));

            dot += QString("    %1 [shape=box, style=\"rounded,filled\", fillcolor=\"%2\", color=\"%3\", penwidth=%4, fontcolor=\"%5\", label=\"L%6: %7\"];\n")
                       .arg(retNode)
                       .arg(retActive ? QStringLiteral("#ef4444") : QStringLiteral("#334155"))
                       .arg(retActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                       .arg(retActive ? 3.0 : 1.2)
                       .arg(retActive ? QStringLiteral("#ffffff") : QStringLiteral("#94a3b8"))
                       .arg(lineNo).arg(cleanLine);

            bool toRetActive = activeNodes.contains(lastNode) && retActive;
            dot += QString("    %1 -> %2 [color=\"%3\", penwidth=%4];\n")
                       .arg(lastNode, retNode)
                       .arg(toRetActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                       .arg(toRetActive ? 2.5 : 1.0);

            bool toExitActive = retActive && exitActive;
            dot += QString("    %1 -> Exit [color=\"%2\", penwidth=%3];\n")
                       .arg(retNode)
                       .arg(toExitActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                       .arg(toExitActive ? 2.5 : 1.0);
            continue;
        }

        // 普通关键赋值/调用语句块
        if (line.endsWith(QStringLiteral(";")) && (line.contains(QStringLiteral("=")) || line.contains(QStringLiteral("(")))) {
            if (blockIndex <= 15) {
                QString stmtNode = QString("Stmt_%1").arg(blockIndex++);
                bool stmtActive = path.executedLines.contains(lineNo);
                QString clean = line.toHtmlEscaped();
                clean.replace(QLatin1Char('"'), QStringLiteral("\\\""));

                dot += QString("    %1 [shape=box, style=\"rounded,filled\", fillcolor=\"%2\", color=\"%3\", penwidth=%4, fontcolor=\"%5\", label=\"L%6: %7\"];\n")
                           .arg(stmtNode)
                           .arg(stmtActive ? QStringLiteral("#06b6d4") : QStringLiteral("#334155"))
                           .arg(stmtActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                           .arg(stmtActive ? 3.0 : 1.2)
                           .arg(stmtActive ? QStringLiteral("#ffffff") : QStringLiteral("#94a3b8"))
                           .arg(lineNo).arg(clean);

                bool toStmtActive = activeNodes.contains(lastNode) && stmtActive;
                dot += QString("    %1 -> %2 [color=\"%3\", penwidth=%4];\n")
                           .arg(lastNode, stmtNode)
                           .arg(toStmtActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                           .arg(toStmtActive ? 2.5 : 1.0);
                lastNode = stmtNode;
            }
        }
    }

    if (lastNode != QStringLiteral("Exit")) {
        bool toExitActive = activeNodes.contains(lastNode) && exitActive;
        dot += QString("    %1 -> Exit [color=\"%2\", penwidth=%3];\n")
                   .arg(lastNode)
                   .arg(toExitActive ? QStringLiteral("#10b981") : QStringLiteral("#475569"))
                   .arg(toExitActive ? 2.5 : 1.0);
    }

    dot += QStringLiteral("}\n");
    return dot;
}

QString GraphGenerator::renderDotToSvg(const QString& dotContent,
                                       const QString& dotBinaryPath,
                                       QString* errorMsg)
{
    QString dotExecutable = dotBinaryPath.isEmpty() ? findDotBinary() : dotBinaryPath;

    QProcess process;
    process.start(dotExecutable, QStringList() << QStringLiteral("-Tsvg"));

    if (!process.waitForStarted(3000)) {
        if (errorMsg) {
            *errorMsg = QString("无法启动 Graphviz 程序 (%1)。请确认是否已安装 Graphviz 并配置到系统 PATH 中。")
                            .arg(dotExecutable);
        }
        return QString();
    }

    process.write(dotContent.toUtf8());
    process.closeWriteChannel();

    if (!process.waitForFinished(5000)) {
        process.kill();
        if (errorMsg) {
            *errorMsg = QStringLiteral("Graphviz 渲染超时。");
        }
        return QString();
    }

    if (process.exitCode() != 0) {
        if (errorMsg) {
            *errorMsg = QString::fromUtf8(process.readAllStandardError());
        }
        return QString();
    }

    return QString::fromUtf8(process.readAllStandardOutput());
}

} // namespace Coverage
