#include <iostream>

#include "threadpool.h"

using uLong = unsigned long long;

class MyTask : public Task {
public:
    MyTask(const int begin, const int end);

    Any run() override;

private:
    int m_begin;
    int m_end;
};

MyTask::MyTask(const int begin, const int end)
    : m_begin{ begin }, m_end{ end } {}

Any MyTask::run() {

    // std::this_thread::sleep_for(std::chrono::seconds(5));
    uLong sum{};
    for (uLong i{ static_cast<uLong>(m_begin) }; i <= m_end; ++i) {
        sum += i;
    }

    return sum;
}

int main() {
    ThreadPool threadPool;
    threadPool.start(8);

    Result res1 = threadPool.submitTask(std::make_shared<MyTask>(1, 100000000));
    Result res2 = threadPool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
    Result res3 = threadPool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

    // 随着task被执行完，task对象没了，依赖于task对象的Result对象也没了
    uLong sum1 = res1.get().cast_<uLong>(); // get返回了一个Any类型，怎么转成具体的类型呢？
    uLong sum2 = res2.get().cast_<uLong>();
    uLong sum3 = res3.get().cast_<uLong>();

    // Master-Slave线程模型
    // Master线程用来分解任务，然后给各个Slave线程分配任务
    // 等待各个Slave线程执行完任务，返回结果
    // Master线程合并各个任务结果，输出

    std::cout << (sum1 + sum2 + sum3) << std::endl;

    //验证多线程计算结果
    uLong sum = 0;
    for (uLong i = 1; i <= 300000000; i++)
        sum += i;
    std::cout << sum << std::endl;

    getchar();
}
