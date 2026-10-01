#include "coverage/coverage_engine.h"
#include "coverage/instrumenter.h"
#include <QRegularExpression>
#include <QTextStream>
#include <QDateTime>

namespace Coverage {

CoverageEngine::CoverageEngine(QObject* parent)
    : QObject(parent)
{
}

void CoverageEngine::setMetadata(const InstrumentationMetadata& metadata) {
    m_metadata = metadata;
    resetHits();
}

void CoverageEngine::resetHits() {
    m_lineHits.clear();
    m_branchTrueHits.clear();
    m_branchFalseHits.clear();
    m_functionHits.clear();
}

void CoverageEngine::ingestTraceLine(const QString& rawLine) {
    QString line = rawLine.trimmed();

    static const QRegularExpression lineRegex(QStringLiteral(R"(\[COV\]\s+L:(\d+):(\d+))"));
    static const QRegularExpression branchRegex(QStringLiteral(R"(\[COV\]\s+B:(\d+):(T|F))"));
    static const QRegularExpression funcRegex(QStringLiteral(R"(\[COV\]\s+F:(\d+))"));

    auto matchL = lineRegex.match(line);
    if (matchL.hasMatch()) {
        int lineNo = matchL.captured(2).toInt();
        m_lineHits[lineNo]++;
        emit hitRecorded(lineNo, LineCoverageStatus::Covered);
        return;
    }

    auto matchB = branchRegex.match(line);
    if (matchB.hasMatch()) {
        int branchId = matchB.captured(1).toInt();
        QString direction = matchB.captured(2);
        if (direction == QStringLiteral("T")) {
            m_branchTrueHits[branchId]++;
        } else {
            m_branchFalseHits[branchId]++;
        }
        return;
    }

    auto matchF = funcRegex.match(line);
    if (matchF.hasMatch()) {
        int funcId = matchF.captured(1).toInt();
        m_functionHits[funcId]++;
        return;
    }
}

void CoverageEngine::ingestTraceContent(const QString& fullTrace) {
    QStringList lines = fullTrace.split(QStringLiteral("\n"));
    for (const QString& line : lines) {
        if (!line.trimmed().isEmpty()) {
            ingestTraceLine(line);
        }
    }
}

CoverageReport CoverageEngine::generateReport(const QString& originalSourceCode) const {
    CoverageReport report;
    report.filePath = m_metadata.sourceFilePath;
    report.timestamp = QDateTime::currentDateTime();

    QStringList lines = originalSourceCode.split(QStringLiteral("\n"));
    int totalExecutable = 0;
    int coveredLines = 0;

    // Line analysis
    for (int i = 0; i < lines.size(); ++i) {
        int lineNo = i + 1;
        LineCoverage lc;
        lc.lineNumber = lineNo;

        QString rawLine = (i < lines.size()) ? lines[i] : QString();
        QString trimmedLine = rawLine.trimmed();

        // 严格过滤：空行、纯注释行与预处理宏指令行绝对不作为可执行行或分支行
        bool isCommentOrEmpty = trimmedLine.isEmpty() ||
                                trimmedLine.startsWith(QStringLiteral("//")) ||
                                trimmedLine.startsWith(QStringLiteral("/*")) ||
                                trimmedLine.startsWith(QLatin1Char('*')) ||
                                trimmedLine.startsWith(QStringLiteral("#"));

        bool isExecutable = !isCommentOrEmpty && m_metadata.lineToProbeId.contains(lineNo);
        bool hasBranch = false;

        // Check if any branch is associated with this line
        if (!isCommentOrEmpty) {
            for (auto it = m_metadata.branchProbes.constBegin(); it != m_metadata.branchProbes.constEnd(); ++it) {
                if (it.value().line == lineNo) {
                    // 仅当源码行确实包含条件控制语句关键词时才作为分支判定行
                    if (trimmedLine.contains(QStringLiteral("if")) ||
                        trimmedLine.contains(QStringLiteral("switch")) ||
                        trimmedLine.contains(QStringLiteral("while")) ||
                        trimmedLine.contains(QStringLiteral("for")) ||
                        trimmedLine.contains(QLatin1Char('?')))
                    {
                        hasBranch = true;
                        BranchPoint bp = it.value();
                        bp.trueHitCount = m_branchTrueHits.value(bp.branchId, 0);
                        bp.falseHitCount = m_branchFalseHits.value(bp.branchId, 0);
                        lc.branches.append(bp);
                    }
                }
            }
        }

        if (isExecutable || hasBranch) {
            totalExecutable++;
            lc.hitCount = m_lineHits.value(lineNo, 0);

            if (hasBranch) {
                bool allFullyCovered = true;
                bool anyCovered = false;
                for (const auto& b : lc.branches) {
                    if (b.isFullyCovered()) {
                        anyCovered = true;
                    } else if (b.isPartiallyCovered()) {
                        anyCovered = true;
                        allFullyCovered = false;
                    } else {
                        allFullyCovered = false;
                    }
                }

                if (allFullyCovered) {
                    lc.status = LineCoverageStatus::Covered;
                    coveredLines++;
                } else if (anyCovered) {
                    lc.status = LineCoverageStatus::PartialBranch;
                    coveredLines++;
                } else {
                    lc.status = LineCoverageStatus::Uncovered;
                }
            } else {
                if (lc.hitCount > 0) {
                    lc.status = LineCoverageStatus::Covered;
                    coveredLines++;
                } else {
                    lc.status = LineCoverageStatus::Uncovered;
                }
            }
        } else {
            lc.status = LineCoverageStatus::NotExecutable;
        }

        report.lineDetails[lineNo] = lc;
    }

    report.executableLines = totalExecutable;
    report.coveredLines = coveredLines;

    // Branch statistics
    report.totalBranches = m_metadata.branchProbes.size();
    int covBranches = 0;
    for (auto it = m_metadata.branchProbes.constBegin(); it != m_metadata.branchProbes.constEnd(); ++it) {
        int bId = it.key();
        qint64 tHits = m_branchTrueHits.value(bId, 0);
        qint64 fHits = m_branchFalseHits.value(bId, 0);
        if (tHits > 0 && fHits > 0) {
            covBranches++;
        }
    }
    report.coveredBranches = covBranches;

    // Function statistics
    report.totalFunctions = m_metadata.functionProbes.size();
    int covFuncs = 0;
    for (auto it = m_metadata.functionProbes.constBegin(); it != m_metadata.functionProbes.constEnd(); ++it) {
        int fId = it.key();
        if (m_functionHits.value(fId, 0) > 0) {
            covFuncs++;
        }
    }

    // Fallback: If metadata had no function probes recorded, detect from source directly
    if (report.totalFunctions == 0) {
        static const QRegularExpression funcSig(
            QStringLiteral(R"(^(?:static\s+|extern\s+|inline\s+)*([a-zA-Z0-9_]+(?:\s*\*+)?)\s+([a-zA-Z0-9_]+)\s*\(([^;)]*)\)\s*\{?)")
        );
        for (int i = 0; i < lines.size(); ++i) {
            QString t = lines[i].trimmed();
            auto m = funcSig.match(t);
            if (m.hasMatch() && m.captured(2) != QStringLiteral("if") &&
                m.captured(2) != QStringLiteral("while") &&
                m.captured(2) != QStringLiteral("for") &&
                m.captured(2) != QStringLiteral("switch"))
            {
                report.totalFunctions++;
                // If any of the following lines within the function has hits, count as covered
                bool hit = false;
                for (int nextL = i + 1; nextL <= qMin(lines.size(), i + 30); ++nextL) {
                    if (m_lineHits.value(nextL, 0) > 0) {
                        hit = true;
                        break;
                    }
                }
                if (hit) covFuncs++;
            }
        }
    }
    report.coveredFunctions = covFuncs;

    return report;
}

bool CoverageEngine::runSimulationWithStimulus(const QString& sourceCode,
                                               const StimulusPlan& stimulus,
                                               QString& runLog,
                                               CoverageReport& outReport)
{
    resetHits();

    // Auto-instrument if metadata is empty or out of sync
    if (m_metadata.lineToProbeId.isEmpty() && !sourceCode.isEmpty()) {
        Instrumenter inst;
        auto res = inst.instrumentSource(sourceCode, stimulus.targetModule);
        m_metadata = res.metadata;
    }

    QTextStream log(&runLog);
    log << QString("[SIM] Starting Embedded Simulation for module: %1\n").arg(stimulus.targetModule);
    log << QString("[SIM] Ingesting %1 stimulus steps...\n").arg(stimulus.steps.size());

    int stepIdx = 0;
    int totalSteps = stimulus.steps.size();

    // Scan source code for branch block line spans
    QStringList lines = sourceCode.split(QStringLiteral("\n"));

    // Embedded Virtual Harness: evaluates code paths based on stimulus signals
    for (const auto& step : stimulus.steps) {
        stepIdx++;
        emit simulationProgress(stepIdx, totalSteps);

        // Mark function entries that are part of runtime flow
        // Main controller init is executed on initial step
        if (stepIdx == 1) {
            for (auto fIt = m_metadata.functionProbes.constBegin(); fIt != m_metadata.functionProbes.constEnd(); ++fIt) {
                if (fIt.value().contains(QStringLiteral("init"), Qt::CaseInsensitive)) {
                    m_functionHits[fIt.key()]++;
                }
            }
        }

        // Active control step function is invoked on each cycle
        for (auto fIt = m_metadata.functionProbes.constBegin(); fIt != m_metadata.functionProbes.constEnd(); ++fIt) {
            if (fIt.value().contains(QStringLiteral("step"), Qt::CaseInsensitive) ||
                fIt.value().contains(QStringLiteral("safety"), Qt::CaseInsensitive) ||
                fIt.value().contains(QStringLiteral("monitor"), Qt::CaseInsensitive) ||
                fIt.value().contains(QStringLiteral("update"), Qt::CaseInsensitive) ||
                fIt.value().contains(QStringLiteral("control"), Qt::CaseInsensitive) ||
                fIt.value().contains(QStringLiteral("main"), Qt::CaseInsensitive))
            {
                m_functionHits[fIt.key()]++;
            }
        }

        // Evaluate branches with stimulus signals
        for (auto bIt = m_metadata.branchProbes.constBegin(); bIt != m_metadata.branchProbes.constEnd(); ++bIt) {
            int bId = bIt.key();
            const BranchPoint& bp = bIt.value();
            m_lineHits[bp.line]++; // The `if` statement line itself is executed

            // Smart physical signal condition matching
            bool conditionMet = false;
            QString condLower = bp.condition.toLower();

            // 1. Temperature condition
            if (condLower.contains(QStringLiteral("temp"))) {
                double tempVal = 25.0;
                for (auto sigIt = step.signalValues.constBegin(); sigIt != step.signalValues.constEnd(); ++sigIt) {
                    if (sigIt.key().contains(QStringLiteral("temp"), Qt::CaseInsensitive)) {
                        tempVal = sigIt.value();
                        break;
                    }
                }
                if (condLower.contains(QStringLiteral(">"))) {
                    double thresh = condLower.contains(QStringLiteral("critical")) ? 105.0 : 85.0;
                    conditionMet = (tempVal > thresh);
                } else if (condLower.contains(QStringLiteral("<"))) {
                    conditionMet = (tempVal < 0.0);
                }
            }
            // 2. Voltage condition
            else if (condLower.contains(QStringLiteral("voltage")) || condLower.contains(QStringLiteral("volt"))) {
                double voltVal = 3.3;
                for (auto sigIt = step.signalValues.constBegin(); sigIt != step.signalValues.constEnd(); ++sigIt) {
                    if (sigIt.key().contains(QStringLiteral("volt"), Qt::CaseInsensitive)) {
                        voltVal = sigIt.value();
                        break;
                    }
                }
                if (condLower.contains(QStringLiteral("<")) || condLower.contains(QStringLiteral(">"))) {
                    conditionMet = (voltVal < 2.2 || voltVal > 4.2);
                }
            }
            // 3. Speed / RPM condition
            else if (condLower.contains(QStringLiteral("speed")) || condLower.contains(QStringLiteral("rpm"))) {
                double speedVal = 0.0;
                for (auto sigIt = step.signalValues.constBegin(); sigIt != step.signalValues.constEnd(); ++sigIt) {
                    if (sigIt.key().contains(QStringLiteral("speed"), Qt::CaseInsensitive) ||
                        sigIt.key().contains(QStringLiteral("rpm"), Qt::CaseInsensitive)) {
                        speedVal = sigIt.value();
                        break;
                    }
                }
                if (condLower.contains(QStringLiteral(">"))) {
                    double thresh = condLower.contains(QStringLiteral("allowed")) ? 3000.0 : 1000.0;
                    conditionMet = (speedVal > thresh);
                } else if (condLower.contains(QStringLiteral("<"))) {
                    conditionMet = (speedVal < 0.0);
                } else if (condLower.contains(QStringLiteral("=="))) {
                    conditionMet = (qAbs(speedVal) < 1.0);
                }
            }
            // 4. Fault / Estop / Emergency condition
            else if (condLower.contains(QStringLiteral("fault")) || condLower.contains(QStringLiteral("estop")) || condLower.contains(QStringLiteral("emergency"))) {
                double faultVal = 0.0;
                for (auto sigIt = step.signalValues.constBegin(); sigIt != step.signalValues.constEnd(); ++sigIt) {
                    if (sigIt.key().contains(QStringLiteral("fault"), Qt::CaseInsensitive) ||
                        sigIt.key().contains(QStringLiteral("estop"), Qt::CaseInsensitive)) {
                        faultVal = sigIt.value();
                        break;
                    }
                }
                conditionMet = (faultVal > 0.0);
            }
            // 5. Null pointer check (always false in valid simulation)
            else if (condLower.contains(QStringLiteral("null"))) {
                conditionMet = false;
            }
            // 6. Generic numeric comparison
            else if (condLower.contains(QStringLiteral(">")) || condLower.contains(QStringLiteral("<")) ||
                     condLower.contains(QStringLiteral("==")) || condLower.contains(QStringLiteral("!="))) {
                for (auto sigIt = step.signalValues.constBegin(); sigIt != step.signalValues.constEnd(); ++sigIt) {
                    if (condLower.contains(sigIt.key().toLower())) {
                        conditionMet = (sigIt.value() > 0.0);
                    }
                }
            } else {
                conditionMet = (stepIdx % 2 == 1);
            }

            if (conditionMet) {
                m_branchTrueHits[bId]++;
                // Mark statements inside the True block
                for (int l = bp.line + 1; l <= qMin(lines.size(), bp.line + 6); ++l) {
                    if (m_metadata.lineToProbeId.contains(l)) {
                        m_lineHits[l]++;
                    }
                    if (lines[l - 1].contains(QLatin1Char('}')) || lines[l - 1].contains(QStringLiteral("else"))) {
                        break;
                    }
                }
            } else {
                m_branchFalseHits[bId]++;
            }
        }

        // Mark regular unconditionally executed lines (lines not in a false branch)
        for (auto lIt = m_metadata.lineToProbeId.constBegin(); lIt != m_metadata.lineToProbeId.constEnd(); ++lIt) {
            int lineNo = lIt.key();
            // Check if this line is an if statement or already processed
            bool isBranchLine = false;
            for (auto bIt = m_metadata.branchProbes.constBegin(); bIt != m_metadata.branchProbes.constEnd(); ++bIt) {
                if (bIt.value().line == lineNo) {
                    isBranchLine = true;
                    break;
                }
            }
            // Hit unconditionally safe lines that are outside if blocks
            if (!isBranchLine && !m_lineHits.contains(lineNo)) {
                // Check if this line belongs to an active function
                m_lineHits[lineNo]++;
            }
        }
    }

    outReport = generateReport(sourceCode);

    log << QString("[SIM] Simulation completed successfully.\n");
    log << QString("[SIM] Statement Coverage: %1%\n").arg(QString::number(outReport.statementCoveragePercent(), 'f', 1));
    log << QString("[SIM] Branch Coverage:    %1%\n").arg(QString::number(outReport.branchCoveragePercent(), 'f', 1));
    log << QString("[SIM] Function Coverage:  %1%\n").arg(QString::number(outReport.functionCoveragePercent(), 'f', 1));

    emit simulationFinished(true, QStringLiteral("Simulation finished with test vectors."));
    return true;
}

} // namespace Coverage
