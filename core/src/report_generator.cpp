#include "coverage/report_generator.h"
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Coverage {

QString ReportGenerator::generateHtmlReport(const StaticAnalysisReport& staticReport,
                                            const CoverageReport& covReport,
                                            const QString& sourceFile,
                                            const QString& /* sourceContent */,
                                            const StimulusPlan& plan)
{
    QString genTime = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    QFileInfo fi(sourceFile);
    QString fileName = fi.fileName().isEmpty() ? QStringLiteral("embedded_source.c") : fi.fileName();

    QString html;
    html.reserve(32768);

    html += QStringLiteral(
R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>嵌入式 C 静态分析与覆盖率综合度量报告</title>
<style>
  body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; background-color: #f8f9fa; color: #212529; margin: 0; padding: 24px; line-height: 1.5; }
  .container { max-width: 1100px; margin: 0 auto; background: #ffffff; border-radius: 8px; box-shadow: 0 4px 16px rgba(0,0,0,0.08); padding: 32px; }
  .header { border-bottom: 2px solid #0d6efd; padding-bottom: 16px; margin-bottom: 24px; }
  .header h1 { margin: 0 0 8px 0; color: #0d6efd; font-size: 24px; }
  .header .meta { color: #6c757d; font-size: 13px; }
  .cards { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 28px; }
  .card { background: #f8f9fa; border: 1px solid #dee2e6; border-radius: 6px; padding: 16px; text-align: center; }
  .card .num { font-size: 26px; font-weight: bold; margin: 4px 0; }
  .card .label { font-size: 13px; color: #6c757d; }
  .card-cov { border-left: 4px solid #198754; }
  .card-branch { border-left: 4px solid #0d6efd; }
  .card-issues { border-left: 4px solid #dc3545; }
  .card-complex { border-left: 4px solid #ffc107; }
  h2 { font-size: 18px; color: #343a40; border-left: 4px solid #0d6efd; padding-left: 10px; margin-top: 32px; margin-bottom: 16px; }
  table { width: 100%; border-collapse: collapse; margin-bottom: 20px; font-size: 13px; }
  th, td { padding: 10px 12px; text-align: left; border: 1px solid #dee2e6; }
  th { background-color: #f1f3f5; font-weight: 600; }
  tr:nth-child(even) { background-color: #fdfdfd; }
  .badge { display: inline-block; padding: 3px 8px; font-size: 11px; font-weight: 600; border-radius: 4px; color: #fff; }
  .badge-critical { background-color: #dc3545; }
  .badge-error { background-color: #fd7e14; }
  .badge-warning { background-color: #ffc107; color: #212529; }
  .badge-info { background-color: #0dcaf0; color: #212529; }
  .badge-success { background-color: #198754; }
  .footer { margin-top: 40px; padding-top: 16px; border-top: 1px solid #dee2e6; text-align: center; color: #adb5bd; font-size: 12px; }
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>嵌入式 C 静态分析与覆盖率综合度量报告</h1>
    <div class="meta">
      <b>分析源文件:</b> )");
    html += fileName;
    html += QStringLiteral(" &nbsp;|&nbsp; <b>生成时间:</b> ");
    html += genTime;
    html += QStringLiteral(
R"( &nbsp;|&nbsp; <b>架构模式:</b> 前后端分离 C++17 纯算法内核
    </div>
  </div>

  <div class="cards">
    <div class="card card-cov">
      <div class="label">语句覆盖率 (Statement)</div>
      <div class="num" style="color: #198754;">)");
    html += QString::number(covReport.statementCoveragePercent(), 'f', 1) + QStringLiteral("%");
    html += QStringLiteral("</div><div class=\"label\">已覆盖: ") + QString::number(covReport.coveredLines) + QStringLiteral(" / 总 ") + QString::number(covReport.executableLines) + QStringLiteral(" 行</div></div>");

    html += QStringLiteral(
R"(    <div class="card card-branch">
      <div class="label">分支覆盖率 (Branch)</div>
      <div class="num" style="color: #0d6efd;">)");
    html += QString::number(covReport.branchCoveragePercent(), 'f', 1) + QStringLiteral("%");
    html += QStringLiteral("</div><div class=\"label\">已覆盖分支: ") + QString::number(covReport.coveredBranches) + QStringLiteral(" / 总 ") + QString::number(covReport.totalBranches) + QStringLiteral("</div></div>");

    html += QStringLiteral(
R"(    <div class="card card-issues">
      <div class="label">检测缺陷违规</div>
      <div class="num" style="color: #dc3545;">)");
    html += QString::number(staticReport.issues.size());
    html += QStringLiteral("</div><div class=\"label\">严重: ") + QString::number(staticReport.errorCount()) + QStringLiteral(" | 警告: ") + QString::number(staticReport.warningCount()) + QStringLiteral("</div></div>");

    html += QStringLiteral(
R"(    <div class="card card-complex">
      <div class="label">最大函数圈复杂度</div>
      <div class="num" style="color: #b58900;">)");
    int maxCc = 1;
    for (const auto& f : staticReport.functions) {
        if (f.cyclomaticComplexity > maxCc) maxCc = f.cyclomaticComplexity;
    }
    html += QString::number(maxCc);
    html += QStringLiteral("</div><div class=\"label\">被测函数总数: ") + QString::number(staticReport.functions.size()) + QStringLiteral(" 个</div></div></div>");

    // 1. 静态缺陷列表
    html += QStringLiteral("<h2>1. 静态安全与 MISRA C:2012 规则检查缺陷清单</h2>");
    if (staticReport.issues.isEmpty()) {
        html += QStringLiteral("<p style='color: #198754;'><b>🎉 优秀！未检出任何 MISRA 违规或嵌入式安全风险。</b></p>");
    } else {
        html += QStringLiteral(
            "<table><thead><tr>"
            "<th style='width: 50px;'>序号</th>"
            "<th style='width: 90px;'>级别</th>"
            "<th style='width: 170px;'>规则编号</th>"
            "<th style='width: 80px;'>位置</th>"
            "<th>缺陷描述</th>"
            "<th>建议整改措施</th>"
            "</tr></thead><tbody>"
        );
        int idx = 1;
        for (const auto& iss : staticReport.issues) {
            QString badgeClass = QStringLiteral("badge-warning");
            if (iss.severity == Severity::Critical) badgeClass = QStringLiteral("badge-critical");
            else if (iss.severity == Severity::Error) badgeClass = QStringLiteral("badge-error");
            else if (iss.severity == Severity::Info) badgeClass = QStringLiteral("badge-info");

            html += QString("<tr><td>%1</td><td><span class='badge %2'>%3</span></td><td><b>%4</b></td><td>第 %5 行</td><td>%6</td><td><i>%7</i></td></tr>")
                        .arg(idx++)
                        .arg(badgeClass)
                        .arg(severityToString(iss.severity))
                        .arg(iss.ruleId)
                        .arg(iss.line)
                        .arg(iss.message.toHtmlEscaped())
                        .arg(iss.suggestion.toHtmlEscaped());
        }
        html += QStringLiteral("</tbody></table>");
    }

    // 2. 函数度量与复杂度列表
    html += QStringLiteral("<h2>2. 函数度量与控制结构指标分布</h2>");
    html += QStringLiteral(
        "<table><thead><tr>"
        "<th>序号</th>"
        "<th>函数名称</th>"
        "<th>行号区间</th>"
        "<th>语句数</th>"
        "<th>分支数</th>"
        "<th>圈复杂度</th>"
        "<th>扇入数</th>"
        "<th>扇出数</th>"
        "<th>函数深度</th>"
        "<th>调用深度</th>"
        "<th>复杂度评定</th>"
        "</tr></thead><tbody>"
    );
    int fIndex = 0;
    for (const auto& f : staticReport.functions) {
        QString statusBadge = QStringLiteral("<span class='badge badge-success'>良好 (&le;5)</span>");
        if (f.cyclomaticComplexity > 15) {
            statusBadge = QStringLiteral("<span class='badge badge-critical'>严重超标 (>15 需重构)</span>");
        } else if (f.cyclomaticComplexity > 10) {
            statusBadge = QStringLiteral("<span class='badge badge-warning'>偏高 (11-15 告警)</span>");
        } else if (f.cyclomaticComplexity > 5) {
            statusBadge = QStringLiteral("<span class='badge badge-info'>中等 (6-10)</span>");
        }

        html += QString("<tr><td>%1</td><td><b>%2()</b></td><td>L%3 ~ L%4</td><td>%5</td><td>%6</td><td><b>%7</b></td><td>%8</td><td>%9</td><td>%10 层</td><td>第 %11 层</td><td>%12</td></tr>")
                    .arg(fIndex++)
                    .arg(f.name)
                    .arg(f.startLine)
                    .arg(f.endLine)
                    .arg(f.statementCount)
                    .arg(f.branchCount)
                    .arg(f.cyclomaticComplexity)
                    .arg(f.fanIn)
                    .arg(f.fanOut)
                    .arg(f.functionDepth)
                    .arg(f.callDepth)
                    .arg(statusBadge);
    }
    html += QStringLiteral("</tbody></table>");

    // 3. 各函数测试覆盖度量统计明细 (匹配工业级 9 列标准)
    if (!covReport.functionCoverages.isEmpty()) {
        html += QStringLiteral("<h2>3. 各函数测试覆盖度量统计清单 (Function Coverage Statistics)</h2>");
        html += QStringLiteral(
            "<table><thead><tr>"
            "<th style='width: 45px;'>序号</th>"
            "<th>函数名称</th>"
            "<th>文件名</th>"
            "<th>语句数</th>"
            "<th>语句覆盖数</th>"
            "<th>语句覆盖率(%)</th>"
            "<th>分支数</th>"
            "<th>分支覆盖数</th>"
            "<th>分支覆盖率(%)</th>"
            "</tr></thead><tbody>"
        );
        for (const auto& fc : covReport.functionCoverages) {
            QString stmtBadge = fc.statementCoveragePercent >= 80.0 ? QStringLiteral("badge-success") :
                                fc.statementCoveragePercent >= 50.0 ? QStringLiteral("badge-warning") : QStringLiteral("badge-critical");
            QString branchBadge = fc.branchCoveragePercent >= 80.0 ? QStringLiteral("badge-success") :
                                  fc.branchCoveragePercent >= 50.0 ? QStringLiteral("badge-warning") : QStringLiteral("badge-critical");

            html += QString("<tr><td>%1</td><td><b>%2</b></td><td>%3</td><td>%4</td><td>%5</td><td><span class='badge %6'>%7%</span></td><td>%8</td><td>%9</td><td><span class='badge %10'>%11%</span></td></tr>")
                        .arg(fc.index)
                        .arg(fc.functionName)
                        .arg(fc.fileName)
                        .arg(fc.statementCount)
                        .arg(fc.coveredStatements)
                        .arg(stmtBadge)
                        .arg(QString::number(fc.statementCoveragePercent, 'f', 2))
                        .arg(fc.branchCount)
                        .arg(fc.coveredBranches)
                        .arg(branchBadge)
                        .arg(QString::number(fc.branchCoveragePercent, 'f', 2));
        }
        html += QStringLiteral("</tbody></table>");
    }

    // 3. 激励向量执行摘要
    if (!plan.steps.isEmpty()) {
        html += QStringLiteral("<h2>3. Python 激励注入时序向量记录</h2>");
        html += QString("<p>激励方案：<b>%1</b>，共包含 <b>%2</b> 个时隙的激励向量。</p>")
                    .arg(plan.name.isEmpty() ? QStringLiteral("动态外部注入") : plan.name)
                    .arg(plan.steps.size());
        html += QStringLiteral(
            "<table><thead><tr>"
            "<th style='width: 60px;'>步骤</th>"
            "<th style='width: 100px;'>时间 (ms)</th>"
            "<th>注入信号与键值状态</th>"
            "</tr></thead><tbody>"
        );
        int sIdx = 1;
        int showLimit = qMin(plan.steps.size(), 10);
        for (int i = 0; i < showLimit; ++i) {
            const auto& step = plan.steps[i];
            QString sigStr;
            for (auto it = step.signalValues.constBegin(); it != step.signalValues.constEnd(); ++it) {
                sigStr += QString("%1 = %2; ").arg(it.key()).arg(it.value());
            }
            html += QString("<tr><td>%1</td><td>+%2 ms</td><td><code>%3</code></td></tr>")
                        .arg(sIdx++)
                        .arg(step.timestampMs)
                        .arg(sigStr.toHtmlEscaped());
        }
        if (plan.steps.size() > 10) {
            html += QString("<tr><td colspan='3' style='text-align: center; color: #6c757d;'>... 剩余 %1 组时序步骤已省略 ...</td></tr>")
                        .arg(plan.steps.size() - 10);
        }
        html += QStringLiteral("</tbody></table>");
    }

    html += QStringLiteral(
R"(  <div class="footer">
    本报告由 嵌入式 C 覆盖分析与静态分析套件 自动生成 | 前后端解耦 C++17 纯算法引擎
  </div>
</div>
</body>
</html>
)");

    return html;
}

QString ReportGenerator::generateWordDocument(const QString& htmlReport) {
    // 采用 Word 规范的 MHTML / HTML 包装，让 Microsoft Word 与 WPS 均可无损直接识别为可编辑排版文档
    QString wordDoc;
    wordDoc += QStringLiteral(
R"(<html xmlns:o='urn:schemas-microsoft-com:office:office'
      xmlns:w='urn:schemas-microsoft-com:office:word'
      xmlns='http://www.w3.org/TR/REC-html40'>
<head>
<!--[if gte mso 9]>
<xml>
<w:WordDocument>
<w:View>Print</w:View>
<w:Zoom>100</w:Zoom>
<w:DoNotOptimizeForBrowser/>
</w:WordDocument>
</xml>
<![endif]-->
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>
<body>
)");
    wordDoc += htmlReport;
    wordDoc += QStringLiteral("</body></html>");
    return wordDoc;
}

// -----------------------------------------------------------------------------
// 原生 OpenXML .docx 生成器与内置轻量 ZIP 封包引擎
// -----------------------------------------------------------------------------
struct ZipFileEntry {
    QString path;
    QByteArray data;
};

static uint32_t calcCrc32(const QByteArray& data) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data.constData());
    size_t n = data.size();
    for (size_t i = 0; i < n; ++i) {
        crc ^= p[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

static bool writeZipArchive(const QString& outPath, const QVector<ZipFileEntry>& entries) {
    QFileInfo fi(outPath);
    if (!fi.dir().exists()) {
        fi.dir().mkpath(QStringLiteral("."));
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QByteArray centralDirectory;

    for (const auto& entry : entries) {
        uint32_t offset = static_cast<uint32_t>(file.pos());

        QByteArray fnUtf8 = entry.path.toUtf8();
        uint32_t crc = calcCrc32(entry.data);
        uint32_t sz = static_cast<uint32_t>(entry.data.size());

        // Local header (30 bytes)
        QByteArray loc;
        loc.resize(30);
        uint8_t* h = reinterpret_cast<uint8_t*>(loc.data());
        h[0] = 0x50; h[1] = 0x4b; h[2] = 0x03; h[3] = 0x04;
        h[4] = 20; h[5] = 0;
        h[6] = 0; h[7] = 0x08; // flags (UTF-8)
        h[8] = 0; h[9] = 0; // store (no compression)
        h[10] = 0; h[11] = 0;
        h[12] = 0; h[13] = 0;
        h[14] = crc & 0xFF; h[15] = (crc >> 8) & 0xFF; h[16] = (crc >> 16) & 0xFF; h[17] = (crc >> 24) & 0xFF;
        h[18] = sz & 0xFF; h[19] = (sz >> 8) & 0xFF; h[20] = (sz >> 16) & 0xFF; h[21] = (sz >> 24) & 0xFF;
        h[22] = sz & 0xFF; h[23] = (sz >> 8) & 0xFF; h[24] = (sz >> 16) & 0xFF; h[25] = (sz >> 24) & 0xFF;
        uint16_t fnLen = static_cast<uint16_t>(fnUtf8.size());
        h[26] = fnLen & 0xFF; h[27] = (fnLen >> 8) & 0xFF;
        h[28] = 0; h[29] = 0; // extra len
        file.write(loc);
        file.write(fnUtf8);
        file.write(entry.data);

        // Central dir header (46 bytes)
        QByteArray cd;
        cd.resize(46);
        uint8_t* c = reinterpret_cast<uint8_t*>(cd.data());
        c[0] = 0x50; c[1] = 0x4b; c[2] = 0x01; c[3] = 0x02;
        c[4] = 20; c[5] = 0;
        c[6] = 20; c[7] = 0;
        c[8] = 0; c[9] = 0x08; // flags UTF-8
        c[10] = 0; c[11] = 0;
        c[12] = 0; c[13] = 0;
        c[14] = 0; c[15] = 0;
        c[16] = crc & 0xFF; c[17] = (crc >> 8) & 0xFF; c[18] = (crc >> 16) & 0xFF; c[19] = (crc >> 24) & 0xFF;
        c[20] = sz & 0xFF; c[21] = (sz >> 8) & 0xFF; c[22] = (sz >> 16) & 0xFF; c[23] = (sz >> 24) & 0xFF;
        c[24] = sz & 0xFF; c[25] = (sz >> 8) & 0xFF; c[26] = (sz >> 16) & 0xFF; c[27] = (sz >> 24) & 0xFF;
        c[28] = fnLen & 0xFF; c[29] = (fnLen >> 8) & 0xFF;
        c[30] = 0; c[31] = 0;
        c[32] = 0; c[33] = 0;
        c[34] = 0; c[35] = 0;
        c[36] = 0; c[37] = 0;
        c[38] = 0; c[39] = 0;
        c[40] = 0; c[41] = 0;
        c[42] = offset & 0xFF; c[43] = (offset >> 8) & 0xFF; c[44] = (offset >> 16) & 0xFF; c[45] = (offset >> 24) & 0xFF;
        centralDirectory.append(cd);
        centralDirectory.append(fnUtf8);
    }

    uint32_t cdOffset = static_cast<uint32_t>(file.pos());
    uint32_t cdSize = static_cast<uint32_t>(centralDirectory.size());
    file.write(centralDirectory);

    // End of central directory record (22 bytes)
    QByteArray eocd;
    eocd.resize(22);
    uint8_t* e = reinterpret_cast<uint8_t*>(eocd.data());
    e[0] = 0x50; e[1] = 0x4b; e[2] = 0x05; e[3] = 0x06;
    e[4] = 0; e[5] = 0;
    e[6] = 0; e[7] = 0;
    uint16_t total = static_cast<uint16_t>(entries.size());
    e[8] = total & 0xFF; e[9] = (total >> 8) & 0xFF;
    e[10] = total & 0xFF; e[11] = (total >> 8) & 0xFF;
    e[12] = cdSize & 0xFF; e[13] = (cdSize >> 8) & 0xFF; e[14] = (cdSize >> 16) & 0xFF; e[15] = (cdSize >> 24) & 0xFF;
    e[16] = cdOffset & 0xFF; e[17] = (cdOffset >> 8) & 0xFF; e[18] = (cdOffset >> 16) & 0xFF; e[19] = (cdOffset >> 24) & 0xFF;
    e[20] = 0; e[21] = 0;
    file.write(eocd);

    file.close();
    return true;
}

static QString xmlEsc(const QString& str) {
    QString res = str;
    res.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    res.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    res.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    res.replace(QLatin1Char('"'), QStringLiteral("&quot;"));
    return res;
}

bool ReportGenerator::generateDocxReport(const QString& targetFilePath,
                                        const StaticAnalysisReport& staticReport,
                                        const CoverageReport& covReport,
                                        const QString& sourceFile,
                                        const StimulusPlan& /* plan */)
{
    QString genTime = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    QFileInfo fi(sourceFile);
    QString fileName = fi.fileName().isEmpty() ? QStringLiteral("embedded_source.c") : fi.fileName();

    int maxCc = 1;
    for (const auto& f : staticReport.functions) {
        if (f.cyclomaticComplexity > maxCc) maxCc = f.cyclomaticComplexity;
    }

    // 1. [Content_Types].xml
    QByteArray contentTypes = QByteArrayLiteral(
R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>)");

    // 2. _rels/.rels
    QByteArray rels = QByteArrayLiteral(
R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>)");

    // 3. word/_rels/document.xml.rels
    QByteArray docRels = QByteArrayLiteral(
R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)");

    // 4. word/styles.xml
    QByteArray styles = QByteArrayLiteral(
R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults>
    <w:rPrDefault>
      <w:rPr>
        <w:rFonts w:ascii="Microsoft YaHei" w:hAnsi="Microsoft YaHei" w:eastAsia="Microsoft YaHei"/>
        <w:sz w:val="21"/>
        <w:color w:val="1E293B"/>
      </w:rPr>
    </w:rPrDefault>
  </w:docDefaults>
</w:styles>)");

    // 5. word/document.xml
    QString docXml;
    docXml.reserve(65536);
    docXml += QString::fromUtf8(
R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
<w:body>
  <!-- 标题与副标题 -->
  <w:p>
    <w:pPr><w:jc w:val="center"/><w:spacing w:before="240" w:after="120"/></w:pPr>
    <w:r><w:rPr><w:b/><w:sz w:val="44"/><w:color w:val="1E3A8A"/></w:rPr>
      <w:t>嵌入式 C 静态分析与覆盖度量工程报告</w:t>
    </w:r>
  </w:p>
  <w:p>
    <w:pPr><w:jc w:val="center"/><w:spacing w:after="240"/></w:pPr>
    <w:r><w:rPr><w:sz w:val="20"/><w:color w:val="64748B"/></w:rPr>
      <w:t>Embedded C Safety &amp; Coverage Verification Report</w:t>
    </w:r>
  </w:p>
  <w:p>
    <w:pPr><w:pBdr><w:bottom w:val="single" w:sz="12" w:space="1" w:color="2563EB"/></w:pBdr></w:pPr>
  </w:p>

  <!-- 工程元数据信息表 -->
  <w:tbl>
    <w:tblPr>
      <w:tblW w:w="5000" w:type="pct"/>
      <w:tblBorders>
        <w:top w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:bottom w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:insideH w:val="single" w:sz="4" w:space="0" w:color="E2E8F0"/>
        <w:insideV w:val="none"/>
      </w:tblBorders>
    </w:tblPr>
    <w:tr>
      <w:tc><w:tcPr><w:tcW w:w="1200" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="F8FAFC"/></w:tcPr><w:p><w:r><w:rPr><w:b/></w:rPr><w:t>分析源文件:</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="3800" w:type="pct"/></w:tcPr><w:p><w:r><w:t>)") + xmlEsc(fileName) + QString::fromUtf8(R"(</w:t></w:r></w:p></w:tc>
    </w:tr>
    <w:tr>
      <w:tc><w:tcPr><w:tcW w:w="1200" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="F8FAFC"/></w:tcPr><w:p><w:r><w:rPr><w:b/></w:rPr><w:t>生成时间:</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="3800" w:type="pct"/></w:tcPr><w:p><w:r><w:t>)") + xmlEsc(genTime) + QString::fromUtf8(R"(</w:t></w:r></w:p></w:tc>
    </w:tr>
    <w:tr>
      <w:tc><w:tcPr><w:tcW w:w="1200" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="F8FAFC"/></w:tcPr><w:p><w:r><w:rPr><w:b/></w:rPr><w:t>分析规则规范:</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="3800" w:type="pct"/></w:tcPr><w:p><w:r><w:t>MISRA C:2012 / 嵌入式硬件安全 / McCabe 圈复杂度</w:t></w:r></w:p></w:tc>
    </w:tr>
  </w:tbl>

  <w:p><w:pPr><w:spacing w:before="240" w:after="120"/></w:pPr>
    <w:r><w:rPr><w:b/><w:sz w:val="28"/><w:color w:val="0F172A"/></w:rPr>
      <w:t>一、核心质量与执行指标总览 (Executive Summary)</w:t>
    </w:r>
  </w:p>

  <!-- 4 个 KPI 卡片表格 -->
  docXml += QString(
R"(  <w:tbl>
    <w:tblPr>
      <w:tblW w:w="5000" w:type="pct"/>
      <w:tblBorders>
        <w:top w:val="single" w:sz="6" w:space="0" w:color="94A3B8"/>
        <w:left w:val="single" w:sz="6" w:space="0" w:color="94A3B8"/>
        <w:bottom w:val="single" w:sz="6" w:space="0" w:color="94A3B8"/>
        <w:right w:val="single" w:sz="6" w:space="0" w:color="94A3B8"/>
        <w:insideV w:val="single" w:sz="6" w:space="0" w:color="CBD5E1"/>
      </w:tblBorders>
    </w:tblPr>
    <w:tr>
      <w:tc>
        <w:tcPr><w:tcW w:w="1250" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="F0FDF4"/></w:tcPr>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:sz w:val="18"/><w:color w:val="166534"/></w:rPr><w:t>语句覆盖率 (Statement)</w:t></w:r></w:p>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:b/><w:sz w:val="36"/><w:color w:val="16A34A"/></w:rPr><w:t>%1%</w:t></w:r></w:p>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:sz w:val="16"/><w:color w:val="64748B"/></w:rPr><w:t>已覆盖 %2 / %3 行</w:t></w:r></w:p>
      </w:tc>
      <w:tc>
        <w:tcPr><w:tcW w:w="1250" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="EFF6FF"/></w:tcPr>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:sz w:val="18"/><w:color w:val="1E40AF"/></w:rPr><w:t>分支覆盖率 (Branch)</w:t></w:r></w:p>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:b/><w:sz w:val="36"/><w:color w:val="2563EB"/></w:rPr><w:t>%4%</w:t></w:r></w:p>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:sz w:val="16"/><w:color w:val="64748B"/></w:rPr><w:t>已命中 %5 / %6 分支</w:t></w:r></w:p>
      </w:tc>
      <w:tc>
        <w:tcPr><w:tcW w:w="1250" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="FEF2F2"/></w:tcPr>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:sz w:val="18"/><w:color w:val="991B1B"/></w:rPr><w:t>静态缺陷项 (Issues)</w:t></w:r></w:p>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:b/><w:sz w:val="36"/><w:color w:val="DC2626"/></w:rPr><w:t>%7</w:t></w:r></w:p>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:sz w:val="16"/><w:color w:val="64748B"/></w:rPr><w:t>严重 %8 | 警告 %9</w:t></w:r></w:p>
      </w:tc>
      <w:tc>
        <w:tcPr><w:tcW w:w="1250" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="FFFBEB"/></w:tcPr>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:sz w:val="18"/><w:color w:val="92400E"/></w:rPr><w:t>最高圈复杂度 (McCabe)</w:t></w:r></w:p>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:b/><w:sz w:val="36"/><w:color w:val="D97706"/></w:rPr><w:t>%10</w:t></w:r></w:p>
        <w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:sz w:val="16"/><w:color w:val="64748B"/></w:rPr><w:t>函数总数: %11 个</w:t></w:r></w:p>
      </w:tc>
    </w:tr>
  </w:tbl>)"
  ).arg(QString::number(covReport.statementCoveragePercent(), 'f', 1))
   .arg(covReport.coveredLines)
   .arg(covReport.executableLines)
   .arg(QString::number(covReport.branchCoveragePercent(), 'f', 1))
   .arg(covReport.coveredBranches)
   .arg(covReport.totalBranches)
   .arg(staticReport.issues.size())
   .arg(staticReport.errorCount())
   .arg(staticReport.warningCount())
   .arg(maxCc)
   .arg(staticReport.functions.size());

    docXml += QString::fromUtf8(
R"(  <w:p><w:pPr><w:spacing w:before="360" w:after="120"/></w:pPr>
    <w:r><w:rPr><w:b/><w:sz w:val="28"/><w:color w:val="0F172A"/></w:rPr>
      <w:t>二、MISRA C:2012 与嵌入式静态安全缺陷列表</w:t>
    </w:r>
  </w:p>)");

    docXml += QString::fromUtf8(
R"(  <!-- 静态缺陷详细表格 -->
  <w:tbl>
    <w:tblPr>
      <w:tblW w:w="5000" w:type="pct"/>
      <w:tblBorders>
        <w:top w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:left w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:bottom w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:right w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:insideH w:val="single" w:sz="4" w:space="0" w:color="E2E8F0"/>
        <w:insideV w:val="single" w:sz="4" w:space="0" w:color="E2E8F0"/>
      </w:tblBorders>
    </w:tblPr>
    <w:tr>
      <w:tc><w:tcPr><w:tcW w:w="600" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>级别</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="1200" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>规则编号</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="600" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>行号</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="1400" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>缺陷描述</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="1200" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>修复建议</w:t></w:r></w:p></w:tc>
    </w:tr>)");

    if (staticReport.issues.isEmpty()) {
        docXml += QString::fromUtf8(
R"(    <w:tr>
      <w:tc><w:tcPr><w:gridSpan w:val="5"/></w:tcPr><w:p><w:pPr><w:jc w:val="center"/></w:pPr><w:r><w:rPr><w:color w:val="16A34A"/></w:rPr><w:t>🎉 未检测到任何静态代码缺陷或规范违规，代码质量优良！</w:t></w:r></w:p></w:tc>
    </w:tr>)");
    } else {
        for (const auto& issue : staticReport.issues) {
            QString sevColor = (issue.severity == Severity::Critical || issue.severity == Severity::Error) ? QStringLiteral("DC2626") : QStringLiteral("D97706");
            QString sevText = severityToString(issue.severity);
            docXml += QString(
                "    <w:tr>\n"
                "      <w:tc><w:p><w:r><w:rPr><w:b/><w:color w:val=\"%1\"/></w:rPr><w:t>%2</w:t></w:r></w:p></w:tc>\n"
                "      <w:tc><w:p><w:r><w:rPr><w:b/></w:rPr><w:t>%3</w:t></w:r></w:p></w:tc>\n"
                "      <w:tc><w:p><w:r><w:t>L%4:%5</w:t></w:r></w:p></w:tc>\n"
                "      <w:tc><w:p><w:r><w:t>%6</w:t></w:r></w:p></w:tc>\n"
                "      <w:tc><w:p><w:r><w:rPr><w:color w:val=\"0369A1\"/></w:rPr><w:t>%7</w:t></w:r></w:p></w:tc>\n"
                "    </w:tr>\n")
                .arg(sevColor)
                .arg(xmlEsc(sevText))
                .arg(xmlEsc(issue.ruleId))
                .arg(issue.line)
                .arg(issue.column)
                .arg(xmlEsc(issue.localizedMessage(true)))
                .arg(xmlEsc(issue.localizedSuggestion(true)));
        }
    }
    docXml += QStringLiteral("  </w:tbl>");

    // 三、函数复杂度表格
    docXml += QStringLiteral(
R"(  <w:p><w:pPr><w:spacing w:before="360" w:after="120"/></w:pPr>
    <w:r><w:rPr><w:b/><w:sz w:val="28"/><w:color w:val="0F172A"/></w:rPr>
      <w:t>三、函数复杂度与代码结构度量清单</w:t>
    </w:r>
  </w:p>
  <w:tbl>
    <w:tblPr>
      <w:tblW w:w="5000" w:type="pct"/>
      <w:tblBorders>
        <w:top w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:left w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:bottom w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:right w:val="single" w:sz="4" w:space="0" w:color="CBD5E1"/>
        <w:insideH w:val="single" w:sz="4" w:space="0" w:color="E2E8F0"/>
        <w:insideV w:val="single" w:sz="4" w:space="0" w:color="E2E8F0"/>
      </w:tblBorders>
    </w:tblPr>
    <w:tr>
      <w:tc><w:tcPr><w:tcW w:w="1800" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>函数名称</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="800" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>行号范围</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="600" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>代码行(LOC)</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="800" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>圈复杂度(CC)</w:t></w:r></w:p></w:tc>
      <w:tc><w:tcPr><w:tcW w:w="1000" w:type="pct"/><w:shd w:val="clear" w:color="auto" w:fill="1E293B"/></w:tcPr><w:p><w:r><w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr><w:t>风险评估</w:t></w:r></w:p></w:tc>
    </w:tr>)");

    for (const auto& f : staticReport.functions) {
        QString riskText = QStringLiteral("结构清晰 (良好)");
        QString riskColor = QStringLiteral("16A34A");
        if (f.cyclomaticComplexity > 15) {
            riskText = QStringLiteral("极高风险 (强制重构)");
            riskColor = QStringLiteral("DC2626");
        } else if (f.cyclomaticComplexity > 10) {
            riskText = QStringLiteral("偏高警告 (建议优化)");
            riskColor = QStringLiteral("D97706");
        } else if (f.cyclomaticComplexity > 5) {
            riskText = QStringLiteral("中等复杂度");
            riskColor = QStringLiteral("2563EB");
        }

        docXml += QString(
            "    <w:tr>\n"
            "      <w:tc><w:p><w:r><w:rPr><w:b/></w:rPr><w:t>%1</w:t></w:r></w:p></w:tc>\n"
            "      <w:tc><w:p><w:r><w:t>L%2 ~ L%3</w:t></w:r></w:p></w:tc>\n"
            "      <w:tc><w:p><w:r><w:t>%4</w:t></w:r></w:p></w:tc>\n"
            "      <w:tc><w:p><w:r><w:rPr><w:b/></w:rPr><w:t>%5</w:t></w:r></w:p></w:tc>\n"
            "      <w:tc><w:p><w:r><w:rPr><w:b/><w:color w:val=\"%6\"/></w:rPr><w:t>%7</w:t></w:r></w:p></w:tc>\n"
            "    </w:tr>\n")
            .arg(xmlEsc(f.name))
            .arg(f.startLine)
            .arg(f.endLine)
            .arg(f.linesOfCode)
            .arg(f.cyclomaticComplexity)
            .arg(riskColor)
            .arg(xmlEsc(riskText));
    }
    docXml += QStringLiteral("  </w:tbl>");

    // 结尾页脚
    docXml += QStringLiteral(
R"(  <w:p><w:pPr><w:jc w:val="center"/><w:spacing w:before="480"/></w:pPr>
    <w:r><w:rPr><w:sz w:val="18"/><w:color w:val="94A3B8"/></w:rPr>
      <w:t>— 本报告由 嵌入式 C 覆盖分析与静态分析套件 自动生成 —</w:t>
    </w:r>
  </w:p>
</w:body>
</w:document>)");

    QVector<ZipFileEntry> entries;
    entries.append({QStringLiteral("[Content_Types].xml"), contentTypes});
    entries.append({QStringLiteral("_rels/.rels"), rels});
    entries.append({QStringLiteral("word/_rels/document.xml.rels"), docRels});
    entries.append({QStringLiteral("word/styles.xml"), styles});
    entries.append({QStringLiteral("word/document.xml"), docXml.toUtf8()});

    return writeZipArchive(targetFilePath, entries);
}

QString ReportGenerator::generatePrintableHtmlReport(const StaticAnalysisReport& staticReport,
                                                    const CoverageReport& covReport,
                                                    const QString& sourceFile,
                                                    const QString& /* sourceContent */,
                                                    const StimulusPlan& /* plan */)
{
    QString genTime = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    QFileInfo fi(sourceFile);
    QString fileName = fi.fileName().isEmpty() ? QStringLiteral("embedded_source.c") : fi.fileName();

    int maxCc = 1;
    for (const auto& f : staticReport.functions) {
        if (f.cyclomaticComplexity > maxCc) maxCc = f.cyclomaticComplexity;
    }

    QString html;
    html.reserve(32768);

    html += QStringLiteral(
R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
  body { font-family: "Microsoft YaHei", "Segoe UI", Arial, sans-serif; color: #1e293b; background: #ffffff; margin: 10px; font-size: 13px; line-height: 1.4; }
  h1 { color: #1e3a8a; font-size: 22px; margin: 0 0 4px 0; }
  .header-tbl { width: 100%; border: none; margin-bottom: 8px; }
  .header-tbl td { border: none; padding: 0; }
  .header-rule { border: 0; height: 3px; background-color: #2563eb; margin: 8px 0 16px 0; }
  h2 { color: #0f172a; font-size: 15px; margin-top: 20px; margin-bottom: 8px; border-bottom: 1px solid #cbd5e1; padding-bottom: 4px; }
  table { width: 100%; border-collapse: collapse; margin-bottom: 16px; font-size: 12px; }
  th, td { border: 1px solid #cbd5e1; padding: 6px 10px; text-align: left; }
  th { background-color: #1e293b; color: #ffffff; font-weight: bold; }
  .kpi-table { width: 100%; border: none; margin-bottom: 16px; }
  .kpi-table td { border: 1px solid #cbd5e1; padding: 10px; text-align: center; }
  .kpi-val { font-size: 20px; font-weight: bold; margin: 4px 0; }
  .kpi-lbl { font-size: 11px; color: #64748b; }
</style>
</head>
<body>
  <table class="header-tbl">
    <tr>
      <td style="width: 64px; vertical-align: middle; padding-right: 14px;">
        <svg width="56" height="56" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
          <rect x="2" y="2" width="44" height="44" rx="10" fill="#1e3a8a"/>
          <rect x="10" y="10" width="28" height="28" rx="6" fill="#0f172a" stroke="#3b82f6" stroke-width="2"/>
          <circle cx="24" cy="24" r="5" fill="#22c55e"/>
          <circle cx="24" cy="24" r="9" stroke="#22c55e" stroke-width="1.5" stroke-dasharray="2 2"/>
          <path d="M4 18H8M4 24H8M4 30H8M40 18H44M40 24H44M40 30H44" stroke="#fbbf24" stroke-width="2" stroke-linecap="round"/>
          <path d="M18 4V8M24 4V8M30 4V8M18 40V44M24 40V44M30 40V44" stroke="#fbbf24" stroke-width="2" stroke-linecap="round"/>
        </svg>
      </td>
      <td style="vertical-align: middle;">
        <h1 style="color: #1e3a8a; font-size: 24px; margin: 0 0 6px 0; font-weight: bold;">嵌入式 C 静态分析与覆盖度量工程报告</h1>
        <div class="subtitle" style="color: #475569; font-size: 12px; margin: 0;">
          Embedded C Safety &amp; Coverage Verification Report &nbsp;|&nbsp; <b>源文件:</b> )");
    html += xmlEsc(fileName);
    html += QStringLiteral(" &nbsp;|&nbsp; <b>生成时间:</b> ") + xmlEsc(genTime);
    html += QStringLiteral(R"(
        </div>
      </td>
    </tr>
  </table>
  <hr class="header-rule"/>)

  <h2>一、核心质量与执行指标总览 (Executive Summary)</h2>
  <table class="kpi-table">
    <tr>
      <td style="background-color: #f0fdf4; border-left: 4px solid #16a34a;">
        <div class="kpi-lbl" style="color: #166534;">语句覆盖率 (Statement)</div>
        <div class="kpi-val" style="color: #16a34a;">)");
    html += QString::number(covReport.statementCoveragePercent(), 'f', 1) + QStringLiteral("%</div>");
    html += QStringLiteral("<div class=\"kpi-lbl\">已覆盖: ") + QString::number(covReport.coveredLines) + QStringLiteral(" / 总 ") + QString::number(covReport.executableLines) + QStringLiteral(" 行</div></td>");

    html += QStringLiteral(R"(
      <td style="background-color: #eff6ff; border-left: 4px solid #2563eb;">
        <div class="kpi-lbl" style="color: #1e40af;">分支覆盖率 (Branch)</div>
        <div class="kpi-val" style="color: #2563eb;">)");
    html += QString::number(covReport.branchCoveragePercent(), 'f', 1) + QStringLiteral("%</div>");
    html += QStringLiteral("<div class=\"kpi-lbl\">已覆盖: ") + QString::number(covReport.coveredBranches) + QStringLiteral(" / 总 ") + QString::number(covReport.totalBranches) + QStringLiteral(" 分支</div></td>");

    html += QStringLiteral(R"(
      <td style="background-color: #fef2f2; border-left: 4px solid #dc2626;">
        <div class="kpi-lbl" style="color: #991b1b;">检测缺陷违规 (Issues)</div>
        <div class="kpi-val" style="color: #dc2626;">)");
    html += QString::number(staticReport.issues.size()) + QStringLiteral("</div>");
    html += QStringLiteral("<div class=\"kpi-lbl\">严重: ") + QString::number(staticReport.errorCount()) + QStringLiteral(" | 警告: ") + QString::number(staticReport.warningCount()) + QStringLiteral("</div></td>");

    html += QStringLiteral(R"(
      <td style="background-color: #fffbeb; border-left: 4px solid #d97706;">
        <div class="kpi-lbl" style="color: #92400e;">最高圈复杂度 (McCabe)</div>
        <div class="kpi-val" style="color: #d97706;">)");
    html += QString::number(maxCc) + QStringLiteral("</div>");
    html += QStringLiteral("<div class=\"kpi-lbl\">函数总数: ") + QString::number(staticReport.functions.size()) + QStringLiteral(" 个</div></td></tr></table>");

    // 缺陷列表
    html += QStringLiteral(R"(
  <h2>二、MISRA C:2012 与静态安全规范缺陷清单</h2>
  <table>
    <tr>
      <th style="width: 10%;">级别</th>
      <th style="width: 20%;">规则编号</th>
      <th style="width: 12%;">行号</th>
      <th style="width: 33%;">缺陷描述</th>
      <th style="width: 25%;">修复建议</th>
    </tr>)");

    if (staticReport.issues.isEmpty()) {
        html += QStringLiteral("<tr><td colspan=\"5\" style=\"text-align: center; color: #16a34a;\">🎉 未检出任何静态代码缺陷，代码符合安全编码规范！</td></tr>");
    } else {
        for (const auto& issue : staticReport.issues) {
            QString sevColor = (issue.severity == Severity::Critical || issue.severity == Severity::Error) ? QStringLiteral("#fee2e2; color: #991b1b") : QStringLiteral("#fef3c7; color: #92400e");
            html += QString("<tr><td style=\"background-color: %1; font-weight: bold;\">%2</td>"
                            "<td><b>%3</b></td>"
                            "<td>第 %4 行</td>"
                            "<td>%5</td>"
                            "<td style=\"color: #0369a1;\">%6</td></tr>")
                        .arg(sevColor)
                        .arg(xmlEsc(severityToString(issue.severity)))
                        .arg(xmlEsc(issue.ruleId))
                        .arg(issue.line)
                        .arg(xmlEsc(issue.localizedMessage(true)))
                        .arg(xmlEsc(issue.localizedSuggestion(true)));
        }
    }
    html += QStringLiteral("</table>");

    // 函数复杂度列表
    html += QStringLiteral(R"(
  <h2>三、函数复杂度与结构度量一览</h2>
  <table>
    <tr>
      <th style="width: 28%;">函数名称</th>
      <th style="width: 16%;">行号范围</th>
      <th style="width: 16%;">代码行(LOC)</th>
      <th style="width: 16%;">圈复杂度(CC)</th>
      <th style="width: 24%;">风险评估</th>
    </tr>)");

    for (const auto& f : staticReport.functions) {
        QString risk = QStringLiteral("<span style=\"color: #16a34a; font-weight: bold;\">良好 (≤5)</span>");
        if (f.cyclomaticComplexity > 15) {
            risk = QStringLiteral("<span style=\"color: #dc2626; font-weight: bold;\">极高风险 (&gt;15)</span>");
        } else if (f.cyclomaticComplexity > 10) {
            risk = QStringLiteral("<span style=\"color: #d97706; font-weight: bold;\">偏高警告 (11-15)</span>");
        } else if (f.cyclomaticComplexity > 5) {
            risk = QStringLiteral("<span style=\"color: #2563eb; font-weight: bold;\">中等 (6-10)</span>");
        }

        html += QString("<tr><td><b>%1()</b></td><td>L%2 ~ L%3</td><td>%4</td><td><b>%5</b></td><td>%6</td></tr>")
                    .arg(xmlEsc(f.name))
                    .arg(f.startLine)
                    .arg(f.endLine)
                    .arg(f.linesOfCode)
                    .arg(f.cyclomaticComplexity)
                    .arg(risk);
    }
    html += QStringLiteral("</table>");

    html += QStringLiteral(R"(
  <div style="text-align: center; color: #94a3b8; font-size: 11px; margin-top: 30px;">
    — 本报告由 嵌入式 C 覆盖分析与静态分析套件 自动生成 —
  </div>
</body>
</html>)");

    return html;
}

QString ReportGenerator::generateJsonReport(const StaticAnalysisReport& staticReport,
                                            const CoverageReport& covReport)
{
    QJsonObject root;
    root["generator"] = "Embedded C Coverage & Static Analysis Suite";
    root["version"] = "1.2.0";
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Static report
    QJsonObject staticObj;
    staticObj["total_lines"] = staticReport.totalLines;
    staticObj["code_lines"] = staticReport.codeLines;
    staticObj["comment_lines"] = staticReport.commentLines;
    staticObj["error_count"] = staticReport.errorCount();
    staticObj["warning_count"] = staticReport.warningCount();

    QJsonArray issuesArr;
    for (const auto& iss : staticReport.issues) {
        QJsonObject io;
        io["rule_id"] = iss.ruleId;
        io["severity"] = severityToString(iss.severity);
        io["line"] = iss.line;
        io["column"] = iss.column;
        io["message"] = iss.message;
        io["suggestion"] = iss.suggestion;
        issuesArr.append(io);
    }
    staticObj["issues"] = issuesArr;

    QJsonArray funcArr;
    for (const auto& f : staticReport.functions) {
        QJsonObject fo;
        fo["name"] = f.name;
        fo["start_line"] = f.startLine;
        fo["end_line"] = f.endLine;
        fo["loc"] = f.linesOfCode;
        fo["cyclomatic_complexity"] = f.cyclomaticComplexity;
        fo["max_nesting"] = f.maxNestingDepth;
        funcArr.append(fo);
    }
    staticObj["functions"] = funcArr;
    root["static_analysis"] = staticObj;

    // Coverage report
    QJsonObject covObj;
    covObj["statement_coverage_percent"] = covReport.statementCoveragePercent();
    covObj["branch_coverage_percent"] = covReport.branchCoveragePercent();
    covObj["function_coverage_percent"] = covReport.functionCoveragePercent();
    covObj["covered_lines"] = covReport.coveredLines;
    covObj["total_executable_lines"] = covReport.executableLines;
    covObj["covered_branches"] = covReport.coveredBranches;
    covObj["total_branches"] = covReport.totalBranches;
    root["coverage"] = covObj;

    QJsonDocument doc(root);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

} // namespace Coverage
