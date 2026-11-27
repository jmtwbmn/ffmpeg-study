#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string>
#include<cstring>
#include<unordered_map>
#include<sys/epoll.h>
#include<fcntl.h>
#include<cerrno>
#include<vector>
#include<cstdio>

//map1的结构体
struct Conn {
        int fd;
        std::vector<char> sendq;    //待发送的数据
        size_t sent =0;             //已经发送到sendq的哪个位置
    };

int queue_send(Conn& c, const char* data , size_t len)
{   
    //队列还有要发的，追加
    if(c.sent < c.sendq.size())
    {
        c.sendq.insert(c.sendq.end(),data,data + len);
        return 0;
    }
    //队列空了，直接发试试
        ssize_t n = send(c.fd,data,len,MSG_NOSIGNAL);
      //全部发完了就直接返回
      if(n == (ssize_t)len) return 0;

      //如果n还在len之内,说明发了一部分，剩下的到队列
      if(n<(ssize_t)len && n>0)
      {
        c.sendq.insert(c.sendq.end(),data+n,data+len);
        return 0;
      }
      //返回-1看errno
      if(n ==-1)
      {
       
        //内核缓冲区满了，进队列等待
        if(errno == EAGAIN || errno == EWOULDBLOCK) 
        {
         c.sendq.insert(c.sendq.end(),data,data+len);
         return 0;
        }
         //被信号打断重试
        else if(errno == EINTR) 
        {   
            n = send(c.fd,data,len,MSG_NOSIGNAL);
           if(n == (ssize_t)len) return 0;      //这次全发完了
           if(n > 0)                            //这次发了一部分，剩下的进队列
           {
            c.sendq.insert(c.sendq.end(),data+n,data+len);
            return 0;
           }
           if(n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
           {
            c.sendq.insert(c.sendq.end(),data,data+len);   //一个都没发出去，全部进队列
            return 0;
           }
        }
        //连接坏了，-1退出调用对应处理方法
        return -1;
      }
     
    return 0;
    }

//把队列里剩下的数据尽量发出去
//返回 0：正常（可能还有剩，等下次 EPOLLOUT）
//返回 -1：连接坏了，调用方负责清理
int flush_sendq(Conn& c)
{
    while(c.sent < c.sendq.size())
    {
        ssize_t n = send(c.fd,c.sendq.data()+c.sent,c.sendq.size()-c.sent,MSG_NOSIGNAL);
        if(n>0){ c.sent += n; continue;}        //没发完继续发
        if(n == -1 && errno == EINTR) continue;   //打断了重试
        if(n==-1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return 0;       //堵塞了，等待EPOLLOUT
        return -1;
    }
    c.sendq.clear();    //全部发完，清空队列
    c.sent = 0;         //重置标记
    return 0;
}

int main()
{   
    // 4MB ≈ 2 秒 1080p 积压。超过说明对端已经跟不上，留着只会吃内存。
    static const size_t HIGH_WATER_MARK = 4 * 1024 * 1024;

    
    //提前声明容器，贯穿全局
    std::unordered_map<int,Conn> map1;
    //定义一个计数器，表示ET的特点
    static long wait_count = 0;

    //1.创建TCP socekt,ipv4，流式传播
   
    int server_fd  =socket(AF_INET,SOCK_STREAM,0);
    if(server_fd == -1)
    {
        std::cerr<<"创建socekt失败"<<std::endl;
        return -1;
    }
     int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //2.绑定ip和端口
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   //监听所有网卡
    server_addr.sin_port = htons(8888);         //端口8888,htons把主机字节序转为网络字节序

    if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1)
    {
        std::cerr<<"绑定端口失败"<<std::endl;
        return -1;
    }


    //3.开始监听
    if(listen(server_fd,SOMAXCONN)==-1)
    {
        std::cerr<<"当前端口已被占用"<<std::endl;
        return -1;
        
    }
    std::cout<<"服务端启动,正在监听8888端口"<<std::endl;

    //创建epoll实例，返回一个fd
    int epoll_fd = epoll_create1(0);
    //构造事件结构体
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; //可读,且在服务端标记ET边缘触发
    ev.data.fd = server_fd; //关联的fd
    //注册，修改，删除要监听的fd
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev);


    //设置非阻塞
    int flags = fcntl(server_fd,F_GETFL);
    fcntl(server_fd,F_SETFL,flags | O_NONBLOCK);

    //4.等待客户端连接(三次握手)
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
   
    //5.接收数据
    char buffer[1024 * 64] = {0};
    
    struct epoll_event evs[64];
    while(true)
    {
        int n =  epoll_wait(epoll_fd,evs,64,-1);
        if(n<0)
        {
            if(errno == EINTR) continue;     //被信号打断，再试一次
            perror("epoll_wait");           //真出错了，记录日志
            break;                          //退出循环
        }
        wait_count++;
        for(int i=0;i<n;i++)
        {
            if(evs[i].data.fd == server_fd) //监听fd有动静，连接
          {   
            while(1)
            {
           //accept连接
            client_len = sizeof(client_addr);
            int client_fd = accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
            if(client_fd>=0)
            {   
                printf("有新用户连接:%d\n",client_fd);
                //三步，设置成非阻塞，不继承listen_fd的属性；
                int cflags = fcntl(client_fd,F_GETFL);
                fcntl(client_fd,F_SETFL,cflags | O_NONBLOCK);
                //存入map1
                
                map1.emplace(client_fd,Conn{client_fd,{}});
                //注册到epoll
                struct epoll_event cev;
                cev.events = EPOLLIN | EPOLLET; //同理在客户端标记边缘触发ET
                cev.data.fd = client_fd;
                epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&cev);
                continue;
            }
            if(errno == EINTR) continue;    //信号打断，重试
            if(errno == EAGAIN || errno == EWOULDBLOCK) //这时候输出完毕了，可以退出
            {
                break;
            }
            break;
           }
          }
          else    //其他的已连接客户端有动静
            {   
                int fd = evs[i].data.fd;
                auto it = map1.find(fd);
                if(it == map1.end()) continue;  //不认识的fd，跳过

                Conn& conn = it->second;

                //1.处理可写，把积压的发了
                if(evs[i].events & EPOLLOUT)
                {
                   ssize_t n =  flush_sendq(conn);
                   if( n==-1 )  //发生错误，需要处理
                   {
                    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,NULL);
                    close(fd);
                    map1.erase(it);
                    continue;
                   }
                   if(conn.sent >= conn.sendq.size())   //队列空了摘掉EPOLLOUT，否则空转CPU
                   {
                    struct epoll_event wev;
                    wev.events = EPOLLIN | EPOLLET;
                    wev.data.fd = fd;
                    epoll_ctl(epoll_fd,EPOLL_CTL_MOD,fd,&wev);
                   }
                }
                //2.处理可读
                if(evs[i].events & EPOLLIN)
               {    
                    printf("已连接的老客户端有新数据\n");
                while(1)
                {   
                //1.查map1,发现是已知的客户端
                int bytes = recv(fd,buffer,sizeof(buffer),0);
                if(bytes>0)
                { 
                //2.打印收到的字节数和内容，同时关闭recv=0
                std::cout<<"收到"<<bytes<<"个字节";
                std::cout.write(buffer,bytes);      //输出bytes长度的内容
                std::cout<<std::endl;

                //回复客户端
                std::vector<int> delete1 ={};
                for(auto& kv : map1)
                {

                if(kv.first == fd)continue;

                if((queue_send(kv.second,buffer,bytes))==-1)
                {   
                    delete1.push_back(kv.first);
                    continue;
                }

                if(kv.second.sendq.size()> HIGH_WATER_MARK)
                {
                    delete1.push_back(kv.first);
                    continue;
                }

                struct epoll_event pev;
                pev.events = EPOLLOUT | EPOLLIN | EPOLLET;
                pev.data.fd = kv.first;
                epoll_ctl(epoll_fd,EPOLL_CTL_MOD,kv.first,&pev);
               }
                //同时清理水位限制和返回错误(-1)
                for(size_t i = 0;i<delete1.size();i++)
                {   
                    int k = delete1[i];
                    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,k,NULL);
                    close(k);
                    map1.erase(k);
                    
                }
                
                }
                else if (bytes == 0)
               {
                //对端关闭，break
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);   // ① 先摘牌
                close(fd);                                       // ② 再关门
                map1.erase(fd);                                  // ③ 最后抹记录
                printf("epoll_wait调用总次数:%ld\n",wait_count);
                printf("fd %d 已断开\n", fd);
                break;
               }
               //4.出错重试，ENNTR重试，ENGAIN是数据输出完了
                if(errno == EINTR) continue;
                if(errno == EAGAIN || errno == EWOULDBLOCK) break;

                else{//这下真坏了,delete-close-erase;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    map1.erase(fd);
                    break;
                }
                 
             }
                    
            }
 
          }
        }
    }
    //6.关闭socket(四次挥手)
    /*
    close(server_fd);
    return 0;
    */
    return 0;
}  
    
        
   
