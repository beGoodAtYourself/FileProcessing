/**
 * @file ParallelSplitCoordinator.cpp
 * @brief 并行拆分协调器实现
 *
 * 实现多线程并行拆分的协调逻辑：
 *   - 创建并启动工作线程
 *   - 汇总进度
 *   - 处理暂停/继续/停止请求
 *   - 收集错误信息
 */

#include "ParallelSplitCoordinator.h"
#include <algorithm>

// ============================================================================
// 构造 / 析构
// ============================================================================

ParallelSplitCoordinator::ParallelSplitCoordinator()
    : m_hPauseEvent(NULL)
    , m_totalBytes(0)
    , m_threadCount(0)
    , m_isRunning(false)
{
}

ParallelSplitCoordinator::~ParallelSplitCoordinator()
{
    cleanup();
}

// ============================================================================
// 公共方法
// ============================================================================

/**
 * @brief 启动并行拆分
 *
 * 流程：
 *   1. 校验参数
 *   2. 创建暂停事件对象
 *   3. 分配线程参数和共享状态
 *   4. 创建工作线程
 */
int ParallelSplitCoordinator::start(const std::vector<SplitTask>& tasks, int threadCount)
{
    // 参数校验
    if (tasks.empty() || threadCount <= 0)
    {
        return -1;
    }

    // 线程数不能超过任务数
    int actualThreadCount = (std::min)(threadCount, (int)tasks.size());

    // 清理之前的资源
    cleanup();

    // 创建暂停事件（手动复位，初始有信号=非暂停状态）
    m_hPauseEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (m_hPauseEvent == NULL)
    {
        return -2;
    }

    // 分配资源
    m_threadHandles.resize(actualThreadCount);
    m_threadParams.resize(actualThreadCount);
    m_sharedStates.resize(actualThreadCount);
    m_totalBytes = 0;
    m_threadCount = actualThreadCount;

    // 初始化参数和状态
    for (int i = 0; i < actualThreadCount; ++i)
    {
        m_threadParams[i].task = tasks[i];
        m_threadParams[i].pSharedState = &m_sharedStates[i];
        m_threadParams[i].hPauseEvent = m_hPauseEvent;

        m_sharedStates[i].partIndex = tasks[i].partIndex;
        m_sharedStates[i].stopFlag = 0;
        m_sharedStates[i].pauseFlag = 0;
        m_sharedStates[i].completedBytes = 0;
        m_sharedStates[i].hasError = 0;
        m_sharedStates[i].errorCode = 0;
        memset(m_sharedStates[i].errorMessage, 0, sizeof(m_sharedStates[i].errorMessage));

        m_totalBytes += tasks[i].bytesToProcess;
    }

    // 创建工作线程
    for (int i = 0; i < actualThreadCount; ++i)
    {
        m_threadHandles[i] = CreateThread(
            NULL,                           // 默认安全属性
            0,                              // 默认栈大小
            ParallelSplitWorkerProc,        // 线程函数
            &m_threadParams[i],             // 参数
            0,                              // 立即运行
            NULL                            // 不需要线程ID
        );

        if (m_threadHandles[i] == NULL)
        {
            // 创建线程失败，停止已创建的线程
            stop();
            cleanup();
            return -3;
        }
    }

    m_isRunning = true;
    return 0;
}

/**
 * @brief 等待所有线程完成
 */
bool ParallelSplitCoordinator::wait(DWORD timeoutMs)
{
    if (m_threadHandles.empty())
    {
        return true;
    }

    DWORD result = WaitForMultipleObjects(
        (DWORD)m_threadHandles.size(),
        &m_threadHandles[0],
        TRUE,       // 等待所有线程
        timeoutMs
    );

    if (result == WAIT_TIMEOUT)
    {
        return false;
    }

    m_isRunning = false;
    return true;
}

/**
 * @brief 请求暂停
 *
 * 将暂停事件设为无信号状态，工作线程将在检测点挂起。
 */
void ParallelSplitCoordinator::pause()
{
    if (m_hPauseEvent != NULL)
    {
        ResetEvent(m_hPauseEvent);
    }
}

/**
 * @brief 请求继续
 *
 * 将暂停事件设为有信号状态，唤醒所有挂起的工作线程。
 */
void ParallelSplitCoordinator::resume()
{
    if (m_hPauseEvent != NULL)
    {
        SetEvent(m_hPauseEvent);
    }
}

/**
 * @brief 请求停止
 *
 * 设置所有共享状态的 stopFlag，工作线程将在检测点退出。
 */
void ParallelSplitCoordinator::stop()
{
    for (int i = 0; i < m_threadCount; ++i)
    {
        InterlockedExchange(&m_sharedStates[i].stopFlag, 1);
    }

    // 确保暂停的线程能被唤醒以检测停止标志
    resume();
}

/**
 * @brief 获取总进度
 */
int ParallelSplitCoordinator::getProgress(long long& completedBytes, long long& totalBytes) const
{
    totalBytes = m_totalBytes;
    completedBytes = 0;

    for (int i = 0; i < m_threadCount; ++i)
    {
        // 使用 const_cast 移除 const 属性以进行原子读取
        completedBytes += InterlockedCompareExchange64(
            const_cast<volatile LONGLONG*>(&m_sharedStates[i].completedBytes), 0, 0);
    }

    if (m_totalBytes <= 0)
    {
        return 0;
    }

    int percent = (int)((completedBytes * 100) / m_totalBytes);
    if (percent > 100) percent = 100;
    return percent;
}

/**
 * @brief 检查是否已完成
 */
bool ParallelSplitCoordinator::isFinished() const
{
    if (!m_isRunning)
    {
        return true;
    }

    if (m_threadHandles.empty())
    {
        return true;
    }

    // 检查所有线程是否已结束
    DWORD result = WaitForMultipleObjects(
        (DWORD)m_threadHandles.size(),
        const_cast<HANDLE*>(&m_threadHandles[0]),
        TRUE,
        0   // 不等待，立即返回
    );

    return (result == WAIT_OBJECT_0);
}

/**
 * @brief 获取错误信息
 */
std::string ParallelSplitCoordinator::getError(int& errorCode) const
{
    errorCode = 0;

    for (int i = 0; i < m_threadCount; ++i)
    {
        if (InterlockedCompareExchange(
                const_cast<volatile LONG*>(&m_sharedStates[i].hasError), 0, 0) != 0)
        {
            errorCode = m_sharedStates[i].errorCode;
            return std::string(m_sharedStates[i].errorMessage);
        }
    }

    return "";
}

/**
 * @brief 获取返回码
 */
int ParallelSplitCoordinator::getReturnCode() const
{
    // 检查是否有停止请求（用户取消）
    for (int i = 0; i < m_threadCount; ++i)
    {
        if (InterlockedCompareExchange(
                const_cast<volatile LONG*>(&m_sharedStates[i].stopFlag), 0, 0) != 0)
        {
            return -100;  // 用户取消
        }
    }

    // 检查是否有错误
    int errorCode = 0;
    std::string errorMsg = getError(errorCode);
    if (!errorMsg.empty())
    {
        return errorCode;
    }

    // 检查是否已完成
    if (isFinished())
    {
        return 0;  // 成功
    }

    return -1;  // 未完成
}

// ============================================================================
// 私有方法
// ============================================================================

/**
 * @brief 清理资源
 */
void ParallelSplitCoordinator::cleanup()
{
    // 关闭线程句柄
    for (size_t i = 0; i < m_threadHandles.size(); ++i)
    {
        if (m_threadHandles[i] != NULL)
        {
            CloseHandle(m_threadHandles[i]);
        }
    }
    m_threadHandles.clear();

    // 关闭暂停事件句柄
    if (m_hPauseEvent != NULL)
    {
        CloseHandle(m_hPauseEvent);
        m_hPauseEvent = NULL;
    }

    // 清理其他资源
    m_threadParams.clear();
    m_sharedStates.clear();
    m_totalBytes = 0;
    m_threadCount = 0;
    m_isRunning = false;
}
