#include "proxytaskmgr.h"
#include "../process/srs_app_process.hpp"

using namespace std;

// IngestTask类实现 - 负责管理FFMPEG转码任务

IngestTask::~IngestTask() {
    free(ffmpeg);
}

// 启动FFMPEG进程
int IngestTask::start() {
    return ffmpeg->start();
}

// 停止FFMPEG进程
void IngestTask::stop() {
    ffmpeg->stop();
}

// 检查FFMPEG进程状态
srs_error_t IngestTask::cycle() {
    return ffmpeg->cycle();
}

// 快速停止FFMPEG进程
void IngestTask::fast_stop() {
    ffmpeg->fast_stop();
}

// 强制终止FFMPEG进程
void IngestTask::fast_kill() {
    ffmpeg->fast_kill();
}

// 字符串替换工具函数
// instr: 输入字符串
// from: 要替换的子串
// to: 替换后的子串
std::string replaceAll(const std::string& instr, const std::string& from, const std::string& to) {
    if (from.empty())
        return instr;
    std::string str = instr;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // 处理'to'中包含'from'的情况
    }
    return str;
}

// 初始化转码任务
// src: 源RTMP流地址
// dest: 目标HLS路径
void IngestTask::init(std::string src, std::string dest) {
    this->src = src;
    this->dest = dest;

    // 创建HLS输出目录和文件路径
    std::string m3u8_dir = "./html" + dest;
    std::string m3u8 = m3u8_dir + std::string("/hls.m3u8");
    string cmd = "mkdir -p " + string(m3u8_dir);
    system(cmd.c_str());

    // 设置FFMPEG路径并确保可执行
    string ffmpeg_path = "./bin/ffmpeg";
    string cmd1 = "chmod +x " + ffmpeg_path;
    system(cmd.c_str());

    // 创建FFMPEG实例
    ffmpeg = new SrsFFMPEG(ffmpeg_path);
    // 将目标路径中的'/'替换为'_'用于日志文件名
    auto name = replaceAll(dest, "/", "_");
    string log_file = "./logs/ffmpeg" + name + ".log";
    
    // 初始化FFMPEG任务
    ffmpeg->initialize(src, m3u8, log_file);
}

// ProxytaskMgr类实现 - 负责管理所有转码任务

// 初始化任务管理器
int ProxytaskMgr::init() {
    return 0;
}

// 从数据库加载任务（预留接口）
int ProxytaskMgr::load_from_db() {
    return 0;
}

// 启动新的转码任务
int ProxytaskMgr::start(std::string src, std::string dest) {
    return add_task(src, dest);
}

// 定期检查所有任务状态
// timecnt: 检查计数器
int ProxytaskMgr::check(int timecnt) {
    // 检查SRS服务是否正常运行
    if (m_srs_process) {
        m_srs_process->cycle();  // 检查进程状态
        m_srs_process->start();  // 如果进程不存在则重启
    }

    // 检查所有FFMPEG转码任务
    for (auto &item : m_taskMap) {    
        int err = 0;
        auto task = item.second;
        
        // 检查FFMPEG状态
        if ((err = task->cycle()) != srs_success) {
            printf("ingest cycle. err:%d\n", err);
        }
        
        // 尝试重启失败的任务
        if ((err = task->start()) != srs_success) {
            printf("ingester start. err:%d\n", err);
        }
    }

    return 0;
}




