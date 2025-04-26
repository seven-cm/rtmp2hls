#pragma once
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

/**
 * @brief 通用定时器类，支持周期性任务执行和定时任务调度
 * 
 * Timer类提供了以下功能：
 * - 周期性执行指定任务
 * - 支持同步和异步定时任务
 * - 安全的任务取消机制
 * - 线程安全的实现
 */
class Timer {
public:
    /**
     * @brief 默认构造函数
     * 初始化定时器为过期状态，未尝试过期
     */
    Timer() : expired_(true), try_to_expire_(false) {}

    /**
     * @brief 拷贝构造函数
     * @param t 源定时器对象
     */
    Timer(const Timer& t) {
        expired_ = t.expired_.load();
        try_to_expire_ = t.try_to_expire_.load();
    }

    /**
     * @brief 析构函数
     * 确保定时器被正确停止
     */
    ~Timer() {
        Expire();
    }

    /**
     * @brief 启动定时器，周期性执行指定任务
     * @param interval 任务执行间隔(毫秒)
     * @param task 需要执行的任务函数
     */
    void StartTimer(int interval, std::function<void()> task) {
        if (expired_ == false) {
            return;
        }
        expired_ = false;
        std::thread([this, interval, task]() {
            while (!try_to_expire_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                task();
            }
            {
                std::lock_guard<std::mutex> locker(mutex_);
                expired_ = true;
                expired_cond_.notify_one();
            }
        }).detach();
    }

    /**
     * @brief 停止定时器
     * 安全地终止正在执行的定时任务
     */
    void Expire() {
        if (expired_) {
            return;
        }

        if (try_to_expire_) {
            return;
        }
        try_to_expire_ = true;
        {
            std::unique_lock<std::mutex> locker(mutex_);
            expired_cond_.wait(locker, [this] { return expired_ == true; });
            if (expired_ == true) {
                try_to_expire_ = false;
            }
        }
    }

    /**
     * @brief 同步等待执行任务
     * @param after 延迟执行时间(毫秒)
     * @param f 待执行的函数
     * @param args 函数参数
     */
    template<typename callable, class... arguments>
    void SyncWait(int after, callable&& f, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task
            (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
        std::this_thread::sleep_for(std::chrono::milliseconds(after));
        task();
    }

    /**
     * @brief 异步等待执行任务
     * @param after 延迟执行时间(毫秒)
     * @param f 待执行的函数
     * @param args 函数参数
     */
    template<typename callable, class... arguments>
    void AsyncWait(int after, callable&& f, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task
            (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

        std::thread([after, task]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(after));
            task();
        }).detach();
    }

private:
    std::atomic<bool> expired_;        ///< 定时器是否已过期
    std::atomic<bool> try_to_expire_;  ///< 是否正在尝试使定时器过期
    std::mutex mutex_;                 ///< 互斥锁，用于线程同步
    std::condition_variable expired_cond_; ///< 条件变量，用于等待定时器过期
};
