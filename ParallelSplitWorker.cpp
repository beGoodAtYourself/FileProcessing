/**
 * @file ParallelSplitWorker.cpp
 * @brief 并行拆分工作线程函数实现
 *
 * 实现多线程并行文件拆分的核心逻辑：
 *   - 每个线程独立打开源文件
 *   - 通过 _fseeki64 定位到指定偏移量
 *   - 循环读取源文件并写入输出文件
 *   - 支持暂停检测和停止请求
 */

#include "ParallelSplitWorker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 工作线程入口函数
 *
 * 执行单个分块的拆分任务：
 *   1. 打开源文件（独立 FILE*）
 *   2. 定位到指定偏移量
 *   3. 创建输出文件
 *   4. 循环读取并写入
 *   5. 检查暂停/停止信号
 *   6. 更新进度
 *
 * @param lpParam WorkerThreadParam 指针
 * @return 0 成功，非0 错误码
 */
DWORD WINAPI ParallelSplitWorkerProc(LPVOID lpParam)
{
    WorkerThreadParam* pParam = (WorkerThreadParam*)lpParam;
    if (pParam == NULL)
    {
        return -1;
    }

    FILE* srcFp = NULL;
    FILE* dstFp = NULL;
    char* buffer = NULL;
    int ret = 0;

    // ---- 打开源文件（独立句柄） ----
    srcFp = fopen(pParam->task.sourcePath, "rb");
    if (srcFp == NULL)
    {
        pParam->pSharedState->errorCode = -2;
        strncpy(pParam->pSharedState->errorMessage, "Failed to open source file", 
                sizeof(pParam->pSharedState->errorMessage) - 1);
        InterlockedExchange(&pParam->pSharedState->hasError, 1);
        return -2;
    }

    // ---- 定位到起始偏移量 ----
    if (_fseeki64(srcFp, pParam->task.startOffset, SEEK_SET) != 0)
    {
        pParam->pSharedState->errorCode = -3;
        strncpy(pParam->pSharedState->errorMessage, "Failed to seek source file", 
                sizeof(pParam->pSharedState->errorMessage) - 1);
        InterlockedExchange(&pParam->pSharedState->hasError, 1);
        fclose(srcFp);
        return -3;
    }

    // ---- 创建输出文件 ----
    dstFp = fopen(pParam->task.outputPath, "wb");
    if (dstFp == NULL)
    {
        pParam->pSharedState->errorCode = -4;
        strncpy(pParam->pSharedState->errorMessage, "Failed to create output file", 
                sizeof(pParam->pSharedState->errorMessage) - 1);
        InterlockedExchange(&pParam->pSharedState->hasError, 1);
        fclose(srcFp);
        return -4;
    }

    // ---- 分配缓冲区 ----
    buffer = (char*)malloc(PS_BUFFER_SIZE);
    if (buffer == NULL)
    {
        pParam->pSharedState->errorCode = -5;
        strncpy(pParam->pSharedState->errorMessage, "Failed to allocate buffer", 
                sizeof(pParam->pSharedState->errorMessage) - 1);
        InterlockedExchange(&pParam->pSharedState->hasError, 1);
        fclose(dstFp);
        fclose(srcFp);
        return -5;
    }

    // ---- 主读写循环 ----
    long long remaining = pParam->task.bytesToProcess;
    long long totalWritten = 0;

    while (remaining > 0)
    {
        // 检查停止标志
        if (InterlockedCompareExchange(&pParam->pSharedState->stopFlag, 0, 0) != 0)
        {
            ret = -100;  // 用户取消
            break;
        }

        // 检查暂停标志（通过事件对象）
        if (pParam->hPauseEvent != NULL)
        {
            // 等待暂停事件变为有信号状态（非暂停状态）
            WaitForSingleObject(pParam->hPauseEvent, INFINITE);
        }

        // 检查其他线程是否出错（级联停止）
        if (InterlockedCompareExchange(&pParam->pSharedState->hasError, 0, 0) != 0)
        {
            ret = -200;  // 其他线程出错
            break;
        }

        // 计算本次读取字节数
        long long bytesToRead = (remaining > PS_BUFFER_SIZE) ? PS_BUFFER_SIZE : remaining;

        // 读取源文件
        size_t bytesRead = fread(buffer, 1, (size_t)bytesToRead, srcFp);
        if (bytesRead == 0 && ferror(srcFp))
        {
            pParam->pSharedState->errorCode = -6;
            strncpy(pParam->pSharedState->errorMessage, "Failed to read source file", 
                    sizeof(pParam->pSharedState->errorMessage) - 1);
            InterlockedExchange(&pParam->pSharedState->hasError, 1);
            ret = -6;
            break;
        }

        // 写入输出文件
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, dstFp);
        if (bytesWritten != bytesRead)
        {
            pParam->pSharedState->errorCode = -7;
            strncpy(pParam->pSharedState->errorMessage, "Failed to write output file", 
                    sizeof(pParam->pSharedState->errorMessage) - 1);
            InterlockedExchange(&pParam->pSharedState->hasError, 1);
            ret = -7;
            break;
        }

        // 更新计数器
        remaining -= bytesRead;
        totalWritten += bytesWritten;

        // 原子更新完成字节数（64位）
        InterlockedExchangeAdd64(&pParam->pSharedState->completedBytes, (LONGLONG)bytesWritten);
    }

    // ---- 资源清理 ----
    if (buffer != NULL)
    {
        free(buffer);
    }
    if (dstFp != NULL)
    {
        fclose(dstFp);
    }
    if (srcFp != NULL)
    {
        fclose(srcFp);
    }

    return ret;
}
