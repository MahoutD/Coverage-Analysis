#include "coverage/graph_layout.h"
#include "coverage/graph_generator.h"
#include "coverage/logger.h"

#include <graphviz/gvc.h>
#include <graphviz/cgraph.h>

#include <QProcess>
#include <QRegularExpression>
#include <QFile>

namespace Coverage {

static QStringList parsePlainTokens(const QString& line) {
    QStringList tokens;
    QString current;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        QChar c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c.isSpace() && !inQuotes) {
            if (!current.isEmpty()) {
                tokens.append(current);
                current.clear();
            }
        } else if (c == '\\' && inQuotes && i + 1 < line.size()) {
            QChar next = line[++i];
            if (next == 'n') current.append('\n');
            else if (next == '"') current.append('"');
            else current.append(next);
        } else {
            current.append(c);
        }
    }
    if (!current.isEmpty()) {
        tokens.append(current);
    }
    return tokens;
}

LayoutGraph GraphLayoutEngine::computeLayout(const QString& dotContent,
                                             const StaticAnalysisReport* report,
                                             QString* outError)
{
    QString plainText;

    // 1. 优先调用 Graphviz C 动态库进行零失真内存计算
    GVC_t* gvc = gvContext();
    if (gvc) {
        QByteArray dotBytes = dotContent.toUtf8();
        Agraph_t* g = agmemread(dotBytes.constData());
        if (g) {
            if (gvLayout(gvc, g, "dot") == 0) {
                char* result = nullptr;
                unsigned int len = 0;
                if (gvRenderData(gvc, g, "plain-ext", &result, &len) == 0 && result) {
                    plainText = QString::fromUtf8(result, len);
                    gvFreeRenderData(result);
                }
                gvFreeLayout(gvc, g);
            }
            agclose(g);
        }
        gvFreeContext(gvc);
    }

    // 2. 备用保障：若内存调用未取到输出，则调用本地打包的 dot.exe -Tplain-ext
    if (plainText.isEmpty()) {
        QString dotBin = GraphGenerator::findDotBinary();
        QProcess proc;
        proc.start(dotBin, QStringList() << QStringLiteral("-Tplain-ext"));
        if (proc.waitForStarted(2000)) {
            proc.write(dotContent.toUtf8());
            proc.closeWriteChannel();
            if (proc.waitForFinished(5000)) {
                if (proc.exitCode() == 0) {
                    plainText = QString::fromUtf8(proc.readAllStandardOutput());
                } else if (outError) {
                    *outError = QString::fromUtf8(proc.readAllStandardError());
                }
            }
        }
    }

    if (plainText.isEmpty()) {
        LayoutGraph graph;
        graph.isValid = false;
        graph.errorMessage = outError ? *outError : QStringLiteral("无法执行 Graphviz 布局计算");
        Logger::instance().log(LogLevel::Error, QStringLiteral("GraphLayout"), graph.errorMessage);
        return graph;
    }

    return parsePlainOutput(plainText, report);
}

LayoutGraph GraphLayoutEngine::parsePlainOutput(const QString& plainText,
                                                const StaticAnalysisReport* report)
{
    LayoutGraph graph;
    QStringList lines = plainText.split(QStringLiteral("\n"), Qt::SkipEmptyParts);

    qreal graphHeightInches = 0.0;

    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        QStringList tokens = parsePlainTokens(line);
        if (tokens.isEmpty()) continue;

        const QString& cmd = tokens[0];

        // 1. graph <scale> <width> <height>
        if (cmd == QStringLiteral("graph") && tokens.size() >= 4) {
            graph.width = tokens[2].toDouble() * 72.0;
            graphHeightInches = tokens[3].toDouble();
            graph.height = graphHeightInches * 72.0;
        }
        // 2. node <name> <x> <y> <width> <height> <label> <style> <shape> <color> <fillcolor>
        else if (cmd == QStringLiteral("node") && tokens.size() >= 11) {
            LayoutNode n;
            n.id = tokens[1];
            n.name = n.id;

            qreal cx = tokens[2].toDouble() * 72.0;
            qreal cy = (graphHeightInches - tokens[3].toDouble()) * 72.0;
            n.width = tokens[4].toDouble() * 72.0;
            n.height = tokens[5].toDouble() * 72.0;
            n.x = cx - (n.width / 2.0);
            n.y = cy - (n.height / 2.0);

            n.label = tokens[6];
            n.shape = tokens[8];
            n.color = tokens[9];
            n.fillColor = tokens[10];

            // 智能关联源代码行号与元数据 (支持函数调用图与 CFG 节点)
            if (report) {
                // 尝试按函数名匹配全局调用图节点
                for (const auto& f : report->functions) {
                    if (f.name == n.name) {
                        n.startLine = f.startLine;
                        n.endLine = f.endLine;
                        n.complexity = f.cyclomaticComplexity;
                        n.type = QStringLiteral("function");
                        break;
                    }
                }
            }

            // 若节点 ID 包含具体行号（如 CFG 节点 cond_42, stmt_75），提取行号
            static QRegularExpression reLine(QStringLiteral("(\\d+)"));
            auto match = reLine.match(n.id);
            if (match.hasMatch() && n.startLine == 0) {
                n.startLine = match.captured(1).toInt();
                n.endLine = n.startLine;
                n.type = QStringLiteral("statement");
            }

            graph.nodes.append(n);
        }
        // 3. edge <tail> <head> <n> <x1> <y1> ... <xn> <yn> [label xl yl] <style> <color>
        else if (cmd == QStringLiteral("edge") && tokens.size() >= 4) {
            LayoutEdge e;
            e.sourceId = tokens[1];
            e.targetId = tokens[2];
            int nPoints = tokens[3].toInt();

            int pointIdx = 4;
            if (tokens.size() >= pointIdx + nPoints * 2) {
                for (int i = 0; i < nPoints; ++i) {
                    qreal px = tokens[pointIdx + i * 2].toDouble() * 72.0;
                    qreal py = (graphHeightInches - tokens[pointIdx + i * 2 + 1].toDouble()) * 72.0;
                    e.splinePoints.append(QPointF(px, py));
                }

                int restIdx = pointIdx + nPoints * 2;
                // 检查是否存在边标签 [label xl yl]
                if (restIdx + 3 <= tokens.size() - 2) {
                    e.label = tokens[restIdx];
                    qreal lx = tokens[restIdx + 1].toDouble() * 72.0;
                    qreal ly = (graphHeightInches - tokens[restIdx + 2].toDouble()) * 72.0;
                    e.labelPos = QPointF(lx, ly);
                    restIdx += 3;
                }

                if (restIdx + 1 < tokens.size()) {
                    e.color = tokens.last();
                } else {
                    e.color = QStringLiteral("#89b4fa");
                }

                graph.edges.append(e);
            }
        }
    }

    graph.isValid = (!graph.nodes.isEmpty());
    return graph;
}

} // namespace Coverage
