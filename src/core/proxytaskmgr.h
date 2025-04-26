#pragma once

#include <string>
#include <map>
#include <string>
#include <vector>

#include "../common/srs_common.h"
#include "../process/srs_app_process.hpp"
#include "../process/srs_app_ffmpeg.hpp"

/**
 * @brief 单个转码任务类，负责管理RTMP到HLS的转码过程
 * 使用FFMPEG进行实际的转码工作
 */
class IngestTask
{
public:
    virtual ~IngestTask();

    // 基本任务控制接口
    int start();      // 启动转码任务
    void stop();      // 停止转码任务
    int cycle();      // 检查任务状态
    void fast_stop(); // 快速停止任务
    void fast_kill(); // 强制终止任务

    // 初始化任务参数
    void init(std::string src, std::string dest);

    // 任务配置参数
    std::string src;   // 源RTMP流地址
    std::string dest;  // 目标路径，例如：/live/my
    std::string rtmp;  // RTMP服务地址，例如：rtmp://127.0.0.1:1936/live/my
    std::string hls;   // HLS播放地址，例如：http://127.0.0.1:8081/live/my.m3u8
    bool enable = true;// 任务启用状态

    // 运行时状态
    time_t starttime = time(0);  // 任务启动时间
    SrsFFMPEG* ffmpeg = nullptr; // FFMPEG实例指针
};

/**
 * @brief 转码任务管理器类，负责管理所有RTMP到HLS的转码任务
 * 采用单例模式实现，确保全局只有一个任务管理器实例
 */
class ProxytaskMgr
{
private:
    // 私有构造函数，实现单例模式
    ProxytaskMgr() {}

    // 成员变量
    std::map<std::string, IngestTask*> m_taskMap;  // 任务映射表，key为目标路径
    SrsProcess* m_srs_process = nullptr;           // SRS服务进程指针
    std::string m_errmsg;                         // 错误信息

    // 服务配置
    std::string m_hls_port = "8081";  // HLS服务端口
    std::string m_rtmp_port = "1936"; // RTMP服务端口

public:
    // 获取单例实例
    static ProxytaskMgr& getinstance()
    {
        static ProxytaskMgr instance;
        return instance;
    }

    // 基本接口
    int init();      // 初始化管理器
    ~ProxytaskMgr()
    {
        delete m_srs_process;
    }

    // 数据持久化接口
    int load_from_db(); // 从数据库加载任务配置

    /**
     * @brief 添加新的转码任务
     * @param src 源RTMP流地址
     * @param dest 目标HLS路径
     * @return 成功返回0，失败返回-1
     */
    int add_task(std::string src, std::string dest)
    {
        if (src.empty() || dest.empty())
        {
            m_errmsg = "failed. parameter is empty.";
            return -1;
        }

        // 检查任务是否已存在
        if (m_taskMap.find(dest) != m_taskMap.end())
        {
            m_errmsg = "failed." + dest + " exists.";
            return -1;
        }

        // 创建并初始化新任务
        IngestTask* ptask = new IngestTask();
        ptask->init(src, dest);
        m_taskMap[dest] = ptask;
        ptask->start();
        return 0;
    }

    /**
     * @brief 删除指定的转码任务
     * @param dest 目标HLS路径
     * @return 成功返回0，失败返回-1
     */
    int del_task(std::string dest)
    {
        auto iter = m_taskMap.find(dest);
        if (iter == m_taskMap.end())
        {
            m_errmsg = "dest not found. " + dest;
            return -1;
        }

        auto ptask = iter->second;
        ptask->stop();
        m_taskMap.erase(iter);
        delete ptask;
        return 0;
    }

    // 获取任务列表
    const decltype(m_taskMap)& get_task_list()
    {
        return m_taskMap;
    }

    /**
     * @brief 获取指定任务的HLS播放地址
     * @param dest 目标HLS路径
     * @return HLS播放地址，如果任务不存在则返回空字符串
     */
    std::string get_hls_path(const std::string &dest)
    {
        auto iter = m_taskMap.find(dest);
        if (iter == m_taskMap.end())
            return "";

        auto ptask = iter->second;
        if (ptask)
            return ptask->hls;
        else
            return "";
    }

    // 获取错误信息
    std::string get_errmsg() const
    {
        return m_errmsg;
    }

    // 任务控制接口
    int startAll() // 启动所有任务
    {
        for (auto &item : m_taskMap)
        {
            auto task = item.second;
            task->start();
        }
        return 0;
    }

    int start(std::string src, std::string dest); // 启动指定任务

    /**
     * @brief 快速停止指定任务
     * @param dest 目标HLS路径
     * @return 成功返回0
     */
    int fast(std::string dest)
    {
        auto iter = m_taskMap.find(dest);
        if (iter != m_taskMap.end())
            return 0;

        auto ptask = iter->second;
        ptask->stop();
        return 0;
    }

    // 强制终止所有任务
    int fast_kill()
    {
        for (auto &item : m_taskMap)
        {
            auto ptask = item.second;
            if(ptask)
                ptask->fast_kill();
        }
        return 0;
    }

    /**
     * @brief 定期检查所有任务状态
     * @param timecnt 检查计数器
     * @return 成功返回0
     */
    int check(int timecnt);
};