#include "coverage/logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QThread>
#include <QSysInfo>
#include <QMutexLocker>
#include <iostream>

#include <QLoggingCategory>

namespace Coverage {

static void customQtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    // 1. 过滤 Windows 平台与 DirectWrite/字体枚举中的框架内部良性诊断/裁剪警告
    if (msg.contains(QLatin1String("setGeometry: Unable to set geometry")) ||
        msg.contains(QLatin1String("DirectWrite: CreateFontFaceFromHDC() failed")) ||
        msg.contains(QLatin1String("OpenType support missing"))) {
        return;
    }

    // 2. 规范化 category 模块名称，避免出现模糊且不规范的 [default     ]
    QString category = context.category ? QString::fromUtf8(context.category) : QString();
    if (category.isEmpty() || category == QLatin1String("default")) {
        category = QStringLiteral("System");
    } else if (category.startsWith(QLatin1String("qt."))) {
        // 过滤 Qt 框架底层非严重内部调试/字体匹配信息，避免污染用户业务实时运行日志
        if (type != QtCriticalMsg && type != QtFatalMsg) {
            return;
        }
        category = QStringLiteral("QtPlatform");
    }

    LogLevel level = LogLevel::Info;
    switch (type) {
    case QtDebugMsg:
        level = LogLevel::Debug;
        break;
    case QtInfoMsg:
        level = LogLevel::Info;
        break;
    case QtWarningMsg:
        level = LogLevel::Warning;
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        level = LogLevel::Error;
        break;
    }

    Logger::instance().log(level, category, msg);
}

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

Logger::~Logger() {
    flush();
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::init(const QString& customDir) {
    QMutexLocker locker(&m_mutex);
    if (m_initialized) return;

    if (!customDir.isEmpty()) {
        m_logDir = customDir;
    } else {
        QString appDir = QCoreApplication::applicationDirPath();
        if (appDir.isEmpty()) {
            appDir = QDir::currentPath();
        }
        m_logDir = appDir + QStringLiteral("/logs");
    }

    QDir dir(m_logDir);
    if (!dir.exists()) {
        dir.mkpath(m_logDir);
    }

    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"));
    m_logFilePath = m_logDir + QStringLiteral("/coverage_studio_") + timestamp + QStringLiteral(".log");

    m_logFile.setFileName(m_logFilePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_logFile);
        m_stream << "================================================================================" << "\n";
        m_stream << " Embedded C Coverage & Static Analysis Suite - System Runtime Log" << "\n";
        m_stream << " Session Started: " << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")) << "\n";
        m_stream << " OS Platform:     " << QSysInfo::prettyProductName() << " (" << QSysInfo::currentCpuArchitecture() << ")" << "\n";
        m_stream << " Qt Version:      " << QT_VERSION_STR << "\n";
        m_stream << " Log File:        " << m_logFilePath << "\n";
        m_stream << "================================================================================" << "\n\n";
        m_stream.flush();
    }

    m_initialized = true;
}

void Logger::installQtMessageHandler() {
    // 过滤底层良性框架日志与字体库枚举告警
    QLoggingCategory::setFilterRules(QStringLiteral(
        "qt.text.font.db.warning=false\n"
        "qt.text.font.db.info=false\n"
        "qt.qpa.fonts.warning=false\n"
        "qt.qpa.windows.warning=false\n"
        "default.warning=false\n"
    ));

    qInstallMessageHandler(customQtMessageHandler);
}

void Logger::log(LogLevel level, const QString& module, const QString& message) {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        init();
    }

    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    QString levelStr = QStringLiteral("INFO");
    switch (level) {
    case LogLevel::Debug:
        levelStr = QStringLiteral("DEBUG");
        break;
    case LogLevel::Info:
        levelStr = QStringLiteral("INFO");
        break;
    case LogLevel::Warning:
        levelStr = QStringLiteral("WARN");
        break;
    case LogLevel::Error:
        levelStr = QStringLiteral("ERROR");
        break;
    }

    quintptr threadId = reinterpret_cast<quintptr>(QThread::currentThreadId());
    QString formatted = QStringLiteral("[%1] [%2] [T:0x%3] [%4] %5")
                            .arg(timeStr)
                            .arg(levelStr, -5)
                            .arg(QString::number(threadId, 16))
                            .arg(module, -12)
                            .arg(message);

    // 内存缓冲区保留最近记录
    m_memoryBuffer.append(formatted);
    if (m_memoryBuffer.size() > 500) {
        m_memoryBuffer.removeFirst();
    }

    // 写入日志文件
    if (m_logFile.isOpen()) {
        m_stream << formatted << "\n";
        m_stream.flush();
    }
}

QString Logger::currentLogFilePath() const {
    QMutexLocker locker(&m_mutex);
    return m_logFilePath;
}

QString Logger::logDirectory() const {
    QMutexLocker locker(&m_mutex);
    return m_logDir;
}

QStringList Logger::recentLogs(int maxCount) const {
    QMutexLocker locker(&m_mutex);
    if (maxCount >= m_memoryBuffer.size()) {
        return m_memoryBuffer;
    }
    return m_memoryBuffer.mid(m_memoryBuffer.size() - maxCount);
}

void Logger::flush() {
    QMutexLocker locker(&m_mutex);
    if (m_logFile.isOpen()) {
        m_stream.flush();
    }
}

} // namespace Coverage
