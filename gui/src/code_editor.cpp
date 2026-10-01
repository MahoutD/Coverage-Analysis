#include "code_editor.h"
#include "theme_manager.h"
#include <QPainter>
#include <QTextBlock>
#include <QToolTip>

namespace Coverage {

GutterArea::GutterArea(CodeEditor* editor)
    : QWidget(editor)
    , m_editor(editor)
{
}

QSize GutterArea::sizeHint() const {
    return QSize(m_editor->gutterAreaWidth(), 0);
}

void GutterArea::paintEvent(QPaintEvent* event) {
    m_editor->gutterAreaPaintEvent(event);
}

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    m_gutterArea = new GutterArea(this);
    m_highlighter = new CSyntaxHighlighter(document());

    setStyleSheet(ThemeManager::getEditorStyleSheet());

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateGutterAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateGutterArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateGutterAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::gutterAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        digits++;
    }
    // Gutter width: line number digits + clean margins (no indicator dots)
    int space = 16 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateGutterAreaWidth(int /* newBlockCount */) {
    setViewportMargins(gutterAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateGutterArea(const QRect& rect, int dy) {
    if (dy) {
        m_gutterArea->scroll(0, dy);
    } else {
        m_gutterArea->update(0, rect.y(), m_gutterArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateGutterAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_gutterArea->setGeometry(QRect(cr.left(), cr.top(), gutterAreaWidth(), cr.height()));
}

void CodeEditor::applyCurrentTheme() {
    setStyleSheet(ThemeManager::getEditorStyleSheet());
    m_gutterArea->update();
    highlightCurrentLine();
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    // 1. Path highlighted lines (if any)
    if (!m_pathHighlightedLines.isEmpty()) {
        ThemeType theme = ThemeManager::currentTheme();
        QColor pathBg = (theme == ThemeType::LightModern) ?
                        QColor(16, 185, 129, 65) :
                        QColor(16, 185, 129, 45); // Emerald translucent tint

        for (int lineNo : m_pathHighlightedLines) {
            QTextBlock block = document()->findBlockByLineNumber(lineNo - 1);
            if (block.isValid()) {
                QTextEdit::ExtraSelection sel;
                sel.format.setBackground(pathBg);
                sel.format.setForeground((theme == ThemeType::LightModern) ?
                                         QColor(QStringLiteral("#047857")) :
                                         QColor(QStringLiteral("#10b981"))); // 选中的路径把代码颜色变为亮绿色
                sel.format.setFontWeight(QFont::Bold);
                sel.format.setProperty(QTextFormat::FullWidthSelection, true);
                sel.cursor = QTextCursor(block);
                sel.cursor.select(QTextCursor::LineUnderCursor);
                extraSelections.append(sel);
            }
        }
    }

    // 2. Current active line cursor highlight
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        ThemeType theme = ThemeManager::currentTheme();
        QColor lineColor = (theme == ThemeType::LightModern) ? QColor(QStringLiteral("#e8f0fe")) :
                           (theme == ThemeType::SolarizedDark) ? QColor(QStringLiteral("#073642")) :
                           (theme == ThemeType::MonokaiPro) ? QColor(QStringLiteral("#403e41")) :
                           QColor(QStringLiteral("#1e1e2e")).lighter(130);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // 3. Static analysis issue underlines
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    for (auto it = m_issuesByLine.constBegin(); it != m_issuesByLine.constEnd(); ++it) {
        int lineNo = it.key();
        QTextBlock block = document()->findBlockByLineNumber(lineNo - 1);
        if (block.isValid()) {
            QTextEdit::ExtraSelection issueSel;
            issueSel.cursor = QTextCursor(block);
            issueSel.cursor.select(QTextCursor::LineUnderCursor);

            bool hasError = false;
            for (const auto& iss : it.value()) {
                if (iss.severity == Severity::Error || iss.severity == Severity::Critical) {
                    hasError = true;
                    break;
                }
            }

            QColor underlineColor = hasError ?
                (isLight ? QColor(QStringLiteral("#dc2626")) : QColor(QStringLiteral("#f38ba8"))) :
                (isLight ? QColor(QStringLiteral("#d97706")) : QColor(QStringLiteral("#f9e2af")));
            issueSel.format.setUnderlineColor(underlineColor);
            issueSel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            extraSelections.append(issueSel);
        }
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::highlightPathLines(const QVector<int>& lines) {
    m_pathHighlightedLines = lines;
    highlightCurrentLine();
}

void CodeEditor::clearPathHighlights() {
    m_pathHighlightedLines.clear();
    highlightCurrentLine();
}

void CodeEditor::setCoverageReport(const CoverageReport& report) {
    m_coverageReport = report;
    m_gutterArea->update();
}

void CodeEditor::setStaticIssues(const QVector<StaticIssue>& issues) {
    m_issuesByLine.clear();
    for (const auto& issue : issues) {
        m_issuesByLine[issue.line].append(issue);
    }
    highlightCurrentLine();
    m_gutterArea->update();
}

void CodeEditor::gotoLine(int line, int column) {
    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        int pos = block.position() + qMax(0, column - 1);
        cursor.setPosition(pos);
        setTextCursor(cursor);
        ensureCursorVisible();
        setFocus();
    }
}

void CodeEditor::gutterAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_gutterArea);
    
    QColor gutterBg = QColor(QStringLiteral("#181825"));
    QColor numColor = QColor(QStringLiteral("#6c7086"));
    ThemeType theme = ThemeManager::currentTheme();
    if (theme == ThemeType::LightModern) {
        gutterBg = QColor(QStringLiteral("#f1f3f4"));
        numColor = QColor(QStringLiteral("#70757a"));
    } else if (theme == ThemeType::SolarizedDark) {
        gutterBg = QColor(QStringLiteral("#073642"));
        numColor = QColor(QStringLiteral("#586e75"));
    } else if (theme == ThemeType::MonokaiPro) {
        gutterBg = QColor(QStringLiteral("#221f22"));
        numColor = QColor(QStringLiteral("#727072"));
    }
    painter.fillRect(event->rect(), gutterBg);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNo = blockNumber + 1;

            // Draw Clean Line Number
            QString number = QString::number(lineNo);
            painter.setPen(numColor);
            painter.drawText(0, top, m_gutterArea->width() - 6, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

} // namespace Coverage
