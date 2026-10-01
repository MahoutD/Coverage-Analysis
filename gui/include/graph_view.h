#pragma once

#include "coverage/models.h"
#include "graph_canvas_view.h"
#include "theme_manager.h"
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>

namespace Coverage {

/**
 * @brief Graphviz 代码结构拓扑与控制流图可视化视图
 * 采用 Qt 原生画布 (QGraphicsScene/QGraphicsView) 与自定义图元 (NodeGraphicsItem/EdgeGraphicsItem)
 * 进行无失真矢量渲染，支持两级图谱展示：
 * 1. 全局函数调用关系图 (Call Graph)
 * 2. 单个函数内部控制流图 (Control Flow Graph - CFG) 节点与分支边
 * 支持节点鼠标选中、双击、单击直接跳转到源码对应行。
 */
class GraphView : public QWidget {
    Q_OBJECT
public:
    explicit GraphView(QWidget* parent = nullptr);

    /**
     * @brief 刷新视图所引用的静态分析报告与源码数据
     */
    void updateData(const StaticAnalysisReport& report, const QString& sourceCode);

    void clear();

    void applyTheme(ThemeType type = ThemeType::DarkCatppuccin);

signals:
    void functionSelected(const QString& funcName);
    void nodeSelected(const QString& name, int startLine, int endLine);

private slots:
    void onModeChanged(int index);
    void onFunctionChanged(int index);
    void onGenerateGraph();
    void onExportDot();
    void onExportImage();

private:
    void setupUi();
    void renderCurrentDot();

    StaticAnalysisReport m_report;
    QString m_sourceCode;
    QString m_currentDot;

    // 控件成员
    QComboBox* m_cmbGraphMode;
    QComboBox* m_cmbFunctionSelect;
    QPushButton* m_btnRefresh;
    QPushButton* m_btnExportDot;
    QPushButton* m_btnExportImage;
    QPushButton* m_btnZoomIn;
    QPushButton* m_btnZoomOut;
    QPushButton* m_btnFit;
    QPushButton* m_btnReset;

    GraphCanvasView* m_canvasView = nullptr;
    QLabel* m_lblStatus = nullptr;
    QTextEdit* m_txtDotSource = nullptr;
    QTabWidget* m_tabs = nullptr;
    ThemeType m_currentTheme = ThemeType::DarkCatppuccin;
};

} // namespace Coverage
