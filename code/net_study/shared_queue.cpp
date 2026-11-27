#include<thread>
#include<iostream>
#include<mutex>
#include<queue>
#include<condition_variable>

std::queue<int> q;
std::mutex mtx;
std::condition_variable cond;
bool finished = false;

void producer()
{   
    
    for(int i=0;i<100;i++)
    {   
        std::lock_guard<std::mutex> lock(mtx);
        {
        q.push(i);
        }
        cond.notify_one();
        
        {
            std::lock_guard<std::mutex> lock(mtx);
            finished = true;
        }
      
    }
    
    cond.notify_one();

}

void consumer()
{
    long sum =0;
    while(true)
    {
        std::unique_lock<std::mutex> lk(mtx);
        cond.wait(lk,[&]{return !q.empty()||finished;});
        if(q.empty()&&finished)
        break;
        sum+=q.front();
        q.pop();
    }
    std::cout<<sum<<std::endl;
}

int main(int argc, char const *argv[])
{
    
    std::thread t1(producer);
    std::thread t2(consumer);

    t1.join();
    t2.join();

    return 0;
}
