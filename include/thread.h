#ifndef THREAD_H
#define THREAD_H

#include <functional>

class Thread {
public:
    //线程函数对象类型
    using ThreadFunc = std::function<void()>;

    Thread(ThreadFunc threadFunc);
    ~Thread();

    void start(); //启动线程

private:
    ThreadFunc m_threadFunc;
};


#endif //THREAD_H
