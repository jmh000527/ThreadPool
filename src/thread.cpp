#include "thread.h"

#include <thread>
#include <utility>

Thread::Thread(ThreadFunc threadFunc)
    : m_threadFunc{ std::move(threadFunc) } {}

Thread::~Thread() {}

void Thread::start() {
    //创建一个线程来执行线程函数
    std::thread t{ m_threadFunc };

    //分离线程
    t.detach();
}
