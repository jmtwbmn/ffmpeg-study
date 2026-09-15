


                //线程安全模板

#include<queue>
#include<mutex>
#include<condition_variable>
#include<utility>   //std::move 转移所有权

//模板类 T表示队列里面存储的数据类型
template <typename T>
class ThreadSafeQueue
{

    public:
    //============================生产者调用
    //通常由解复用线程(demux)或者解码线程(decoder)调用将数据入队
    void push(T new_value)
    {
        //RALL管理，无需手动解锁，离开作用域自动解锁
        std::lock_guard<std::mutex> lock(mtx);

        //std::move将new_value的所有权转移到队列，避免深拷贝，提高性能,移动之后，外面的new_value变成为定义状态
        data_queue.push(std::move(new_value));

        //通知消费者线程查看队列状态,notify_one()是唤醒一个等待的消费者线程
        cond_var.notify_one();

    }

    //clear清空队列
    void clear()
    {
        std::lock_guard<std::mutex>lock(mtx);
        std::queue<T> empty;
        std::swap(data_queue,empty);
    }

    //关闭播放器
    void close()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(m_isstoped == true)
        {
            cond_var.notify_all();
            //唤醒所有消费者程序结束
        }
    }

    //==============================消费者调用
    //从队列取数据，如果队列为空，阻塞等待有数据进入
    bool pop(T& value){

        //消费者线程需要随时解锁又加锁，lock_guard没有中途解锁的能力，所以用unique_lock
        std::unique_lock<std::mutex>lock(mtx);

        //lambda函数在线程被唤醒的时候执行，如果队列为空则继续等待
        cond_var.wait(lock,[this]{
            //如果不为空，或者停止条件为true则唤醒
            return !data_queue.empty() || m_isstoped;
        });

        //如果发现是停止播放器而唤醒则直接返回false
        if(m_isstoped)
        return false;


        //通过Move取出队首元素，然后队首为空，pop释放内存
        value = std::move(data_queue.front());
        data_queue.pop();

        return true;
    }

    //-----------------------状态查询

    //检查队列是否为空
    bool empty() const{
        std::lock_guard<std::mutex>lock(mtx);
        return data_queue.empty();
    }

    private:
    //mutable关键字可以让const修饰的变量也可以被修改,这里让empty加const修饰仍可以加锁
    mutable std::mutex mtx;

    std::condition_variable cond_var;

    std::queue<T> data_queue;

    bool m_isstoped = true;

};