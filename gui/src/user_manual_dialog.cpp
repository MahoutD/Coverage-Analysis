#include "user_manual_dialog.h"
#include "theme_manager.h"
#include "localization_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QRegularExpression>
#include <QMessageBox>
#include <QHeaderView>

namespace Coverage {

UserManualDialog::UserManualDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("软件使用说明文档 (User Manual) - 实时同步与查阅"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.svg")));
    // 启用完整的窗口控制按钮：最大化、最小化、关闭与自由拉伸
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    resize(1100, 750);

    setupUi();
    reloadManual();
}

void UserManualDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // 1. 顶部控制栏: 搜索框、文档路径显示、重新加载按钮
    auto* topBar = new QHBoxLayout();
    bool isLight = (ThemeManager::currentTheme() == ThemeType::LightModern);
    
    auto* lblSearch = new QLabel(QStringLiteral("🔍 关键字搜索:"), this);
    lblSearch->setStyleSheet(isLight ?
        QStringLiteral("font-weight: bold; color: #1a73e8;") :
        QStringLiteral("font-weight: bold; color: #89b4fa;"));
    topBar->addWidget(lblSearch);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("输入关键词在说明书中查找..."));
    m_searchEdit->setClearButtonEnabled(true);
    topBar->addWidget(m_searchEdit, 1);

    m_lblDocPath = new QLabel(this);
    m_lblDocPath->setStyleSheet(isLight ?
        QStringLiteral("color: #5f6368; font-size: 11px;") :
        QStringLiteral("color: #a6adc8; font-size: 11px;"));
    topBar->addWidget(m_lblDocPath);

    mainLayout->addLayout(topBar);

    // 2. 中央内容分割区: 左侧目录树，右侧 Markdown 富文本浏览器
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧目录树
    auto* leftContainer = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    auto* lblToc = new QLabel(QStringLiteral("📑 章节目录导航"), leftContainer);
    lblToc->setStyleSheet(QStringLiteral("font-weight: bold; padding: 4px;"));
    leftLayout->addWidget(lblToc);

    m_tocTree = new QTreeWidget(leftContainer);
    m_tocTree->setHeaderHidden(true);
    m_tocTree->setRootIsDecorated(true);
    leftLayout->addWidget(m_tocTree);
    splitter->addWidget(leftContainer);

    // 右侧内容浏览
    m_contentBrowser = new QTextBrowser(this);
    m_contentBrowser->setOpenExternalLinks(true);
    m_contentBrowser->setStyleSheet(QStringLiteral(
        "QTextBrowser {"
        "  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "  font-size: 13px;"
        "  line-height: 1.6;"
        "  padding: 12px;"
        "}"
    ));
    splitter->addWidget(m_contentBrowser);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter, 1);

    // 3. 底部状态与关闭
    auto* bottomBar = new QHBoxLayout();
    auto* lblSyncNote = new QLabel(QStringLiteral("💡 说明：本使用说明书与 docs/user_manual.md 强关联，软件功能有变动时更新该文件即可在此处实时同步。"), this);
    lblSyncNote->setStyleSheet(isLight ?
        QStringLiteral("color: #5f6368; font-style: italic;") :
        QStringLiteral("color: #a6adc8; font-style: italic;"));
    bottomBar->addWidget(lblSyncNote, 1);

    auto* btnClose = new QPushButton(QStringLiteral("关闭"), this);
    btnClose->setFixedWidth(90);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    bottomBar->addWidget(btnClose);

    mainLayout->addLayout(bottomBar);

    // 信号连接
    connect(m_searchEdit, &QLineEdit::textChanged, this, &UserManualDialog::searchInManual);
    connect(m_tocTree, &QTreeWidget::itemClicked, this, &UserManualDialog::onTocItemClicked);
}

QString UserManualDialog::findManualFilePath() const {
    const QStringList candidates = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/docs/user_manual.md"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../docs/user_manual.md"),
        QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis/docs/user_manual.md"),
        QDir::currentPath() + QStringLiteral("/docs/user_manual.md")
    };

    for (const QString& path : candidates) {
        if (QFile::exists(path)) {
            return QDir::toNativeSeparators(path);
        }
    }
    return QString();
}

void UserManualDialog::reloadManual() {
    QString docPath = findManualFilePath();
    m_loadedPath = docPath;

    QString markdown;
    if (!docPath.isEmpty() && QFile::exists(docPath)) {
        QFile file(docPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            markdown = in.readAll();
            file.close();
            QFileInfo fi(docPath);
            m_lblDocPath->setText(QStringLiteral("来源: %1 (更新于 %2)")
                                      .arg(fi.fileName(), fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))));
        }
    }

    if (markdown.isEmpty()) {
        m_lblDocPath->setText(QStringLiteral("使用内置备用文档"));
        markdown = QStringLiteral(
            "# 嵌入式 C 覆盖分析与静态分析软件 使用说明书\n\n"
            "**提示**：未能在本地找到 `docs/user_manual.md` 文件，正在展示内置备用说明。\n\n"
            "## 1. 软件概述\n"
            "本系统提供嵌入式 C 静态代码分析（MISRA-C 检测）、Graphviz 结构拓扑分析、覆盖率源码插桩与 Python 激励生成。\n"
        );
    }

    m_rawMarkdown = markdown;
    m_contentBrowser->setMarkdown(markdown);
    parseTocAndContent(markdown);
}

void UserManualDialog::parseTocAndContent(const QString& markdown) {
    m_tocTree->clear();
    QStringList lines = markdown.split(QStringLiteral("\n"));

    QTreeWidgetItem* currentH1Item = nullptr;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("# "))) {
            QString title = trimmed.mid(2).trimmed();
            currentH1Item = new QTreeWidgetItem(m_tocTree, {title});
            currentH1Item->setExpanded(true);
        } else if (trimmed.startsWith(QStringLiteral("## "))) {
            QString title = trimmed.mid(3).trimmed();
            if (currentH1Item) {
                new QTreeWidgetItem(currentH1Item, {title});
            } else {
                new QTreeWidgetItem(m_tocTree, {title});
            }
        } else if (trimmed.startsWith(QStringLiteral("### "))) {
            QString title = trimmed.mid(4).trimmed();
            if (currentH1Item && currentH1Item->childCount() > 0) {
                auto* lastH2 = currentH1Item->child(currentH1Item->childCount() - 1);
                new QTreeWidgetItem(lastH2, {title});
            }
        }
    }
}

void UserManualDialog::onTocItemClicked(QTreeWidgetItem* item, int /* column */) {
    if (!item) return;
    QString targetText = item->text(0);
    // 在正文中寻找该标题并定位跳转
    m_contentBrowser->find(targetText, QTextDocument::FindCaseSensitively);
}

void UserManualDialog::searchInManual(const QString& query) {
    if (query.trimmed().isEmpty()) {
        // 重置光标到文档开头
        QTextCursor cursor = m_contentBrowser->textCursor();
        cursor.movePosition(QTextCursor::Start);
        m_contentBrowser->setTextCursor(cursor);
        return;
    }

    m_contentBrowser->find(query);
}

} // namespace Coverage
