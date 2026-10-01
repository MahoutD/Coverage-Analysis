#include "coverage/backend_service.h"
#include "coverage/report_generator.h"
#include "coverage/logger.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <iostream>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    Coverage::Logger::instance().init();
    app.setApplicationName(QStringLiteral("coverage_cli"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Embedded C Coverage & Static Analysis CLI (Headless Core Engine)"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fileOption(QStringList() << QStringLiteral("f") << QStringLiteral("file"),
                                  QStringLiteral("Embedded C source file to analyze/instrument"),
                                  QStringLiteral("file"));
    QCommandLineOption analyzeOption(QStringList() << QStringLiteral("a") << QStringLiteral("analyze"),
                                     QStringLiteral("Run static analysis on target source file"));
    QCommandLineOption instrumentOption(QStringList() << QStringLiteral("i") << QStringLiteral("instrument"),
                                        QStringLiteral("Instrument source code with coverage probes"));
    QCommandLineOption outputOption(QStringList() << QStringLiteral("o") << QStringLiteral("output"),
                                    QStringLiteral("Output path for instrumented code or report"),
                                    QStringLiteral("output"));
    QCommandLineOption stimulusOption(QStringList() << QStringLiteral("s") << QStringLiteral("stimulus"),
                                      QStringLiteral("Python stimulus script to execute for test vector injection"),
                                      QStringLiteral("script.py"));
    QCommandLineOption reportOption(QStringList() << QStringLiteral("r") << QStringLiteral("report"),
                                    QStringLiteral("Export analysis and coverage report to JSON"),
                                    QStringLiteral("report.json"));
    QCommandLineOption docxOption(QStringList() << QStringLiteral("w") << QStringLiteral("docx"),
                                  QStringLiteral("Export comprehensive report to genuine Word (.docx)"),
                                  QStringLiteral("report.docx"));
    QCommandLineOption htmlOption(QStringList() << QStringLiteral("m") << QStringLiteral("html"),
                                  QStringLiteral("Export comprehensive report to HTML format"),
                                  QStringLiteral("report.html"));

    parser.addOption(fileOption);
    parser.addOption(analyzeOption);
    parser.addOption(instrumentOption);
    parser.addOption(outputOption);
    parser.addOption(stimulusOption);
    parser.addOption(reportOption);
    parser.addOption(docxOption);
    parser.addOption(htmlOption);

    parser.process(app);

    QString sourceFile = parser.value(fileOption);
    if (sourceFile.isEmpty()) {
        std::cerr << "Error: Target source file (-f / --file) is required." << std::endl;
        parser.showHelp(1);
        return 1;
    }

    auto& backend = Coverage::BackendService::instance();
    backend.loadSourceFile(sourceFile);

    // 1. Static Analysis
    if (parser.isSet(analyzeOption) || (!parser.isSet(instrumentOption) && !parser.isSet(stimulusOption))) {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << " STATIC ANALYSIS REPORT: " << sourceFile.toStdString() << std::endl;
        std::cout << "=======================================================" << std::endl;
        backend.runStaticAnalysis();
        const auto& report = backend.latestStaticReport();

        std::cout << "Total Lines: " << report.totalLines
                  << " | Code Lines: " << report.codeLines
                  << " | Comments: " << report.commentLines << std::endl;
        std::cout << "Issues Found: " << report.issues.size()
                  << " (Errors: " << report.errorCount()
                  << ", Warnings: " << report.warningCount() << ")\n" << std::endl;

        for (const auto& issue : report.issues) {
            std::cout << "[" << Coverage::severityToString(issue.severity).toStdString() << "] "
                      << issue.ruleId.toStdString()
                      << " at Line " << issue.line << ":" << issue.column << "\n  "
                      << issue.message.toStdString() << "\n  Suggestion: "
                      << issue.suggestion.toStdString() << "\n" << std::endl;
        }

        std::cout << "--- Function Metrics & Complexity ---" << std::endl;
        for (const auto& f : report.functions) {
            std::cout << "  " << f.name.toStdString() << "() -> Complexity: "
                      << f.cyclomaticComplexity << ", LOC: " << f.linesOfCode
                      << ", Max Nesting: " << f.maxNestingDepth
                      << (f.isRecursive ? " [RECURSION ERROR]" : "") << std::endl;
        }
    }

    // 2. Instrumentation
    if (parser.isSet(instrumentOption)) {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << " INSTRUMENTING SOURCE CODE..." << std::endl;
        std::cout << "=======================================================" << std::endl;
        QString outPath = parser.value(outputOption);
        backend.runInstrumentation(outPath);
        const auto& meta = backend.latestMetadata();
        std::cout << "Generated: " << meta.instrumentedFilePath.toStdString() << std::endl;
        std::cout << "Probes: " << meta.totalProbeLines << " line probes, "
                  << meta.totalProbeBranches << " branch probes, "
                  << meta.totalProbeFunctions << " function probes." << std::endl;
    }

    // 3. Stimulus Injection
    if (parser.isSet(stimulusOption)) {
        QString script = parser.value(stimulusOption);
        std::cout << "\n=======================================================" << std::endl;
        std::cout << " EXECUTING PYTHON STIMULUS: " << script.toStdString() << std::endl;
        std::cout << "=======================================================" << std::endl;
        backend.runStimulusScript(script);
        // Wait briefly for process
        QProcess::execute(backend.stimulusEngine().pythonExecutable(), {script});
    }

    // 4. Report export (JSON, Word .docx, HTML)
    if (parser.isSet(reportOption)) {
        QString reportFile = parser.value(reportOption);
        QFileInfo fi(reportFile);
        if (!fi.dir().exists()) fi.dir().mkpath(QStringLiteral("."));
        backend.exportReportToJson(reportFile);
        std::cout << "\nJSON Report exported to: " << reportFile.toStdString() << std::endl;
    }

    if (parser.isSet(docxOption)) {
        QString docxFile = parser.value(docxOption);
        QFileInfo fi(docxFile);
        if (!fi.dir().exists()) fi.dir().mkpath(QStringLiteral("."));
        bool ok = Coverage::ReportGenerator::generateDocxReport(docxFile,
                                                               backend.latestStaticReport(),
                                                               backend.latestCoverageReport(),
                                                               sourceFile,
                                                               backend.latestStimulusPlan());
        if (ok) {
            std::cout << "\nGenuine Word (.docx) Report exported to: " << docxFile.toStdString() << std::endl;
        } else {
            std::cerr << "\nFailed to export Word report to: " << docxFile.toStdString() << std::endl;
        }
    }

    if (parser.isSet(htmlOption)) {
        QString htmlFile = parser.value(htmlOption);
        QFileInfo fi(htmlFile);
        if (!fi.dir().exists()) fi.dir().mkpath(QStringLiteral("."));
        QString htmlContent = Coverage::ReportGenerator::generateHtmlReport(backend.latestStaticReport(),
                                                                           backend.latestCoverageReport(),
                                                                           sourceFile,
                                                                           backend.currentSourceContent(),
                                                                           backend.latestStimulusPlan());
        QFile f(htmlFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << htmlContent;
            f.close();
            std::cout << "\nHTML Report exported to: " << htmlFile.toStdString() << std::endl;
        } else {
            std::cerr << "\nFailed to write HTML report to: " << htmlFile.toStdString() << std::endl;
        }
    }

    return 0;
}
