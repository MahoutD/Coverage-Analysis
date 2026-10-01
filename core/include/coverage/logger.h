#pragma once

#include "coverage/types.h"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QRecursiveMutex>
#include <QStringList>

namespace Coverage {

/**
 * @brief 企业级运行日志记录系统 (Logger)
 * 提供线程安全的磁盘日志持久化、崩溃与异常抓取、按运行会话自动切分日志文件，
 * 支持日志回溯检索与 BUG 定位排查。
 */
class Logger {
public:
    static Logger& instance();

    /**
     * @brief 初始化日志系统
     * @param customDir 自定义日志目录（默认定位为应用同级 logs/ 目录）
     */
    void init(const QString& customDir = QString());

    /**
     * @brief 输出一条结构化日志并写入磁盘
     */
    void log(LogLevel level, const QString& module, const QString& message);

    /**
     * @brief 获取当前会话生成的日志文件完整物理路径
     */
    QString currentLogFilePath() const;

    /**
     * @brief 获取日志存储所在的目录路径
     */
    QString logDirectory() const;

    /**
     * @brief 读取最近记录的内存日志列表
     */
    QStringList recentLogs(int maxCount = 100) const;

    /**
     * @brief 强制将缓冲区数据落盘
     */
    void flush();

    /**
     * @brief 安装全局 Qt 消息拦截器 (qInstallMessageHandler)
     */
    void installQtMessageHandler();

private:
    Logger() = default;
    ~Logger();

    mutable QRecursiveMutex m_mutex;
    QFile m_logFile;
    QTextStream m_stream;
    QString m_logDir;
    QString m_logFilePath;
    QStringList m_memoryBuffer;
    bool m_initialized = false;
};

} // namespace Coverage
