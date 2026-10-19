#pragma once
#include<thread>
#include<memory>
#include<condition_variable>
#include<mutex>
#include<utility>
#include<queue>


//这一块的主要任务就是写一个通用类T然后写一个生产消费者模型完成入队和出队的功能

template<typename T> 


class ThreadSafeQueue{

private:
    std::mutex mtx;
    std::condition_variable cond_var;
    std::queue<T> dataqueue;
    bool m_isStopped = false;

public:
//先写生产者
    void push( T value){
        {
            std::lock_guard<std::mutex> lock(mtx);
            dataqueue.push(std::move(value));
        }
            cond_var.notify_one();
    }

    //其次是消费者
    bool pop(T& value)
    {   
        std::unique_lock<std::mutex> lock(mtx);
        cond_var.wait(lock,[this]{
            return !dataqueue.empty()||m_isStopped;
        });
        if(m_isStopped && dataqueue.empty())
        {
            return false;
        }
        //取出然后释放空间
        value = std::move(dataqueue.front());
        dataqueue.pop();
        cond_var.notify_one();

        return true;
    }
    bool empty()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return dataqueue.empty();
    }
    void clear()
    {   
        std::lock_guard<std::mutex> lock(mtx);
        std::queue<T> empty;
        dataqueue.swap(empty);
    }

    void close()
    {
        std::lock_guard<std::mutex> lock(mtx);
        m_isStopped = true;
        cond_var.notify_all();
    }



};