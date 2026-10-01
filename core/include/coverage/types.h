#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <cstdint>

namespace Coverage {

/**
 * @brief 静态分析缺陷严重级别
 */
enum class Severity {
    Info,       ///< 提示信息
    Style,      ///< 编码风格规范
    Warning,    ///< 潜在风险警告
    Error,      ///< 严重安全/功能违规
    Critical    ///< 致命错误 (如安全关键系统中的动态内存分配)
};

/**
 * @brief 缺陷级别转中文字符串
 */
inline QString severityToString(Severity s) {
    switch (s) {
        case Severity::Info: return QStringLiteral("提示 (Info)");
        case Severity::Style: return QStringLiteral("规范 (Style)");
        case Severity::Warning: return QStringLiteral("警告 (Warning)");
        case Severity::Error: return QStringLiteral("错误 (Error)");
        case Severity::Critical: return QStringLiteral("严重违规 (Critical)");
    }
    return QStringLiteral("未知");
}

/**
 * @brief 代码行代码覆盖状态
 */
enum class LineCoverageStatus {
    NotExecutable,  ///< 非可执行语句 (注释、宏定义、大括号等)
    Uncovered,      ///< 未覆盖 (执行次数为 0)
    PartialBranch,  ///< 条件/判定部分分支覆盖 (仅 True 或仅 False 发生)
    Covered         ///< 完全覆盖 (语句执行或分支均被触发)
};

/**
 * @brief 行覆盖状态转中文字符串
 */
inline QString coverageStatusToString(LineCoverageStatus status) {
    switch (status) {
        case LineCoverageStatus::NotExecutable: return QStringLiteral("非可执行行");
        case LineCoverageStatus::Uncovered: return QStringLiteral("未覆盖");
        case LineCoverageStatus::PartialBranch: return QStringLiteral("部分分支覆盖");
        case LineCoverageStatus::Covered: return QStringLiteral("完全覆盖");
    }
    return QStringLiteral("未知");
}

/**
 * @brief 规则所属分类分类体系
 */
enum class RuleCategory {
    MisraC,                     ///< MISRA C:2012 工业安全标准
    MemorySafety,               ///< 内存与指针安全 (越界、栈溢出、动态分配)
    ConcurrencyAndInterrupt,    ///< 中断服务例程 (ISR) 与死循环超时
    HardwareAndRegisters,       ///< 硬件外设寄存器地址 volatile 访问
    ComplexityAndStructure,     ///< 圈复杂度与深度嵌套控制流
    CodeSmell                   ///< 代码坏味道与死代码
};

/**
 * @brief 规则分类转中文字符串
 */
inline QString categoryToString(RuleCategory cat) {
    switch (cat) {
        case RuleCategory::MisraC: return QStringLiteral("MISRA C:2012 规范");
        case RuleCategory::MemorySafety: return QStringLiteral("嵌入式内存安全");
        case RuleCategory::ConcurrencyAndInterrupt: return QStringLiteral("并发与中断/看门狗");
        case RuleCategory::HardwareAndRegisters: return QStringLiteral("硬件寄存器访问");
        case RuleCategory::ComplexityAndStructure: return QStringLiteral("复杂度与结构");
        case RuleCategory::CodeSmell: return QStringLiteral("代码坏味道");
    }
    return QStringLiteral("通用规范");
}

/**
 * @brief 系统运行日志等级
 */
enum class LogLevel {
    Debug,      ///< 调试跟踪
    Info,       ///< 常规信息
    Warning,    ///< 告警
    Error       ///< 运行异常
};

} // namespace Coverage
