#include "coverage/models.h"

namespace Coverage {

// StaticIssue
QJsonObject StaticIssue::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("id")] = id;
    obj[QStringLiteral("file")] = file;
    obj[QStringLiteral("line")] = line;
    obj[QStringLiteral("column")] = column;
    obj[QStringLiteral("ruleId")] = ruleId;
    obj[QStringLiteral("category")] = categoryToString(category);
    obj[QStringLiteral("severity")] = severityToString(severity);
    obj[QStringLiteral("message")] = message;
    obj[QStringLiteral("suggestion")] = suggestion;
    obj[QStringLiteral("messageZh")] = messageZh;
    obj[QStringLiteral("suggestionZh")] = suggestionZh;
    obj[QStringLiteral("messageEn")] = messageEn;
    obj[QStringLiteral("suggestionEn")] = suggestionEn;
    obj[QStringLiteral("codeSnippet")] = codeSnippet;
    return obj;
}

StaticIssue StaticIssue::fromJson(const QJsonObject& json) {
    StaticIssue issue;
    issue.id = json[QStringLiteral("id")].toString();
    issue.file = json[QStringLiteral("file")].toString();
    issue.line = json[QStringLiteral("line")].toInt();
    issue.column = json[QStringLiteral("column")].toInt();
    issue.ruleId = json[QStringLiteral("ruleId")].toString();
    issue.message = json[QStringLiteral("message")].toString();
    issue.suggestion = json[QStringLiteral("suggestion")].toString();
    issue.messageZh = json[QStringLiteral("messageZh")].toString();
    issue.suggestionZh = json[QStringLiteral("suggestionZh")].toString();
    issue.messageEn = json[QStringLiteral("messageEn")].toString();
    issue.suggestionEn = json[QStringLiteral("suggestionEn")].toString();
    issue.codeSnippet = json[QStringLiteral("codeSnippet")].toString();
    return issue;
}

// CodeMetrics
QJsonObject CodeMetrics::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("filePath")] = filePath;
    obj[QStringLiteral("totalLines")] = totalLines;
    obj[QStringLiteral("statementCount")] = statementCount;
    obj[QStringLiteral("branchCount")] = branchCount;
    obj[QStringLiteral("commentLines")] = commentLines;
    obj[QStringLiteral("commentRatio")] = commentRatio;
    obj[QStringLiteral("mcdcDecisions")] = mcdcDecisions;
    obj[QStringLiteral("mcdcConditions")] = mcdcConditions;
    obj[QStringLiteral("functionCount")] = functionCount;
    obj[QStringLiteral("totalComplexity")] = totalComplexity;
    obj[QStringLiteral("maxComplexity")] = maxComplexity;
    return obj;
}

CodeMetrics CodeMetrics::fromJson(const QJsonObject& json) {
    CodeMetrics m;
    m.filePath = json[QStringLiteral("filePath")].toString();
    m.totalLines = json[QStringLiteral("totalLines")].toInt();
    m.statementCount = json[QStringLiteral("statementCount")].toInt();
    m.branchCount = json[QStringLiteral("branchCount")].toInt();
    m.commentLines = json[QStringLiteral("commentLines")].toInt();
    m.commentRatio = json[QStringLiteral("commentRatio")].toDouble();
    m.mcdcDecisions = json[QStringLiteral("mcdcDecisions")].toInt();
    m.mcdcConditions = json[QStringLiteral("mcdcConditions")].toInt();
    m.functionCount = json[QStringLiteral("functionCount")].toInt();
    m.totalComplexity = json[QStringLiteral("totalComplexity")].toInt();
    m.maxComplexity = json[QStringLiteral("maxComplexity")].toInt(1);
    return m;
}

// FunctionPath
QJsonObject FunctionPath::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("pathId")] = pathId;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("conditionDescription")] = conditionDescription;
    QJsonArray linesArr;
    for (int l : executedLines) linesArr.append(l);
    obj[QStringLiteral("executedLines")] = linesArr;
    QJsonArray nodesArr;
    for (const auto& n : visitedNodeIds) nodesArr.append(n);
    obj[QStringLiteral("visitedNodeIds")] = nodesArr;
    obj[QStringLiteral("isErrorPath")] = isErrorPath;
    return obj;
}

FunctionPath FunctionPath::fromJson(const QJsonObject& json) {
    FunctionPath p;
    p.pathId = json[QStringLiteral("pathId")].toInt(1);
    p.name = json[QStringLiteral("name")].toString();
    p.conditionDescription = json[QStringLiteral("conditionDescription")].toString();
    QJsonArray linesArr = json[QStringLiteral("executedLines")].toArray();
    for (auto val : linesArr) p.executedLines.append(val.toInt());
    QJsonArray nodesArr = json[QStringLiteral("visitedNodeIds")].toArray();
    for (auto val : nodesArr) p.visitedNodeIds.append(val.toString());
    p.isErrorPath = json[QStringLiteral("isErrorPath")].toBool();
    return p;
}

// FunctionMetrics
QJsonObject FunctionMetrics::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("returnType")] = returnType;
    obj[QStringLiteral("filePath")] = filePath;
    obj[QStringLiteral("startLine")] = startLine;
    obj[QStringLiteral("endLine")] = endLine;
    obj[QStringLiteral("cyclomaticComplexity")] = cyclomaticComplexity;
    obj[QStringLiteral("parameterCount")] = parameterCount;
    obj[QStringLiteral("linesOfCode")] = linesOfCode;
    obj[QStringLiteral("statementCount")] = statementCount;
    obj[QStringLiteral("branchCount")] = branchCount;
    obj[QStringLiteral("mcdcCount")] = mcdcCount;
    obj[QStringLiteral("maxNestingDepth")] = maxNestingDepth;
    obj[QStringLiteral("fanIn")] = fanIn;
    obj[QStringLiteral("fanOut")] = fanOut;
    obj[QStringLiteral("functionDepth")] = functionDepth;
    obj[QStringLiteral("callDepth")] = callDepth;
    obj[QStringLiteral("isRecursive")] = isRecursive;
    return obj;
}

FunctionMetrics FunctionMetrics::fromJson(const QJsonObject& json) {
    FunctionMetrics f;
    f.name = json[QStringLiteral("name")].toString();
    f.returnType = json[QStringLiteral("returnType")].toString();
    f.filePath = json[QStringLiteral("filePath")].toString();
    f.startLine = json[QStringLiteral("startLine")].toInt();
    f.endLine = json[QStringLiteral("endLine")].toInt();
    f.cyclomaticComplexity = json[QStringLiteral("cyclomaticComplexity")].toInt(1);
    f.parameterCount = json[QStringLiteral("parameterCount")].toInt();
    f.linesOfCode = json[QStringLiteral("linesOfCode")].toInt();
    f.statementCount = json[QStringLiteral("statementCount")].toInt();
    f.branchCount = json[QStringLiteral("branchCount")].toInt();
    f.mcdcCount = json[QStringLiteral("mcdcCount")].toInt();
    f.maxNestingDepth = json[QStringLiteral("maxNestingDepth")].toInt();
    f.fanIn = json[QStringLiteral("fanIn")].toInt(0);
    f.fanOut = json[QStringLiteral("fanOut")].toInt(0);
    f.functionDepth = json[QStringLiteral("functionDepth")].toInt(1);
    f.callDepth = json[QStringLiteral("callDepth")].toInt(1);
    f.isRecursive = json[QStringLiteral("isRecursive")].toBool();
    return f;
}

// FunctionCoverageInfo
QJsonObject FunctionCoverageInfo::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("index")] = index;
    obj[QStringLiteral("functionName")] = functionName;
    obj[QStringLiteral("fileName")] = fileName;
    obj[QStringLiteral("statementCount")] = statementCount;
    obj[QStringLiteral("coveredStatements")] = coveredStatements;
    obj[QStringLiteral("statementCoveragePercent")] = statementCoveragePercent;
    obj[QStringLiteral("branchCount")] = branchCount;
    obj[QStringLiteral("coveredBranches")] = coveredBranches;
    obj[QStringLiteral("branchCoveragePercent")] = branchCoveragePercent;
    obj[QStringLiteral("startLine")] = startLine;
    obj[QStringLiteral("endLine")] = endLine;
    return obj;
}

FunctionCoverageInfo FunctionCoverageInfo::fromJson(const QJsonObject& json) {
    FunctionCoverageInfo fc;
    fc.index = json[QStringLiteral("index")].toInt(0);
    fc.functionName = json[QStringLiteral("functionName")].toString();
    fc.fileName = json[QStringLiteral("fileName")].toString();
    fc.statementCount = json[QStringLiteral("statementCount")].toInt(0);
    fc.coveredStatements = json[QStringLiteral("coveredStatements")].toInt(0);
    fc.statementCoveragePercent = json[QStringLiteral("statementCoveragePercent")].toDouble(0.0);
    fc.branchCount = json[QStringLiteral("branchCount")].toInt(0);
    fc.coveredBranches = json[QStringLiteral("coveredBranches")].toInt(0);
    fc.branchCoveragePercent = json[QStringLiteral("branchCoveragePercent")].toDouble(0.0);
    fc.startLine = json[QStringLiteral("startLine")].toInt(0);
    fc.endLine = json[QStringLiteral("endLine")].toInt(0);
    return fc;
}

// StaticAnalysisReport
int StaticAnalysisReport::errorCount() const {
    int cnt = 0;
    for (const auto& issue : issues) {
        if (issue.severity == Severity::Error || issue.severity == Severity::Critical) cnt++;
    }
    return cnt;
}

int StaticAnalysisReport::warningCount() const {
    int cnt = 0;
    for (const auto& issue : issues) {
        if (issue.severity == Severity::Warning) cnt++;
    }
    return cnt;
}

int StaticAnalysisReport::styleCount() const {
    int cnt = 0;
    for (const auto& issue : issues) {
        if (issue.severity == Severity::Style || issue.severity == Severity::Info) cnt++;
    }
    return cnt;
}

QJsonObject StaticAnalysisReport::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("filePath")] = filePath;
    obj[QStringLiteral("totalLines")] = totalLines;
    obj[QStringLiteral("codeLines")] = codeLines;
    obj[QStringLiteral("commentLines")] = commentLines;
    obj[QStringLiteral("blankLines")] = blankLines;
    obj[QStringLiteral("timestamp")] = timestamp.toString(Qt::ISODate);
    obj[QStringLiteral("elapsedMs")] = elapsedMs;

    QJsonArray issuesArray;
    for (const auto& issue : issues) {
        issuesArray.append(issue.toJson());
    }
    obj[QStringLiteral("issues")] = issuesArray;

    QJsonArray funcsArray;
    for (const auto& f : functions) {
        funcsArray.append(f.toJson());
    }
    obj[QStringLiteral("functions")] = funcsArray;
    obj[QStringLiteral("metrics")] = metrics.toJson();

    return obj;
}

StaticAnalysisReport StaticAnalysisReport::fromJson(const QJsonObject& json) {
    StaticAnalysisReport r;
    r.filePath = json[QStringLiteral("filePath")].toString();
    r.totalLines = json[QStringLiteral("totalLines")].toInt();
    r.codeLines = json[QStringLiteral("codeLines")].toInt();
    r.commentLines = json[QStringLiteral("commentLines")].toInt();
    r.blankLines = json[QStringLiteral("blankLines")].toInt();
    r.timestamp = QDateTime::fromString(json[QStringLiteral("timestamp")].toString(), Qt::ISODate);
    r.elapsedMs = json[QStringLiteral("elapsedMs")].toInteger();
    if (json.contains(QStringLiteral("metrics"))) {
        r.metrics = CodeMetrics::fromJson(json[QStringLiteral("metrics")].toObject());
    }

    QJsonArray issuesArray = json[QStringLiteral("issues")].toArray();
    for (const auto& val : issuesArray) {
        r.issues.append(StaticIssue::fromJson(val.toObject()));
    }

    QJsonArray funcsArray = json[QStringLiteral("functions")].toArray();
    for (const auto& val : funcsArray) {
        r.functions.append(FunctionMetrics::fromJson(val.toObject()));
    }

    return r;
}

// BranchPoint
QJsonObject BranchPoint::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("branchId")] = branchId;
    obj[QStringLiteral("line")] = line;
    obj[QStringLiteral("condition")] = condition;
    obj[QStringLiteral("trueHitCount")] = trueHitCount;
    obj[QStringLiteral("falseHitCount")] = falseHitCount;
    return obj;
}

BranchPoint BranchPoint::fromJson(const QJsonObject& json) {
    BranchPoint bp;
    bp.branchId = json[QStringLiteral("branchId")].toInt();
    bp.line = json[QStringLiteral("line")].toInt();
    bp.condition = json[QStringLiteral("condition")].toString();
    bp.trueHitCount = json[QStringLiteral("trueHitCount")].toInteger();
    bp.falseHitCount = json[QStringLiteral("falseHitCount")].toInteger();
    return bp;
}

// LineCoverage
QJsonObject LineCoverage::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("lineNumber")] = lineNumber;
    obj[QStringLiteral("hitCount")] = hitCount;
    obj[QStringLiteral("status")] = coverageStatusToString(status);
    QJsonArray branchArr;
    for (const auto& b : branches) {
        branchArr.append(b.toJson());
    }
    obj[QStringLiteral("branches")] = branchArr;
    return obj;
}

LineCoverage LineCoverage::fromJson(const QJsonObject& json) {
    LineCoverage lc;
    lc.lineNumber = json[QStringLiteral("lineNumber")].toInt();
    lc.hitCount = json[QStringLiteral("hitCount")].toInteger();
    QJsonArray branchArr = json[QStringLiteral("branches")].toArray();
    for (const auto& bVal : branchArr) {
        lc.branches.append(BranchPoint::fromJson(bVal.toObject()));
    }
    return lc;
}

// CoverageReport
QJsonObject CoverageReport::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("filePath")] = filePath;
    obj[QStringLiteral("executableLines")] = executableLines;
    obj[QStringLiteral("coveredLines")] = coveredLines;
    obj[QStringLiteral("totalBranches")] = totalBranches;
    obj[QStringLiteral("coveredBranches")] = coveredBranches;
    obj[QStringLiteral("totalFunctions")] = totalFunctions;
    obj[QStringLiteral("coveredFunctions")] = coveredFunctions;
    obj[QStringLiteral("statementCoverage")] = statementCoveragePercent();
    obj[QStringLiteral("branchCoverage")] = branchCoveragePercent();
    obj[QStringLiteral("functionCoverage")] = functionCoveragePercent();
    obj[QStringLiteral("timestamp")] = timestamp.toString(Qt::ISODate);

    QJsonObject linesObj;
    for (auto it = lineDetails.constBegin(); it != lineDetails.constEnd(); ++it) {
        linesObj[QString::number(it.key())] = it.value().toJson();
    }
    obj[QStringLiteral("lines")] = linesObj;

    QJsonArray funcsArr;
    for (const auto& fc : functionCoverages) {
        funcsArr.append(fc.toJson());
    }
    obj[QStringLiteral("functionCoverages")] = funcsArr;

    QJsonObject fileReportsObj;
    for (auto it = fileReports.constBegin(); it != fileReports.constEnd(); ++it) {
        fileReportsObj[it.key()] = it.value().toJson();
    }
    obj[QStringLiteral("fileReports")] = fileReportsObj;

    return obj;
}

CoverageReport CoverageReport::fromJson(const QJsonObject& json) {
    CoverageReport r;
    r.filePath = json[QStringLiteral("filePath")].toString();
    r.executableLines = json[QStringLiteral("executableLines")].toInt();
    r.coveredLines = json[QStringLiteral("coveredLines")].toInt();
    r.totalBranches = json[QStringLiteral("totalBranches")].toInt();
    r.coveredBranches = json[QStringLiteral("coveredBranches")].toInt();
    r.totalFunctions = json[QStringLiteral("totalFunctions")].toInt();
    r.coveredFunctions = json[QStringLiteral("coveredFunctions")].toInt();
    r.timestamp = QDateTime::fromString(json[QStringLiteral("timestamp")].toString(), Qt::ISODate);

    QJsonObject linesObj = json[QStringLiteral("lines")].toObject();
    for (auto it = linesObj.constBegin(); it != linesObj.constEnd(); ++it) {
        r.lineDetails[it.key().toInt()] = LineCoverage::fromJson(it.value().toObject());
    }

    QJsonArray funcsArr = json[QStringLiteral("functionCoverages")].toArray();
    for (const auto& fVal : funcsArr) {
        r.functionCoverages.append(FunctionCoverageInfo::fromJson(fVal.toObject()));
    }

    QJsonObject fileReportsObj = json[QStringLiteral("fileReports")].toObject();
    for (auto it = fileReportsObj.constBegin(); it != fileReportsObj.constEnd(); ++it) {
        r.fileReports[it.key()] = CoverageReport::fromJson(it.value().toObject());
    }

    return r;
}

// StimulusStep
QJsonObject StimulusStep::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("timestampMs")] = timestampMs;
    QJsonObject sigObj;
    for (auto it = signalValues.constBegin(); it != signalValues.constEnd(); ++it) {
        sigObj[it.key()] = it.value();
    }
    obj[QStringLiteral("signals")] = sigObj;
    obj[QStringLiteral("rawPayload")] = rawPayload;
    return obj;
}

StimulusStep StimulusStep::fromJson(const QJsonObject& json) {
    StimulusStep s;
    s.timestampMs = json[QStringLiteral("timestampMs")].toDouble();
    s.rawPayload = json[QStringLiteral("rawPayload")].toString();
    QJsonObject sigObj = json[QStringLiteral("signals")].toObject();
    for (auto it = sigObj.constBegin(); it != sigObj.constEnd(); ++it) {
        s.signalValues[it.key()] = it.value().toDouble();
    }
    return s;
}

// StimulusPlan
QJsonObject StimulusPlan::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("scriptPath")] = scriptPath;
    obj[QStringLiteral("targetModule")] = targetModule;
    obj[QStringLiteral("description")] = description;
    obj[QStringLiteral("durationMs")] = durationMs;
    obj[QStringLiteral("generatedAt")] = generatedAt.toString(Qt::ISODate);

    QJsonArray sigNames;
    for (const auto& sig : signalNames) sigNames.append(sig);
    obj[QStringLiteral("signalNames")] = sigNames;

    QJsonArray stepArray;
    for (const auto& step : steps) stepArray.append(step.toJson());
    obj[QStringLiteral("steps")] = stepArray;

    return obj;
}

StimulusPlan StimulusPlan::fromJson(const QJsonObject& json) {
    StimulusPlan plan;
    plan.name = json[QStringLiteral("name")].toString();
    plan.scriptPath = json[QStringLiteral("scriptPath")].toString();
    plan.targetModule = json[QStringLiteral("targetModule")].toString();
    plan.description = json[QStringLiteral("description")].toString();
    plan.durationMs = json[QStringLiteral("durationMs")].toDouble();
    plan.generatedAt = QDateTime::fromString(json[QStringLiteral("generatedAt")].toString(), Qt::ISODate);

    QJsonArray sigNames = json[QStringLiteral("signalNames")].toArray();
    for (const auto& s : sigNames) plan.signalNames.append(s.toString());

    QJsonArray stepArray = json[QStringLiteral("steps")].toArray();
    for (const auto& sVal : stepArray) {
        plan.steps.append(StimulusStep::fromJson(sVal.toObject()));
    }

    return plan;
}

// StackFunctionInfo
QJsonObject StackFunctionInfo::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("sourceFile")] = sourceFile;
    obj[QStringLiteral("line")] = line;
    obj[QStringLiteral("address")] = QString::number(address, 16);
    obj[QStringLiteral("localStackBytes")] = localStackBytes;
    obj[QStringLiteral("frameType")] = frameType;
    obj[QStringLiteral("maxCallStackBytes")] = maxCallStackBytes;
    obj[QStringLiteral("isRecursive")] = isRecursive;
    obj[QStringLiteral("disassembly")] = disassembly;

    QJsonArray pArr;
    for (const auto& p : worstCasePath) pArr.append(p);
    obj[QStringLiteral("worstCasePath")] = pArr;

    QJsonArray ceArr;
    for (const auto& c : callees) ceArr.append(c);
    obj[QStringLiteral("callees")] = ceArr;

    QJsonArray crArr;
    for (const auto& c : callers) crArr.append(c);
    obj[QStringLiteral("callers")] = crArr;

    return obj;
}

StackFunctionInfo StackFunctionInfo::fromJson(const QJsonObject& json) {
    StackFunctionInfo info;
    info.name = json[QStringLiteral("name")].toString();
    info.sourceFile = json[QStringLiteral("sourceFile")].toString();
    info.line = json[QStringLiteral("line")].toInt();
    info.address = json[QStringLiteral("address")].toString().toULongLong(nullptr, 16);
    info.localStackBytes = json[QStringLiteral("localStackBytes")].toInt();
    info.frameType = json[QStringLiteral("frameType")].toString();
    info.maxCallStackBytes = json[QStringLiteral("maxCallStackBytes")].toInt();
    info.isRecursive = json[QStringLiteral("isRecursive")].toBool();
    info.disassembly = json[QStringLiteral("disassembly")].toString();

    QJsonArray pArr = json[QStringLiteral("worstCasePath")].toArray();
    for (const auto& v : pArr) info.worstCasePath.append(v.toString());

    QJsonArray ceArr = json[QStringLiteral("callees")].toArray();
    for (const auto& v : ceArr) info.callees.append(v.toString());

    QJsonArray crArr = json[QStringLiteral("callers")].toArray();
    for (const auto& v : crArr) info.callers.append(v.toString());

    return info;
}

// StackAnalysisReport
QJsonObject StackAnalysisReport::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("binaryPath")] = binaryPath;
    obj[QStringLiteral("architecture")] = architecture;
    obj[QStringLiteral("analyzedAt")] = analyzedAt.toString(Qt::ISODate);
    obj[QStringLiteral("totalFunctions")] = totalFunctions;
    obj[QStringLiteral("maxWorstCaseStackBytes")] = maxWorstCaseStackBytes;
    obj[QStringLiteral("worstCaseRootFunction")] = worstCaseRootFunction;
    obj[QStringLiteral("hasRecursion")] = hasRecursion;
    obj[QStringLiteral("fullDisassembly")] = fullDisassembly;

    QJsonArray chainArr;
    for (const auto& f : worstCaseCallChain) chainArr.append(f);
    obj[QStringLiteral("worstCaseCallChain")] = chainArr;

    QJsonArray fnArr;
    for (const auto& fn : functions) fnArr.append(fn.toJson());
    obj[QStringLiteral("functions")] = fnArr;

    return obj;
}

StackAnalysisReport StackAnalysisReport::fromJson(const QJsonObject& json) {
    StackAnalysisReport rep;
    rep.binaryPath = json[QStringLiteral("binaryPath")].toString();
    rep.architecture = json[QStringLiteral("architecture")].toString();
    rep.analyzedAt = QDateTime::fromString(json[QStringLiteral("analyzedAt")].toString(), Qt::ISODate);
    rep.totalFunctions = json[QStringLiteral("totalFunctions")].toInt();
    rep.maxWorstCaseStackBytes = json[QStringLiteral("maxWorstCaseStackBytes")].toInt();
    rep.worstCaseRootFunction = json[QStringLiteral("worstCaseRootFunction")].toString();
    rep.hasRecursion = json[QStringLiteral("hasRecursion")].toBool();
    rep.fullDisassembly = json[QStringLiteral("fullDisassembly")].toString();

    QJsonArray chainArr = json[QStringLiteral("worstCaseCallChain")].toArray();
    for (const auto& v : chainArr) rep.worstCaseCallChain.append(v.toString());

    QJsonArray fnArr = json[QStringLiteral("functions")].toArray();
    for (const auto& v : fnArr) rep.functions.append(StackFunctionInfo::fromJson(v.toObject()));

    return rep;
}

} // namespace Coverage
