#include "srs_app_process.hpp"

#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#endif

using namespace std;

/**
 * 将字符串向量用指定分隔符连接成一个字符串
 * @param urlVec 待连接的字符串向量
 * @param delim 分隔符
 * @return 连接后的字符串
 */
std::string join_vector_string(std::vector<std::string>& urlVec, std::string delim) {
    std::string text;
    int i = 0;
    for (auto url : urlVec) {
        text.append(url);
        i++;
        if (i < urlVec.size())
            text.append(delim);
    }
    return text;
}

/**
 * 检查字符串是否以指定标志开头
 * @param str 待检查的字符串
 * @param flag 标志字符串
 * @return 如果以flag开头返回true，否则返回false
 */
bool srs_string_starts_with(string str, string flag) {
    return str.find(flag) == 0;
}

/**
 * 移除字符串开头的指定字符
 * @param str 原始字符串
 * @param trim_chars 要移除的字符集合
 * @return 处理后的字符串
 */
string srs_string_trim_start(string str, string trim_chars) {
    std::string ret = str;

    for (int i = 0; i < (int)trim_chars.length(); i++) {
        char ch = trim_chars.at(i);

        while (!ret.empty() && ret.at(0) == ch) {
            ret.erase(ret.begin());

            // 匹配成功，重置搜索
            i = -1;
        }
    }

    return ret;
}

SrsProcess::SrsProcess() {
    is_started = false;
    fast_stopped = false;
    pid = -1;
}

SrsProcess::~SrsProcess() {
}

int SrsProcess::get_pid() {
    return pid;
}

bool SrsProcess::started() {
    return is_started;
}

/**
 * 初始化进程参数
 * @param binary 可执行文件路径
 * @param argv 命令行参数数组
 * @return 成功返回srs_success，失败返回错误码
 */
srs_error_t SrsProcess::initialize(string binary, vector<string> argv) {
    srs_error_t err = srs_success;
    
    bin = binary;
    cli = "";
    
    params.clear();

    // 解析命令行参数，处理输出重定向
    for (int i = 0; i < (int)argv.size(); i++) {
        std::string ffp = argv[i];
        std::string nffp = (i < (int)argv.size() - 1) ? argv[i + 1] : "";
        std::string nnffp = (i < (int)argv.size() - 2) ? argv[i + 2] : "";

        // >file 形式的重定向
        if (srs_string_starts_with(ffp, ">")) {
            stdout_file = ffp.substr(1);
            continue;
        }

        // 1>file 形式的重定向
        if (srs_string_starts_with(ffp, "1>")) {
            stdout_file = ffp.substr(2);
            continue;
        }

        // 2>file 形式的重定向
        if (srs_string_starts_with(ffp, "2>")) {
            stderr_file = ffp.substr(2);
            continue;
        }

        // 1 >X 形式的重定向
        if (ffp == "1" && srs_string_starts_with(nffp, ">")) {
            if (nffp == ">") {
                // 1 > file
                if (!nnffp.empty()) {
                    stdout_file = nnffp;
                    i++;
                }
            }
            else {
                // 1 >file
                stdout_file = srs_string_trim_start(nffp, ">");
            }
            // 跳过 >
            i++;
            continue;
        }

        // 2 >X 形式的重定向
        if (ffp == "2" && srs_string_starts_with(nffp, ">")) {
            if (nffp == ">") {
                // 2 > file
                if (!nnffp.empty()) {
                    stderr_file = nnffp;
                    i++;
                }
            }
            else {
                // 2 >file
                stderr_file = srs_string_trim_start(nffp, ">");
            }
            // 跳过 >
            i++;
            continue;
        }

        params.push_back(ffp);
    }

    actual_cli = join_vector_string(params, " ");
    cli = join_vector_string(argv, " ");
    return err;
}

/**
 * 重定向进程输出到指定文件
 * @param from_file 目标文件路径
 * @param to_fd 要重定向的文件描述符
 * @return 成功返回srs_success，失败返回错误码
 */
srs_error_t srs_redirect_output(string from_file, int to_fd) {
    srs_error_t err = srs_success;
    
#ifndef WIN32
    // 如果未指定输出文件则使用默认输出
    if (from_file.empty()) {
        return err;
    }
    
    // 重定向文件描述符到指定文件
    int fd = -1;
    int flags = O_CREAT|O_RDWR|O_APPEND;
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;
    
    if ((fd = ::open(from_file.c_str(), flags, mode)) < 0) {
        srs_warn("open process %d %s failed", to_fd, from_file.c_str());
        return ERROR_FORK_OPEN_LOG;
    }
    
    if (dup2(fd, to_fd) < 0) {
        srs_warn("dup2 process %d failed", to_fd);
        return ERROR_FORK_DUP2_LOG;
    }
    
    ::close(fd);
#endif

    return err;
}

/**
 * 启动子进程
 * @return 成功返回srs_success，失败返回错误码
 */
srs_error_t SrsProcess::start() {
    srs_error_t err = srs_success;

    if (is_started) {
        return err;
    }

    // 生成进程参数
    srs_info("fork process: %s", cli.c_str());

#ifndef WIN32
    // 记录日志信息
    int cid = 0;// _srs_context->get_id();
    int ppid = getpid();

    // 创建子进程
    if ((pid = fork()) < 0) {
        srs_warn("vfork process failed, cli=%s", cli.c_str());
        return -1;
    }

    // 子进程处理
    if (pid == 0) {
        // 忽略SIGINT和SIGTERM信号
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);

        // 重定向标准输出
        if ((err = srs_redirect_output(stdout_file, STDOUT_FILENO)) != srs_success) {
            srs_warn("redirect output. err:%d", err);
            return err;
        }

        // 重定向标准错误
        if ((err = srs_redirect_output(stderr_file, STDERR_FILENO)) != srs_success) {
            srs_warn("redirect output. err:%d", err);
            return err;
        }

        // 重定向标准输入到/dev/null
        if ((err = srs_redirect_output("/dev/null", STDIN_FILENO)) != srs_success) {
            srs_warn("redirect output. err:%d", err);
            return err;
        }

        // 输出进程基本信息
        if (true) {
            fprintf(stdout, "\n");
            fprintf(stdout, "process ppid=%d, cid=%d, pid=%d, in=%d, out=%d, err=%d\n",
                ppid, cid, getpid(), STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);
            fprintf(stdout, "process binary=%s, cli: %s\n", bin.c_str(), cli.c_str());
            fprintf(stdout, "process actual cli: %s\n", actual_cli.c_str());
        }

        // 准备execv参数
        char** argv = new char*[params.size() + 1];
        for (int i = 0; i < (int)params.size(); i++) {
            std::string& p = params[i];
            char* v = new char[p.length() + 1];
            argv[i] = strcpy(v, p.data());
        }
        argv[params.size()] = NULL;

        // 执行新程序
        int r0 = execv(bin.c_str(), argv);
        if (r0 < 0) {
            srs_warn("fork process failed, errno=%d(%s)", errno, strerror(errno));
        }
        exit(r0);
    }

    // 父进程处理
    if (pid > 0) {
        // 等待子进程启动
        srs_usleep(00 * SRS_UTIME_MILLISECONDS);

        int status = 0;
        pid_t p = waitpid(pid, &status, WNOHANG);
        
        // 检查子进程是否启动失败
        if ((WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
            printf("child process terminated. exit status is %d\n", WEXITSTATUS(status));
            return -1;
        }

        is_started = true;
        srs_trace("forked process, pid=%d, bin=%s, stdout=%s, stderr=%s, argv=%s",
                  pid, bin.c_str(), stdout_file.c_str(), stderr_file.c_str(), actual_cli.c_str());
        return err;
    }
    
#endif

    return err;
}

/**
 * 检查进程状态
 * @return 成功返回srs_success，失败返回错误码
 */
srs_error_t SrsProcess::cycle() {
    srs_error_t err = srs_success;
    
    if (!is_started) {
        return err;
    }
    
    // 如果进程准备停止，不再检查
    if (fast_stopped) {
        return err;
    }

#ifndef WIN32
    int status = 0;
    pid_t p = waitpid(pid, &status, WNOHANG);
    
    if (p < 0) {
        srs_warn("process waitpid failed, pid=%d", pid);
        return ERROR_SYSTEM_WAITPID;
    }
    
    if (p == 0) {
        return err;
    }
    
    srs_trace("process pid=%d terminate, please restart it.", pid);
    is_started = false;
#endif

    return err;
}

/**
 * 停止进程
 * 先发送SIGTERM信号，然后发送SIGKILL信号确保进程停止
 */
void SrsProcess::stop() {
    if (!is_started) {
        return;
    }
    
#ifndef WIN32 
    srs_error_t err = SrsUtil::srs_kill_forced(pid);
    if (err != srs_success) {
        srs_warn("ignore kill the process failed, pid=%d. err=", pid);
        return;
    }
#endif

    is_started = false;
}

/**
 * 快速停止进程
 * 仅发送SIGTERM信号
 */
void SrsProcess::fast_stop() {
    int ret = 0;
    
    if (!is_started) {
        return;
    }
    
    if (pid <= 0) {
        return;
    }

#ifndef WIN32   
    if (kill(pid, SIGTERM) < 0) {
        ret = ERROR_SYSTEM_KILL;
        srs_warn("ignore fast stop process failed, pid=%d. ret=%d", pid, ret);
        return;
    }
#endif

    return;
}

/**
 * 强制终止进程
 * 直接发送SIGKILL信号
 */
void SrsProcess::fast_kill() {
    int ret = 0;

#ifndef WIN32
    if (!is_started) {
        return;
    }

    if (pid <= 0) {
        return;
    }

    if (kill(pid, SIGKILL) < 0) {
        ret = ERROR_SYSTEM_KILL;
        srs_warn("ignore fast kill process failed, pid=%d. ret=%d", pid, ret);
        return;
    }

    // 等待进程退出以避免僵尸进程
    int status = 0;
    waitpid(pid, &status, WNOHANG);
#endif

    return;
}

/**
 * 强制终止进程
 * @param pid 进程ID
 * @return 成功返回srs_success，失败返回错误码
 */
srs_error_t SrsUtil::srs_kill_forced(int& pid) {
    srs_error_t err = srs_success;

    if (pid <= 0) {
        return err;
    }

#ifndef WIN32
    // 先尝试SIGTERM信号
    if (kill(pid, SIGTERM) < 0) {
        srs_warn("kill failed.");
        return ERROR_SYSTEM_KILL;
    }

    // 等待进程退出
    srs_trace("send SIGTERM to pid=%d", pid);
    const int SRS_PROCESS_QUIT_TIMEOUT_MS = 1000;
    for (int i = 0; i < SRS_PROCESS_QUIT_TIMEOUT_MS / 10; i++) {
        int status = 0;
        pid_t qpid = -1;
        if ((qpid = waitpid(pid, &status, WNOHANG)) < 0) {
            srs_warn("kill failed.");
            return ERROR_SYSTEM_KILL;
        }

        // 进程未退出则继续等待
        if (qpid == 0) {
            srs_usleep(10 * 1000);
            continue;
        }

        // 进程已退出
        srs_trace("SIGTERM stop process pid=%d ok.", pid);
        pid = -1;

        return err;
    }

    // 如果SIGTERM无效，使用SIGKILL信号
    if (kill(pid, SIGKILL) < 0) {
        srs_warn("kill failed.");
        return ERROR_SYSTEM_KILL;
    }

    // 等待进程退出
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        srs_usleep(10 * 1000);
        continue;
    }

#endif

    srs_trace("SIGKILL stop process pid=%d ok.", pid);
    pid = -1;

    return err;
}