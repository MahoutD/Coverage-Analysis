#pragma once

#include "coverage/models.h"
#include <QString>
#include <QVector>
#include <QMap>

namespace Coverage {

/**
 * @brief 插桩元数据字典
 * 记录源码行号、分支编号与探针 ID 的对应关系映射，
 * 供后续覆盖度量统计引擎进行运行时轨迹反向映射。
 */
struct InstrumentationMetadata {
    QString sourceFilePath;             ///< 原始源码文件路径
    QString instrumentedFilePath;       ///< 生成的带探针插桩文件路径
    int totalProbeLines = 0;            ///< 注入的语句探针总数
    int totalProbeBranches = 0;         ///< 注入的分支/判定探针总数
    int totalProbeFunctions = 0;        ///< 注入的函数探针总数
    QMap<int, int> lineToProbeId;       ///< 源码行号 -> 语句探针 ID
    QMap<int, BranchPoint> branchProbes;///< 分支探针 ID -> 分支元数据
    QMap<int, QString> functionProbes;  ///< 函数探针 ID -> 函数名称

    QJsonObject toJson() const;
    static InstrumentationMetadata fromJson(const QJsonObject& json);
};

/**
 * @brief 嵌入式 C 源码级自动探针插桩器 (Source-to-Source Instrumenter)
 * 能够无侵入地在 C 源码中插入轻量级运行时探针宏：
 * 1. 语句探针: _cov_hit_line(file_id, line)
 * 2. 判定探针: if (_cov_hit_branch(branch_id, (condition)))
 * 3. 函数探针: _cov_hit_func(func_id)
 * 并自动生成适用于目标微控制器或主机仿真的 coverage_runtime.h。
 */
class Instrumenter {
public:
    Instrumenter() = default;
    ~Instrumenter() = default;

    /**
     * @brief 插桩返回结果包装结构
     */
    struct InstrumentResult {
        bool success = false;               ///< 是否插桩成功
        QString instrumentedCode;           ///< 插桩后的完整 C 代码文本
        InstrumentationMetadata metadata;   ///< 生成的探针映射元数据
        QString errorMessage;               ///< 失败时的错误信息
    };

    /**
     * @brief 对内存中的 C 源码进行语句与分支插桩
     * @param sourceCode 原始 C 代码
     * @param sourceFileName 源码文件名
     * @return 插桩结果与元数据
     */
    InstrumentResult instrumentSource(const QString& sourceCode, const QString& sourceFileName = QStringLiteral("source.c"));

    /**
     * @brief 对指定文件插桩并保存到目标路径，并在同级目录下输出 coverage_runtime.h
     * @param inputFilePath 输入原始 C 文件
     * @param outputFilePath 输出插桩后 C 文件 (留空则默认以 .inst.c 结尾)
     * @return 插桩结果
     */
    InstrumentResult instrumentFile(const QString& inputFilePath, const QString& outputFilePath = QString());

    /**
     * @brief 生成极小内存占用的嵌入式探针运行时头文件 coverage_runtime.h
     */
    static QString generateEmbeddedRuntimeHeader();
};

} // namespace Coverage
