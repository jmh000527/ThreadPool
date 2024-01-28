#ifndef THREADPOOL_H
#define THREADPOOL_H


#include "thread.h"

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Any类型：可以接收任意数据的类型
class Any {
public:
    Any() = default;
    ~Any() = default;

    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;

    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    // 这个构造函数可以让Any类型接收任意其它的数据
    template<typename T> // T:int    Derive<int>
    Any(T data)
        : base_(std::make_unique<Derive<T>>(data)) {}

    // 这个方法能把Any对象里面存储的data数据提取出来
    template<typename T>
    T cast_() {
        // 我们怎么从base_找到它所指向的Derive对象，从它里面取出data成员变量
        // 基类指针 =》 派生类指针   RTTI
        Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
        if (pd == nullptr) {
            throw "type is unmatch!";
        }
        return pd->data_;
    }

private:
    // 基类类型
    class Base {
    public:
        virtual ~Base() = default;
    };

    // 派生类类型
    template<typename T>
    class Derive : public Base {
    public:
        Derive(T data)
            : data_(data) {}

        T data_; // 保存了任意的其它类型
    };

    // 定义一个基类的指针
    std::unique_ptr<Base> base_;
};

// 实现一个信号量类
class Semaphore {
public:
    Semaphore(int limit = 0)
        : resLimit_(limit) {}

    ~Semaphore() = default;

    // 获取一个信号量资源
    void wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待信号量有资源，没有资源的话，会阻塞当前线程
        cond_.wait(lock, [&]()-> bool {
            return resLimit_ > 0;
        });
        resLimit_--;
    }

    // 增加一个信号量资源
    void post() {
        std::unique_lock<std::mutex> lock(mtx_);
        resLimit_++;
        // linux下condition_variable的析构函数什么也没做
        // 导致这里状态已经失效，无故阻塞
        cond_.notify_all(); // 等待状态，释放mutex锁 通知条件变量wait的地方，可以起来干活了
    }

private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

// Task类型的前置声明
class Task;

// 实现接收提交到线程池的task任务执行完成后的返回值类型Result
class Result {
public:
    Result(std::shared_ptr<Task> task, bool isValid = true);
    ~Result() = default;

    // 问题一：setVal方法，获取任务执行完的返回值的
    void setVal(Any any);

    // 问题二：get方法，用户调用这个方法获取task的返回值
    Any get();

private:
    Any any_;                    // 存储任务的返回值
    Semaphore sem_;              // 线程通信信号量
    std::shared_ptr<Task> task_; //指向对应获取返回值的任务对象
    std::atomic_bool isValid_;   // 返回值是否有效
};

// 任务抽象基类
class Task {
public:
    Task();
    ~Task() = default;
    void exec();
    void setResult(Result* res);

    // 用户可以自定义任意任务类型，从Task继承，重写run方法，实现自定义任务处理
    virtual Any run() = 0;

private:
    Result* result_; // Result对象的声明周期 》 Task的
};


// 线程池工作模式
enum class PoolMode {
    // 固定线程数
    FIXED,
    // 动态增长线程数
    CACHED,
};


class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;


    void start(size_t initThreadSize = std::thread::hardware_concurrency()); // 开启线程池
    void setMode(PoolMode mode);                                             // 设置线程池工作模式
    void setInitThreadSize(size_t size);                                     // 设置初始线程数量
    void setTaskQueueMaxThreshold(size_t threshold);                         // 设置任务队列任务数量阈值上限
    Result submitTask(const std::shared_ptr<Task>& pTask);                   // 向线程池提交任务

private:
    void threadFunc(); // 定义线程函数

private:
    std::vector<std::unique_ptr<Thread>> m_threads; // 线程列表
    size_t m_initThreadSize;                        // 初始线程数量

    std::queue<std::shared_ptr<Task>> m_taskQueue; // 任务队列
    std::atomic<size_t> m_taskSize;                // 任务数量
    size_t m_taskSizeMaxThreshold;                 // 任务队列数量的阈值上限

    std::mutex m_taskQueueMtx;          // 保证任务队列线程安全
    std::condition_variable m_notFull;  // 表示任务队列不满
    std::condition_variable m_notEmpty; // 表示任务队列不空

    PoolMode m_poolMode; // 当前线程池工作模式
};


#endif //THREADPOOL_H
