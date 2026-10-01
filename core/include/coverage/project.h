#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

namespace Coverage {

/**
 * @brief 嵌入式分析工程数据模型 (CoverageProject)
 * 代表一个完整的 .covproj 分析工程，包含目标架构、源码清单、激励脚本、编译器配置及最后修改时间。
 */
struct CoverageProject {
    QString name = QStringLiteral("Untitled_Project");
    QString filePath;             ///< .covproj 绝对路径
    QString rootDir;              ///< 工程根路径
    QString description;          ///< 描述说明
    QString targetArch = QStringLiteral("ARM Cortex-M4 (STM32)"); ///< 目标 MCU 硬件架构
    QString outputDir = QStringLiteral("reports");

    QStringList sourceFiles;      ///< 嵌入式 C/C++ 源文件路径 (*.c, *.h, *.cpp)
    QStringList scriptFiles;      ///< Python 激励注入脚本路径 (*.py)
    QString activeFile;           ///< 当前活动的源文件

    QDateTime createdTime;
    QDateTime lastModifiedTime;

    bool isValid() const { return !name.isEmpty() && !filePath.isEmpty(); }

    QJsonObject toJson() const;
    static CoverageProject fromJson(const QJsonObject& obj, const QString& filePath);

    bool saveToFile(const QString& path = QString());
    static CoverageProject loadFromFile(const QString& path, bool* ok = nullptr);
};

/**
 * @brief 工程管理器单例 (ProjectManager)
 * 负责工程的创建、打开、保存、关闭，以及最近工程列表 (Recent Projects) 的持久化管理。
 */
class ProjectManager {
public:
    static ProjectManager& instance();

    bool hasActiveProject() const { return m_currentProject.isValid(); }
    CoverageProject& currentProject() { return m_currentProject; }
    const CoverageProject& currentProject() const { return m_currentProject; }

    bool createNewProject(const QString& name, const QString& projectDir,
                          const QString& targetArch = QStringLiteral("ARM Cortex-M4 (STM32)"),
                          const QStringList& initialSources = {},
                          const QStringList& initialScripts = {});

    bool openProject(const QString& covprojPath);
    bool saveCurrentProject();
    void closeCurrentProject();

    // 示例工程快速加载
    bool openBuiltinSample(const QString& sampleId); // "motor_controller" or "sensor_fusion"

    // 最近打开工程列表
    QStringList recentProjects() const;
    void addRecentProject(const QString& path);
    void removeRecentProject(const QString& path);
    void clearRecentProjects();

private:
    ProjectManager();
    void loadRecentProjects();
    void saveRecentProjects();

    CoverageProject m_currentProject;
    QStringList m_recentProjects;
    QString m_recentProjectsFile;
};

} // namespace Coverage
