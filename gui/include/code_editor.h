#pragma once

#include "coverage/models.h"
#include "syntax_highlighter.h"
#include <QPlainTextEdit>
#include <QWidget>
#include <QMap>
#include <QVector>

namespace Coverage {

class CodeEditor;

class GutterArea : public QWidget {
public:
    explicit GutterArea(CodeEditor* editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    CodeEditor* m_editor;
};

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent = nullptr);

    void gutterAreaPaintEvent(QPaintEvent* event);
    int gutterAreaWidth() const;

    void setCoverageReport(const CoverageReport& report);
    void setStaticIssues(const QVector<StaticIssue>& issues);
    void highlightPathLines(const QVector<int>& lines);
    void clearPathHighlights();
    void gotoLine(int line, int column = 1);
    void applyCurrentTheme();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateGutterAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateGutterArea(const QRect& rect, int dy);

private:
    QWidget* m_gutterArea;
    CSyntaxHighlighter* m_highlighter;
    CoverageReport m_coverageReport;
    QMap<int, QVector<StaticIssue>> m_issuesByLine;
    QVector<int> m_pathHighlightedLines;

    void applyIssueUnderlines();
};

} // namespace Coverage
