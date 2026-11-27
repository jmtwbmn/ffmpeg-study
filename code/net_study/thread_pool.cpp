// ============================================================
// S3 · 线程池  thread_pool.cpp
// 编译: g++ -std=c++17 -pthread thread_pool.cpp -o thread_pool
// 运行: ./thread_pool
// ============================================================
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <set>          // 验收②：收集 thread::id
#include <chrono>       // 附加实验：计时

class ThreadPool {
public:
    explicit ThreadPool(size_t n);
    ~ThreadPool();
    void submit(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cond;
    bool stop = false;
};

// ------------------------------------------------------------
// 构造：起 n 个 worker，每个 worker 跑一个「取任务 → 解锁 → 执行」的死循环
// 思考点 1：循环体长什么样？
//   - 加锁
//   - 等一个条件（谓词怎么写？）
//   - 醒来后判断是不是该退出
//   - 取任务、从队列里 pop 掉
//   - 锁在哪一行之前就该放开？为什么？（提示：执行任务的时候还拿着锁会怎样）
//   - 执行完回到循环开头
// 陷阱：workers.reserve(n) —— 不 reserve 会怎样？
// ------------------------------------------------------------
ThreadPool::ThreadPool(size_t n) {
    // TODO: for (size_t i = 0; i < n; ++i) workers.emplace_back([this] { ... });
    for(size_t i=0;i<n;i++)
    {
        workers.emplace_back([this]{
            std::unique_lock<std::mutex> lock(mtx);
            {
            cond.wait(lock,[&]{return !tasks.empty() && stop});
            if(task.empty() && !stop)
            break;
            workers[i] = tasks.front();
            tasks.pop();
            }
            //回到开头
        });
    }
}

// ------------------------------------------------------------
// 析构：优雅退出
// 思考点 3（面试高频）：为什么顺序必须是
//      ① stop = true   ② cond.notify_all()   ③ 逐个 join
//   调换 ① 和 ② 会怎样？只 join 不 notify 会怎样？
//   为什么这里必须是 notify_all 而不是 notify_one？
// ------------------------------------------------------------
ThreadPool::~ThreadPool() {
    // TODO
    while(tasks.empty() | !stop)
    {   
        int i=0;
        std::lock_guard<std::mutex> lock(mtx);
        {
            stop = true;
            cond.notify_all();
            worker[i].join();
        }
        i++;
    }


}

// ------------------------------------------------------------
// 提交任务
// 思考点 4：为什么参数是 std::function<void()> 而不是 void(*)()？
//   （想想你以后要 submit([&]{ ... }) 这种带捕获的 lambda）
// 注意锁的粒度：push 完就该放手，notify 不需要持锁
// ------------------------------------------------------------
void ThreadPool::submit(std::function<void()> task) {
    // TODO: 加锁 → push → 解锁 → notify_one（零参数！）
    std::lock_guard<std::mutex> lock(mtx);
    {
        tasks.push(task);
    }
    cond.notify_one();

}

int main() {
    // ---------------- 验收 ① + ② ----------------
    // 起 n=4，扔 1000 个任务，每个任务只做两件事：
    //   1) 原子或加锁地把全局计数 +1
    //   2) 把自己的 std::this_thread::get_id() 塞进 std::set<std::thread::id>
    // 期望：计数 == 1000，set.size() == 4
    //
    // 提示：计数用 std::atomic<int> 最省事，想练锁就再套一层 mutex（但别死锁）

    size_t n = 4;
    ThreadPool thread1();
    thread1.submit(1000);

    ThreadPoll thread2();
    ThreadPoll thread3();
    ThreadPoll thread4();
    


    // ---------------- 附加实验 ----------------
    // n=1 和 n=4 各跑一遍：1000 个 sleep_for(1ms) 的任务
    // 用 std::chrono::steady_clock 卡时间，期望 ≈1s vs ≈250ms
    // 思考：为什么不是严格 1000ms / 250ms？（调度、唤醒开销）

    return 0;
}
