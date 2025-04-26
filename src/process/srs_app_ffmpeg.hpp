#pragma once

#include "../common/srs_common.h"

#include <vector>
#include <string>

class SrsConfDirective;
class SrsPithyPrint;
class SrsProcess;

/**
 * @class SrsFFMPEG
 * @brief 封装FFmpeg进程管理，用于处理RTMP流转HLS流的转码工作
 * 
 * 该类负责管理FFmpeg进程的生命周期，包括启动、停止和参数配置。
 * 主要用于将RTMP流转换为HLS流，支持设置输入输出路径、日志文件等参数。
 */
class SrsFFMPEG
{
private:
    SrsProcess* process;        ///< FFmpeg进程管理对象
    std::vector<std::string> params;  ///< FFmpeg命令行参数列表
    std::string log_file;      ///< FFmpeg日志文件路径
    
    std::string ffmpeg;        ///< FFmpeg可执行文件路径
    std::vector<std::string> iparams;  ///< 输入参数列表
    std::vector<std::string> perfile;  ///< 每个文件的参数列表
    std::string iformat;       ///< 输入格式
    std::string _input;        ///< 输入URL或文件路径
    std::string _output;       ///< 输出URL或文件路径

public:
    /**
     * @brief 构造函数
     * @param ffmpeg_bin FFmpeg可执行文件的路径
     */
    SrsFFMPEG(std::string ffmpeg_bin);
    
    /**
     * @brief 析构函数，负责清理资源
     */
    virtual ~SrsFFMPEG();
    
    /**
     * @brief 添加输入参数
     * @param iparam 要添加的输入参数
     */
    virtual void append_iparam(std::string iparam);
    
    /**
     * @brief 获取输出路径
     * @return 返回当前设置的输出路径
     */
    virtual std::string output();
    
    /**
     * @brief 初始化FFmpeg参数
     * @param in 输入URL或文件路径
     * @param out 输出URL或文件路径
     * @param log 日志文件路径
     * @return 成功返回srs_success，否则返回具体错误码
     */
    virtual srs_error_t initialize(std::string in, std::string out, std::string log);
    
    /**
     * @brief 启动FFmpeg进程
     * @return 成功返回srs_success，否则返回具体错误码
     */
    virtual srs_error_t start();
    
    /**
     * @brief 循环检查FFmpeg进程状态
     * @return 成功返回srs_success，否则返回具体错误码
     */
    virtual srs_error_t cycle();
    
    /**
     * @brief 停止FFmpeg进程
     * 会等待进程正常退出
     */
    virtual void stop();
    
    /**
     * @brief 快速停止FFmpeg进程
     * 发送SIGTERM信号，不等待进程退出
     */
    virtual void fast_stop();
    
    /**
     * @brief 强制终止FFmpeg进程
     * 发送SIGKILL信号立即终止进程
     */
    virtual void fast_kill();
};




