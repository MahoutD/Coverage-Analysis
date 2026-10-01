#include "syntax_highlighter.h"

namespace Coverage {

CSyntaxHighlighter::CSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    // Keyword format (mauve / purple)
    m_keywordFormat.setForeground(QColor(QStringLiteral("#cba6f7")));
    m_keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        QStringLiteral(R"(\bbreak\b)"), QStringLiteral(R"(\bcase\b)"), QStringLiteral(R"(\bcontinue\b)"),
        QStringLiteral(R"(\bdefault\b)"), QStringLiteral(R"(\bdo\b)"), QStringLiteral(R"(\belse\b)"),
        QStringLiteral(R"(\bfor\b)"), QStringLiteral(R"(\bgoto\b)"), QStringLiteral(R"(\bif\b)"),
        QStringLiteral(R"(\breturn\b)"), QStringLiteral(R"(\bswitch\b)"), QStringLiteral(R"(\bwhile\b)"),
        QStringLiteral(R"(\bsizeof\b)"), QStringLiteral(R"(\btypedef\b)"), QStringLiteral(R"(\bstruct\b)"),
        QStringLiteral(R"(\bunion\b)"), QStringLiteral(R"(\benum\b)")
    };
    for (const QString& pattern : keywordPatterns) {
        m_highlightingRules.append({QRegularExpression(pattern), m_keywordFormat});
    }

    // Type format (yellow / peach)
    m_typeFormat.setForeground(QColor(QStringLiteral("#fab387")));
    m_typeFormat.setFontWeight(QFont::DemiBold);
    const QString typePatterns[] = {
        QStringLiteral(R"(\bint\b)"), QStringLiteral(R"(\bchar\b)"), QStringLiteral(R"(\bshort\b)"),
        QStringLiteral(R"(\blong\b)"), QStringLiteral(R"(\bfloat\b)"), QStringLiteral(R"(\bdouble\b)"),
        QStringLiteral(R"(\bvoid\b)"), QStringLiteral(R"(\bsigned\b)"), QStringLiteral(R"(\bunsigned\b)"),
        QStringLiteral(R"(\bconst\b)"), QStringLiteral(R"(\bstatic\b)"), QStringLiteral(R"(\bvolatile\b)"),
        QStringLiteral(R"(\bextern\b)"), QStringLiteral(R"(\binline\b)"), QStringLiteral(R"(\buint8_t\b)"),
        QStringLiteral(R"(\buint16_t\b)"), QStringLiteral(R"(\buint32_t\b)"), QStringLiteral(R"(\buint64_t\b)"),
        QStringLiteral(R"(\bint8_t\b)"), QStringLiteral(R"(\bint16_t\b)"), QStringLiteral(R"(\bint32_t\b)"),
        QStringLiteral(R"(\bbool\b)"), QStringLiteral(R"(\btrue\b)"), QStringLiteral(R"(\bfalse\b)"),
        QStringLiteral(R"(\bNULL\b)")
    };
    for (const QString& pattern : typePatterns) {
        m_highlightingRules.append({QRegularExpression(pattern), m_typeFormat});
    }

    // Preprocessor format (pink)
    m_preprocessorFormat.setForeground(QColor(QStringLiteral("#f5c2e7")));
    m_highlightingRules.append({QRegularExpression(QStringLiteral(R"(^\s*#\s*[a-zA-Z_]+)")), m_preprocessorFormat});

    // Probe macros format (teal / sky)
    m_probeFormat.setForeground(QColor(QStringLiteral("#89dceb")));
    m_probeFormat.setFontWeight(QFont::Bold);
    m_highlightingRules.append({QRegularExpression(QStringLiteral(R"(\b_cov_hit_[a-zA-Z0-9_]+\b)")), m_probeFormat});

    // String format (green)
    m_stringFormat.setForeground(QColor(QStringLiteral("#a6e3a1")));
    m_highlightingRules.append({QRegularExpression(QStringLiteral(R"("[^"\\]*(\\.[^"\\]*)*")")), m_stringFormat});
    m_highlightingRules.append({QRegularExpression(QStringLiteral(R"('[^'\\]*(\\.[^'\\]*)*')")), m_stringFormat});

    // Number format (sapphire)
    m_numberFormat.setForeground(QColor(QStringLiteral("#74c7ec")));
    m_highlightingRules.append({QRegularExpression(QStringLiteral(R"(\b0x[0-9a-fA-F]+\b|\b\d+(\.\d+)?[fF]?\b)")), m_numberFormat});

    // Single-line comment (subtext)
    m_commentFormat.setForeground(QColor(QStringLiteral("#6c7086")));
    m_commentFormat.setFontItalic(true);
    m_highlightingRules.append({QRegularExpression(QStringLiteral(R"(//[^\n]*)")), m_commentFormat});

    // Multi-line comment patterns
    m_commentStartPattern = QRegularExpression(QStringLiteral(R"(/\*)"));
    m_commentEndPattern = QRegularExpression(QStringLiteral(R"(\*/)"));
}

void CSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : m_highlightingRules) {
        auto matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            auto match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(m_commentStartPattern);
    }

    while (startIndex >= 0) {
        auto match = m_commentEndPattern.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, m_commentFormat);
        startIndex = text.indexOf(m_commentStartPattern, startIndex + commentLength);
    }
}

} // namespace Coverage
