/**
 * @file ParallelSplitWorker.h
 * @brief 并行拆分工作线程数据结构与函数声明
 *
 * 基于 Windows 原生线程 API 实现多线程并行文件拆分。
 * 每个工作线程独立打开源文件，定位到指定偏移量，并行读写。
 */

#ifndef PARALLELSPLITWORKER_H
#define PARALLELSPLITWORKER_H

#include <string>
#include <vector>
#include <windows.h>

// 缓冲区大小（4MB）—— 与 FileSplitterCore 保持一致
#define PS_BUFFER_SIZE (4 * 1024 * 1024)

/**
 * @brief 拆分任务参数结构体
 *
 * 描述单个工作线程需要处理的源文件区间和输出路径。
 */
struct SplitTask
{
    char        sourcePath[512];    ///< 源文件完整路径
	long long   startOffset;        ///< 源文件起始偏移量（字节）
	long long   bytesToProcess;     ///< 需要处理的字节数
    char        outputPath[512];    ///< 输出子文件完整路径
    int         partIndex;          ///< 分块序号（用于进度回调，0-based）
    
    SplitTask()
        : startOffset(0)
        , bytesToProcess(0)
        , partIndex(0)
    {
        memset(sourcePath, 0, sizeof(sourcePath));
        memset(outputPath, 0, sizeof(outputPath));
    }
};

/**
 * @brief 工作线程共享状态结构体
 *
 * 用于协调器与工作线程之间交换状态信息。
 * 所有 volatile 变量通过 Windows Interlocked API 原子操作。
 */
struct WorkerSharedState
{
    volatile LONG    stopFlag;          ///< 停止标志（非0表示请求停止）
    volatile LONG    pauseFlag;         ///< 暂停标志（非0表示请求暂停）
    volatile LONGLONG completedBytes;   ///< 该线程已完成的字节数（64位支持大文件）
    volatile LONG    hasError;          ///< 错误标志（非0表示发生错误）
    int              errorCode;         ///< 错误码（仅当 hasError 非零时有效）
    char             errorMessage[512]; ///< 错误信息（仅当 hasError 非零时有效）
    int              partIndex;         ///< 分块序号（用于进度回调）
    
    WorkerSharedState()
        : stopFlag(0)
        , pauseFlag(0)
        , completedBytes(0)
        , hasError(0)
        , errorCode(0)
        , partIndex(0)
    {
        memset(errorMessage, 0, sizeof(errorMessage));
    }
};

/**
 * @brief 工作线程参数结构体
 *
 * 作为 CreateThread 的参数传递给线程函数。
 */
struct WorkerThreadParam
{
    SplitTask           task;           ///< 拆分任务参数
    WorkerSharedState*  pSharedState;   ///< 共享状态指针（由协调器分配）
    HANDLE              hPauseEvent;    ///< 暂停事件句柄（手动复位）
    
    WorkerThreadParam()
        : pSharedState(NULL)
        , hPauseEvent(NULL)
    {
    }
};

/**
 * @brief 并行拆分工作线程入口函数
 *
 * 每个工作线程独立执行以下流程：
 *   1. 独立 fopen 源文件（只读模式）
 *   2. _fseeki64 定位到 startOffset
 *   3. 循环读取并写入输出文件
 *   4. 检查暂停/停止信号
 *   5. 更新完成字节数
 *
 * @param lpParam WorkerThreadParam 指针
 * @return 0 成功，非0 错误码
 */
DWORD WINAPI ParallelSplitWorkerProc(LPVOID lpParam);

#endif // PARALLELSPLITWORKER_H
