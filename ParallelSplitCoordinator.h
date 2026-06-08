/**
 * @file ParallelSplitCoordinator.h
 * @brief 并行拆分协调器声明
 *
 * ParallelSplitCoordinator 负责管理多个工作线程的生命周期，
 * 汇总进度，处理暂停/继续/停止请求，收集错误信息。
 */

#ifndef PARALLELSPLITCOORDINATOR_H
#define PARALLELSPLITCOORDINATOR_H

#include "ParallelSplitWorker.h"
#include <windows.h>
#include <vector>

/**
 * @brief 并行拆分协调器类
 *
 * 协调多个 ParallelSplitWorker 线程，提供统一的启动、暂停、继续、停止接口。
 */
class ParallelSplitCoordinator
{
public:
    ParallelSplitCoordinator();
    ~ParallelSplitCoordinator();

    /**
     * @brief 启动并行拆分
     *
     * 创建并启动多个工作线程，每个线程处理一个分块。
     *
     * @param tasks     拆分任务列表
     * @param threadCount 线程数量（必须 <= tasks.size()）
     * @return 0 成功，负数失败
     */
    int start(const std::vector<SplitTask>& tasks, int threadCount);

    /**
     * @brief 等待所有线程完成
     *
     * 阻塞直到所有工作线程结束。
     *
     * @param timeoutMs 超时时间（毫秒），INFINITE 表示无限等待
     * @return true 所有线程已完成，false 超时
     */
    bool wait(DWORD timeoutMs = INFINITE);

    /**
     * @brief 请求暂停
     *
     * 所有工作线程将在下次检测点挂起。
     */
    void pause();

    /**
     * @brief 请求继续
     *
     * 唤醒所有挂起的工作线程。
     */
    void resume();

    /**
     * @brief 请求停止
     *
     * 所有工作线程将在下次检测点退出。
     */
    void stop();

    /**
     * @brief 获取总进度
     *
     * 汇总所有工作线程的完成字节数。
     *
     * @param completedBytes 已完成字节数（输出）
     * @param totalBytes     总字节数（输出）
     * @return 进度百分比（0-100）
     */
    int getProgress(long long& completedBytes, long long& totalBytes) const;

    /**
     * @brief 检查是否已完成
     *
     * @return true 所有线程已完成，false 仍有线程运行
     */
    bool isFinished() const;

    /**
     * @brief 获取错误信息
     *
     * 返回第一个发生错误的线程的错误信息。
     *
     * @param errorCode 错误码（输出）
     * @return 错误信息字符串，无错误时返回空字符串
     */
    std::string getError(int& errorCode) const;

    /**
     * @brief 获取返回码
     *
     * 返回拆分结果：
     *   - 0    成功
     *   - -100 用户取消
     *   - -200 其他线程出错
     *   - 其他 负数错误码
     *
     * @return 返回码
     */
    int getReturnCode() const;

private:
    // 禁止拷贝
    ParallelSplitCoordinator(const ParallelSplitCoordinator&);
    ParallelSplitCoordinator& operator=(const ParallelSplitCoordinator&);

    /**
     * @brief 清理资源
     *
     * 关闭线程句柄，释放状态数组。
     */
    void cleanup();

    // 成员变量
    std::vector<HANDLE>             m_threadHandles;    ///< 线程句柄数组
    std::vector<WorkerThreadParam>  m_threadParams;     ///< 线程参数数组
    std::vector<WorkerSharedState>  m_sharedStates;     ///< 共享状态数组
    HANDLE                          m_hPauseEvent;      ///< 暂停事件句柄
    long long                       m_totalBytes;       ///< 总字节数
    int                             m_threadCount;      ///< 线程数量
    bool                            m_isRunning;        ///< 是否正在运行
};

#endif // PARALLELSPLITCOORDINATOR_H
