
#include "srs_app_ffmpeg.hpp"
#include "srs_app_process.hpp"

#include <stdlib.h>

#ifndef WIN32
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif


#include <vector>
using namespace std;




/**
 * @brief 构造函数，初始化FFmpeg实例
 * @param ffmpeg_bin FFmpeg可执行文件的路径
 */
SrsFFMPEG::SrsFFMPEG(std::string ffmpeg_bin)
{
    ffmpeg = ffmpeg_bin;
    process = new SrsProcess();
}

/**
 * @brief 析构函数，负责清理资源
 * 在销毁对象前会先停止FFmpeg进程，然后释放进程管理对象
 */
SrsFFMPEG::~SrsFFMPEG()
{
    stop();
    
    free(process);
}

/**
 * @brief 添加FFmpeg输入参数
 * @param iparam 要添加的输入参数字符串
 */
void SrsFFMPEG::append_iparam(string iparam)
{
    iparams.push_back(iparam);
}


/**
 * @brief 获取当前设置的输出路径
 * @return 返回输出文件或URL的路径
 */
string SrsFFMPEG::output()
{
    return _output;
}

/**
 * @brief 初始化FFmpeg转码参数
 * @param in 输入流URL或文件路径
 * @param out 输出流URL或文件路径
 * @param log 日志文件路径
 * @return 成功返回srs_success，失败返回错误码
 */
srs_error_t SrsFFMPEG::initialize(string in, string out, string log)
{
    srs_error_t err = srs_success;
    
    _input = in;
    _output = out;
    log_file = log;
    
    return err;
}



/**
 * @brief 启动FFmpeg进程进行流转换
 * 
 * 该函数负责：
 * 1. 检查进程是否已启动
 * 2. 构建FFmpeg命令行参数
 * 3. 配置输入输出格式
 * 4. 设置HLS相关参数
 * 5. 配置日志输出
 * 6. 初始化并启动进程
 * 
 * @return 成功返回srs_success，失败返回错误码
 */
srs_error_t SrsFFMPEG::start()
{
    srs_error_t err = srs_success;
    
    if (process->started()) {
        return err;
    }
    
    // 清空参数列表
    params.clear();
    
    // 设置FFmpeg可执行文件路径作为第一个参数
    params.push_back(ffmpeg);
    
    // 设置日志级别
    params.push_back("-loglevel");
    params.push_back("warning");

    // 配置输入参数
    params.push_back("-f");
    params.push_back("flv");
    params.push_back("-i");
    params.push_back(_input);

    // 配置视频和音频编码参数
    params.push_back("-vcodec");
    params.push_back("copy");
    params.push_back("-acodec");
    params.push_back("copy");

    // 配置HLS输出参数
    params.push_back("-f");
    params.push_back("hls");
    params.push_back("-hls_flags");
    params.push_back("delete_segments");
    params.push_back("-segment_list_size");
    params.push_back("8");
    params.push_back("-hls_list_size");
    params.push_back("5");
    
    // 设置输出路径
    params.push_back(_output);

    // 配置日志输出
    if (!log_file.empty()) {
        // 重定向标准输出到日志文件
        params.push_back("1");
        params.push_back(">");
        params.push_back(log_file);
        // 重定向标准错误到日志文件
        params.push_back("2");
        params.push_back(">");
        params.push_back(log_file);
    }
    
    // initialize the process.
    if ((err = process->initialize(ffmpeg, params)) != srs_success) {
        return srs_error_wrap(err, "init process");
    }
    
    return process->start();
}

/**
 * @brief 循环检查FFmpeg进程状态
 * @return 成功返回srs_success，失败返回错误码
 */
srs_error_t SrsFFMPEG::cycle()
{
    return process->cycle();
}

/**
 * @brief 停止FFmpeg进程
 * 该方法会等待进程正常退出
 */
void SrsFFMPEG::stop()
{
    process->stop();
}

/**
 * @brief 快速停止FFmpeg进程
 * 发送SIGTERM信号给进程，不等待其退出
 */
void SrsFFMPEG::fast_stop()
{
    process->fast_stop();
}

/**
 * @brief 强制终止FFmpeg进程
 * 发送SIGKILL信号立即终止进程
 */
void SrsFFMPEG::fast_kill()
{
    process->fast_kill();
}