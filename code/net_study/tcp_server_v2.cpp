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
#include<cstdio>
int main()
{   



    //提前声明容器，贯穿全局
    std::unordered_map<int,int> map1;
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
    char buffer[1024 * 6] = {0};
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
                map1.emplace(client_fd,client_fd);
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
               printf("已连接的老客户端有新数据\n");
                int fd = evs[i].data.fd;
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
                continue;
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
    //6.关闭socket(四次挥手)
    /*
    close(server_fd);
    return 0;
    */
    return 0;
}  
    
        
   
