#include "coverage/project.h"
#include "coverage/app_config.h"
#include "coverage/logger.h"

#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

namespace Coverage {

QJsonObject CoverageProject::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("description")] = description;
    obj[QStringLiteral("targetArch")] = targetArch;
    obj[QStringLiteral("outputDir")] = outputDir;
    obj[QStringLiteral("activeFile")] = activeFile;
    obj[QStringLiteral("createdTime")] = createdTime.toString(Qt::ISODate);
    obj[QStringLiteral("lastModifiedTime")] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray srcArr;
    QDir projectDir(rootDir);
    for (const QString& src : sourceFiles) {
        // 保存相对路径以便工程便携移动
        srcArr.append(projectDir.relativeFilePath(src));
    }
    obj[QStringLiteral("sourceFiles")] = srcArr;

    QJsonArray scriptArr;
    for (const QString& sc : scriptFiles) {
        scriptArr.append(projectDir.relativeFilePath(sc));
    }
    obj[QStringLiteral("scriptFiles")] = scriptArr;

    return obj;
}

CoverageProject CoverageProject::fromJson(const QJsonObject& obj, const QString& filePath) {
    CoverageProject proj;
    proj.filePath = QDir::toNativeSeparators(filePath);
    proj.rootDir = QDir::toNativeSeparators(QFileInfo(filePath).absolutePath());
    proj.name = obj.value(QStringLiteral("name")).toString(QFileInfo(filePath).baseName());
    proj.description = obj.value(QStringLiteral("description")).toString();
    proj.targetArch = obj.value(QStringLiteral("targetArch")).toString(QStringLiteral("ARM Cortex-M4 (STM32)"));
    proj.outputDir = obj.value(QStringLiteral("outputDir")).toString(QStringLiteral("reports"));
    proj.activeFile = obj.value(QStringLiteral("activeFile")).toString();

    proj.createdTime = QDateTime::fromString(obj.value(QStringLiteral("createdTime")).toString(), Qt::ISODate);
    if (!proj.createdTime.isValid()) proj.createdTime = QDateTime::currentDateTime();
    proj.lastModifiedTime = QDateTime::fromString(obj.value(QStringLiteral("lastModifiedTime")).toString(), Qt::ISODate);
    if (!proj.lastModifiedTime.isValid()) proj.lastModifiedTime = QDateTime::currentDateTime();

    QDir projectDir(proj.rootDir);

    QJsonArray srcArr = obj.value(QStringLiteral("sourceFiles")).toArray();
    for (auto val : srcArr) {
        QString rel = val.toString();
        QString full = projectDir.absoluteFilePath(rel);
        proj.sourceFiles.append(QDir::toNativeSeparators(full));
    }

    QJsonArray scriptArr = obj.value(QStringLiteral("scriptFiles")).toArray();
    for (auto val : scriptArr) {
        QString rel = val.toString();
        QString full = projectDir.absoluteFilePath(rel);
        proj.scriptFiles.append(QDir::toNativeSeparators(full));
    }

    if (!proj.activeFile.isEmpty() && !QFileInfo(proj.activeFile).isAbsolute()) {
        proj.activeFile = QDir::toNativeSeparators(projectDir.absoluteFilePath(proj.activeFile));
    }

    return proj;
}

bool CoverageProject::saveToFile(const QString& path) {
    QString savePath = path.isEmpty() ? filePath : path;
    if (savePath.isEmpty()) return false;

    QFileInfo fi(savePath);
    QDir().mkpath(fi.absolutePath());
    rootDir = fi.absolutePath();
    filePath = QDir::toNativeSeparators(savePath);

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

CoverageProject CoverageProject::loadFromFile(const QString& path, bool* ok) {
    if (ok) *ok = false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return CoverageProject();
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return CoverageProject();
    }

    if (ok) *ok = true;
    return fromJson(doc.object(), path);
}

// -----------------------------------------------------------------------------
// ProjectManager 实现
// -----------------------------------------------------------------------------

ProjectManager& ProjectManager::instance() {
    static ProjectManager s_instance;
    return s_instance;
}

ProjectManager::ProjectManager() {
    QString configDir = QDir::currentPath() + QStringLiteral("/config");
    QDir().mkpath(configDir);
    m_recentProjectsFile = configDir + QStringLiteral("/recent_projects.json");
    loadRecentProjects();
}

void ProjectManager::loadRecentProjects() {
    m_recentProjects.clear();
    QFile file(m_recentProjectsFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) {
            for (auto val : doc.array()) {
                QString p = val.toString();
                if (QFile::exists(p)) {
                    m_recentProjects.append(QDir::toNativeSeparators(p));
                }
            }
        }
        file.close();
    }
}

void ProjectManager::saveRecentProjects() {
    QJsonArray arr;
    for (const QString& p : m_recentProjects) {
        arr.append(p);
    }
    QFile file(m_recentProjectsFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(arr);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

QStringList ProjectManager::recentProjects() const {
    return m_recentProjects;
}

void ProjectManager::addRecentProject(const QString& path) {
    QString norm = QDir::toNativeSeparators(path);
    m_recentProjects.removeAll(norm);
    m_recentProjects.prepend(norm);
    while (m_recentProjects.size() > 10) {
        m_recentProjects.removeLast();
    }
    saveRecentProjects();

    // 同步到 AppConfig
    AppConfig::instance().lastProjectPath = norm;
    AppConfig::instance().save();
}

void ProjectManager::removeRecentProject(const QString& path) {
    QString norm = QDir::toNativeSeparators(path);
    m_recentProjects.removeAll(norm);
    saveRecentProjects();
}

void ProjectManager::clearRecentProjects() {
    m_recentProjects.clear();
    saveRecentProjects();
}

bool ProjectManager::createNewProject(const QString& name, const QString& projectDir,
                                      const QString& targetArch,
                                      const QStringList& initialSources,
                                      const QStringList& initialScripts)
{
    QDir dir(projectDir);
    if (!dir.exists()) {
        dir.mkpath(projectDir);
    }

    // 严格按照工程分层规范创建子目录：Source, Header, Script, Report
    dir.mkpath(projectDir + QStringLiteral("/Source"));
    dir.mkpath(projectDir + QStringLiteral("/Header"));
    dir.mkpath(projectDir + QStringLiteral("/Script"));
    dir.mkpath(projectDir + QStringLiteral("/Report"));

    QString covprojPath = dir.filePath(name + QStringLiteral(".covproj"));
    CoverageProject proj;
    proj.name = name;
    proj.filePath = QDir::toNativeSeparators(covprojPath);
    proj.rootDir = QDir::toNativeSeparators(projectDir);
    proj.targetArch = targetArch;
    proj.outputDir = QStringLiteral("Report");
    proj.sourceFiles = initialSources;
    proj.scriptFiles = initialScripts;
    proj.createdTime = QDateTime::currentDateTime();
    proj.lastModifiedTime = proj.createdTime;

    if (!initialSources.isEmpty()) {
        proj.activeFile = initialSources.first();
    }

    if (proj.saveToFile(covprojPath)) {
        m_currentProject = proj;
        addRecentProject(covprojPath);
        Logger::instance().log(LogLevel::Info, QStringLiteral("Project"),
            QStringLiteral("成功创建新工程：%1 (%2)").arg(name, covprojPath));
        return true;
    }
    return false;
}

bool ProjectManager::openProject(const QString& covprojPath) {
    if (!QFile::exists(covprojPath)) return false;

    bool ok = false;
    CoverageProject proj = CoverageProject::loadFromFile(covprojPath, &ok);
    if (ok) {
        m_currentProject = proj;
        addRecentProject(covprojPath);
        Logger::instance().log(LogLevel::Info, QStringLiteral("Project"),
            QStringLiteral("已成功打开工程：%1，包含 %2 个源文件，%3 个激励脚本。")
                .arg(proj.name).arg(proj.sourceFiles.size()).arg(proj.scriptFiles.size()));
        return true;
    }
    return false;
}

bool ProjectManager::saveCurrentProject() {
    if (!m_currentProject.isValid()) return false;
    m_currentProject.lastModifiedTime = QDateTime::currentDateTime();
    return m_currentProject.saveToFile();
}

void ProjectManager::closeCurrentProject() {
    m_currentProject = CoverageProject();
    Logger::instance().log(LogLevel::Info, QStringLiteral("Project"), QStringLiteral("工程已关闭。"));
}

bool ProjectManager::openBuiltinSample(const QString& sampleId) {
    QString workspace = QStringLiteral("d:/WorkSpace/AI_Work/Coverage Analysis");
    if (QDir(QDir::currentPath() + QStringLiteral("/examples")).exists()) {
        workspace = QDir::currentPath();
    } else if (QDir(QCoreApplication::applicationDirPath() + QStringLiteral("/../examples")).exists()) {
        workspace = QFileInfo(QCoreApplication::applicationDirPath() + QStringLiteral("/..")).absoluteFilePath();
    }

    QString sampleProjPath;

    if (sampleId == QStringLiteral("sensor_fusion")) {
        sampleProjPath = workspace + QStringLiteral("/examples/sensor_fusion/sensor_fusion.covproj");
        if (!QFile::exists(sampleProjPath)) {
            // 创建示例工程文件
            CoverageProject p;
            p.name = QStringLiteral("航姿参考多传感器融合与位姿滤波系统 (Sensor Fusion)");
            p.description = QStringLiteral("适用于无人机与高精度惯导系统的姿态航向参考系统 (AHRS)。融合六轴陀螺仪、加速度计与电子罗盘互补滤波算法，包含四元数姿态解算、气压计高度滤波以及正弦振动波形激励注入套件。");
            p.targetArch = QStringLiteral("ARM Cortex-M4 (STM32F4)");
            p.filePath = sampleProjPath;
            p.rootDir = workspace + QStringLiteral("/examples/sensor_fusion");
            p.outputDir = QStringLiteral("Report");
            p.sourceFiles = {
                workspace + QStringLiteral("/examples/sensor_fusion/Source/sensor_fusion.c"),
                workspace + QStringLiteral("/examples/sensor_fusion/Header/sensor_fusion.h")
            };
            p.scriptFiles = {
                workspace + QStringLiteral("/examples/sensor_fusion/Script/sensor_waveform_injector.py")
            };
            p.activeFile = workspace + QStringLiteral("/examples/sensor_fusion/Source/sensor_fusion.c");
            p.createdTime = QDateTime::currentDateTime();
            p.lastModifiedTime = p.createdTime;
            p.saveToFile(sampleProjPath);
        }
    } else {
        // default: motor_controller
        sampleProjPath = workspace + QStringLiteral("/examples/motor_controller/motor_controller.covproj");
        if (!QFile::exists(sampleProjPath)) {
            CoverageProject p;
            p.name = QStringLiteral("车载电机驱动与故障安全系统 (Motor Controller)");
            p.description = QStringLiteral("车规级高可靠三相直流无刷电机 (BLDC) FOC 矢量控制与故障安全闭锁保护系统 (符合 ISO 26262 ASIL-B 规范)。包含速度/电流双闭环 PID 控制器、相电流校准、PWM 死区硬件驱动、超温/过压闭锁、CAN 报文通信协议栈以及主调度器。");
            p.targetArch = QStringLiteral("ARM Cortex-M4 (STM32F4)");
            p.filePath = sampleProjPath;
            p.rootDir = workspace + QStringLiteral("/examples/motor_controller");
            p.outputDir = QStringLiteral("Report");
            p.sourceFiles = {
                workspace + QStringLiteral("/examples/motor_controller/Source/main_embedded.c"),
                workspace + QStringLiteral("/examples/motor_controller/Source/motor_controller.c"),
                workspace + QStringLiteral("/examples/motor_controller/Source/bsp_pwm.c"),
                workspace + QStringLiteral("/examples/motor_controller/Source/safety_monitor.c"),
                workspace + QStringLiteral("/examples/motor_controller/Source/can_comm.c"),
                workspace + QStringLiteral("/examples/motor_controller/Header/motor_types.h"),
                workspace + QStringLiteral("/examples/motor_controller/Header/motor_controller.h"),
                workspace + QStringLiteral("/examples/motor_controller/Header/bsp_pwm.h"),
                workspace + QStringLiteral("/examples/motor_controller/Header/safety_monitor.h"),
                workspace + QStringLiteral("/examples/motor_controller/Header/can_comm.h")
            };
            p.scriptFiles = {
                workspace + QStringLiteral("/examples/motor_controller/Script/can_frame_injector.py"),
                workspace + QStringLiteral("/examples/motor_controller/Script/boundary_fuzzer.py")
            };
            p.activeFile = workspace + QStringLiteral("/examples/motor_controller/Source/main_embedded.c");
            p.createdTime = QDateTime::currentDateTime();
            p.lastModifiedTime = p.createdTime;
            p.saveToFile(sampleProjPath);
        }
    }

    return openProject(sampleProjPath);
}

} // namespace Coverage
