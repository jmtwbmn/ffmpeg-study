#include <iostream>
#include <thread>
#include<mutex>

long counter = 0;
std::mutex mtx;

void work()
{
    for (int i = 0; i < 1000000; ++i)
     std::lock_guard<std::mutex> lock(mtx);
        counter++;          // 就这一行，看着完全没问题
}

int main()
{   
    std::thread t1(work);
    
    std::thread t2(work);

    t1.join();
    
    t2.join();
    std::cout << "counter = " << counter << std::endl;
}
