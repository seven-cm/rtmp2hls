
#include <cstdio>
#include "http/httplib.h"
#include <iostream>
#include "utils/timer.hpp"
#include "core/proxytaskmgr.h"
#include "utils/csv.hpp"
#include "common/logger.h"

using namespace httplib;
using namespace std;

#ifndef WIN32
#include <signal.h>

// 信号处理函数，用于处理SIGINT和SIGTERM信号
void signalDeal(int sig) {
    // 处理Ctrl+C (SIGINT)和终止信号(SIGTERM)
    if (sig == SIGINT || sig == SIGTERM) {
        ProxytaskMgr::getinstance().fast_kill();
        printf("receive signal %d, and kill sub process.\n", sig);
        exit(0);
    } else {
        printf("receive signal %d. but ignore.\n", sig);
    }
}
#endif

// 生成HTTP请求日志
string log(const Request &req, const Response &res) {
    string s;
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), "%s %s %s\n", 
             req.method.c_str(),
             req.version.c_str(), 
             req.path.c_str());
    s += buf;
    return s;
}

// 定时器计数器
static int timer_cnt = 0;

// 定时检查函数，每3次检查一次代理任务状态
void check() {
    timer_cnt++;
    if (timer_cnt % 3 == 0) {
        ProxytaskMgr::getinstance().check(timer_cnt);
    }
}

// CSV文件路径常量
const string CSV_FILE = "tasks.csv";

// 从CSV文件加载任务配置
map<string, string> load_task_from_csv() {
    map<string, string> taskmap;
    try {
        // 设置CSV格式，去除空格和制表符
        csv::CSVFormat format;
        format.trim({ ' ', '\t' });
        
        // 读取CSV文件
        csv::CSVReader reader(CSV_FILE, format);
        csv::CSVRow row;
        
        // 遍历每一行数据
        while (reader.read_row(row)) {
            if (!row["dest"].is_null() && !row["src"].is_null()) {
                auto dest = row["dest"].get();
                auto src = row["src"].get();
                taskmap[src] = dest;
            }
        }
    } catch (exception& ex) {
        printf("exception:%s\n", ex.what());
    }
    return taskmap;
}

int main(int argc, const char **argv) {
    #ifndef WIN32
    // 注册信号处理函数
    signal(SIGINT, signalDeal);   // 注册SIGINT信号处理
    signal(SIGTERM, signalDeal);  // 注册SIGTERM信号处理
    #endif

    // 创建HTTP服务器实例
    httplib::Server svr;

    // 设置错误处理器
    svr.set_error_handler([](const Request & /*req*/, Response &res) {
        const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html");
    });

    // 设置日志处理器
    svr.set_logger([](const Request &req, const Response &res) {
        auto logger = MyLogger::getLogger("http");
        LOG_INFO(logger, log(req, res));
    });

    // 设置服务器端口，默认8086
    auto port = 8086;
    if (argc > 1) { 
        port = atoi(argv[1]); 
    }

    // 设置基础目录，默认./html
    auto base_dir = "./html";
    if (argc > 2) { 
        base_dir = argv[2]; 
    }

    // 创建必要的目录
    string cmd = "mkdir  -p " + string(base_dir);
    system(cmd.c_str());
    system("mkdir -p logs");

    // 设置静态文件挂载点
    if (!svr.set_mount_point("/", base_dir)) {
        cout << "The specified base directory doesn't exist...";
        return 1;
    }

    // 从CSV文件加载任务并添加到任务管理器
    map<string, string> taskmap = load_task_from_csv();
    for (auto item : taskmap) {
        auto src = item.first;
        auto dest = item.second;
        ProxytaskMgr::getinstance().add_task(src, dest);
    }

    // 启动定时器，定期检查任务状态
    Timer m_timer;
    m_timer.StartTimer(1000, std::bind(check));

    // 启动服务器
    cout << "The server started at port " << port << endl;
    svr.listen("0.0.0.0", port);

    return 0;
}
