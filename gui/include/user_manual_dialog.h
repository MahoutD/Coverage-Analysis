#pragma once

#include <QDialog>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

namespace Coverage {

/**
 * @brief 软件使用说明文档浏览器对话框
 * 支持直接热加载本地 docs/user_manual.md 文档，提供章节导航树、关键字快速检索、
 * 格式化 Markdown 渲染，并在外部文档修改后支持一键刷新同步。
 */
class UserManualDialog : public QDialog {
    Q_OBJECT
public:
    explicit UserManualDialog(QWidget* parent = nullptr);
    ~UserManualDialog() override = default;

public slots:
    void reloadManual();
    void searchInManual(const QString& query);

private slots:
    void onTocItemClicked(QTreeWidgetItem* item, int column);

private:
    void setupUi();
    QString findManualFilePath() const;
    void parseTocAndContent(const QString& markdown);
    QString renderMarkdownToHtml(const QString& markdown);

    QTreeWidget* m_tocTree;
    QTextBrowser* m_contentBrowser;
    QLineEdit* m_searchEdit;
    QLabel* m_lblDocPath;

    QString m_loadedPath;
    QString m_rawMarkdown;
};

} // namespace Coverage
