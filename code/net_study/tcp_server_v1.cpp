#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string>
#include<cstring>
#include<unordered_map>
#include<sys/epoll.h>
#include<fcntl.h>

int main()
{   
    //提前声明容器，贯穿全局
    std::unordered_map<int,int> map1;

    //1.创建TCP socekt,ipv4，流式传播

    int server_fd  =socket(AF_INET,SOCK_STREAM,0);
    if(server_fd == -1)
    {
        std::cerr<<"创建socekt失败"<<std::endl;
        return -1;
    }

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

     //创建epoll实例，返回一个fd
    int epoll_fd = epoll_create1(0);
    //构造事件结构体
    struct epoll_event ev;
    ev.events = EPOLLIN; //可读
    ev.data.fd = server_fd; //关联的fd
    //注册，修改，删除要监听的fd
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev);

    //3.开始监听
    if(listen(server_fd,SOMAXCONN)==-1)
    {
        std::cerr<<"当前服务上限已满"<<std::endl;
        return -1;
        
    }
    std::cout<<"服务端启动,正在监听8888端口"<<std::endl;

    //设置非阻塞
    int flags = fcntl(server_fd,F_GETFL);
    fcntl(server_fd,F_SETFL,flags | O_NONBLOCK);


    //4.等待客户端连接(三次握手)
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
   
    //5.接收数据
    char buffer[1024] = {0};
    struct epoll_event evs[64];
    while(true)
    {
        int n =  epoll_wait(epoll_fd,evs,64,-1);
        for(int i =0;i<n;i++)
        {   
            if(evs[i].data.fd == server_fd) //监听fd有动静，连接
          {   
            
           //accept连接
            client_len = sizeof(client_addr);
            int client_fd = accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
            if(client_fd>0)
            {   
                printf("有数据:%d",server_fd);
                //三步，设置成非阻塞，不继承listen_fd的属性；

                fcntl(server_fd,F_SETFL,flags | O_NONBLOCK);
                //存入map1
                map1.emplace(client_fd,client_fd);
                //注册到epoll
                struct epoll_event cev;
                cev.events = EPOLLIN;
                cev.data.fd = client_fd;
                epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&cev);
            }
            close(client_fd);
          }
            else    //其他的客户端有动静
            {
               printf("其他客户端有新连接");
               //1.查map1,发现是已知的客户端
               int fd = evs[i].data.fd;
               int bytes = recv(fd,buffer,sizeof(buffer),0);

               if(n>0){
                //2.打印收到的字节数和内容
                std::cout<<"收到"<<n<<"个字节";
                std::cout.write(buffer,bytes);      //输出n长度的内容
                std::cout<<std::endl;
               }
               else if(n==0)
               {}
               //3.对端关闭：EPOLL_CTL_DEL -- close -- map1.erase
               else{

               };
               
               //4.出错重试，ENNTR重试，ENGAIN是暂时没有数据

               
            }
            
        }
        
    }
        memset(buffer,0,sizeof(buffer));    //清空缓冲区
    //6.关闭socket(四次挥手)
    
    close(server_fd);
    return 0;
}
