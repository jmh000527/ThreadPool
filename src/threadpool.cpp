#include "threadpool.h"

#include <functional>
#include <iostream>
#include <thread>

/////////////////  Task方法实现
Task::Task()
    : result_(nullptr) {}

void Task::exec() {
    if (result_ != nullptr) {
        result_->setVal(run()); // 这里发生多态调用
    }
}

void Task::setResult(Result* res) {
    result_ = res;
}

/////////////////   Result方法的实现
Result::Result(std::shared_ptr<Task> task, bool isValid)
    : isValid_(isValid), task_(std::move(task)) {
    task_->setResult(this);
}

Any Result::get() // 用户调用的
{
    if (!isValid_) {
        return "";
    }
    sem_.wait(); // task任务如果没有执行完，这里会阻塞用户的线程
    return std::move(any_);
}

void Result::setVal(Any any) // 谁调用的呢？？？
{
    // 存储task的返回值
    this->any_ = std::move(any);
    sem_.post(); // 已经获取的任务的返回值，增加信号量资源
}

constexpr int TASK_MAX_THRESHOLD = 512;

ThreadPool::ThreadPool()
    : m_initThreadSize{ std::thread::hardware_concurrency() }, m_taskSize{ 0 },
      m_taskSizeMaxThreshold{ TASK_MAX_THRESHOLD },
      m_poolMode{ PoolMode::FIXED } {}

ThreadPool::~ThreadPool() = default;

void ThreadPool::start(const size_t initThreadSize) {
    //设置初始线程数量
    setInitThreadSize(initThreadSize);

    //创建线程数量
    for (size_t i{}; i < m_initThreadSize; i++) {
        //创建Thread线程对象时，把线程函数给到Thread对象
        auto ptr{ std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this)) };
        m_threads.emplace_back(std::move(ptr));
    }

    //启动所有线程
    for (size_t i{}; i < m_initThreadSize; i++) {
        m_threads.at(i)->start();
    }
}

void ThreadPool::setMode(const PoolMode mode) {
    m_poolMode = mode;
}

void ThreadPool::setInitThreadSize(const size_t size) {
    m_initThreadSize = size;
}

void ThreadPool::setTaskQueueMaxThreshold(const size_t threshold) {
    m_taskSizeMaxThreshold = threshold;
}

Result ThreadPool::submitTask(const std::shared_ptr<Task>& pTask) {
    std::unique_lock uniqueLock{ m_taskQueueMtx };

    // 线程的通信  等待任务队列有空余
    // 用户提交任务，最长不能阻塞超过1s，否则判断提交任务失败，返回
    if (!m_notFull.wait_for(uniqueLock, std::chrono::seconds(1), [&]()-> bool {
        return m_taskQueue.size() < m_taskSizeMaxThreshold;
    })) {
        std::cerr << "Task queue is full, submit task failed." << std::endl;
        return Result{ pTask, false };
    }

    // 如果有空余，把任务放入任务队列中
    m_taskQueue.emplace(pTask);
    ++m_taskSize;

    // 因为新放了任务，任务队列肯定不空了，在notEmpty_上进行通知，赶快分配线程执行任务
    m_notEmpty.notify_all();
    return Result{ pTask };
}

void ThreadPool::threadFunc() {
    for (;;) {
        std::shared_ptr<Task> pTask;
        {
            std::unique_lock uniqueLock{ m_taskQueueMtx };

            // std::cout << "尝试获取任务：" << std::this_thread::get_id() << std::endl;

            // cached模式下，有可能已经创建了很多的线程，但是空闲时间超过60s，应该把多余的线程
            // 结束回收掉（超过initThreadSize_数量的线程要进行回收）
            // 当前时间 - 上一次线程执行的时间 > 60s

            m_notEmpty.wait(uniqueLock, [&]()-> bool {
                return !m_taskQueue.empty();
            });

            // std::cout << "获取任务成功：" << std::this_thread::get_id() << std::endl;

            pTask = m_taskQueue.front();
            m_taskQueue.pop();
            --m_taskSize;

            // 任务队列中依然有任务，通知其他线程执行
            if (!m_taskQueue.empty()) {
                m_notEmpty.notify_all();
            }

            // 取出一个任务，进行通知，可以继续提交生产任务
            m_notFull.notify_all();
        }

        if (pTask != nullptr) {
            // pTask->run();
            pTask->exec(); //通过exec()调用run()
        }
    }
}
