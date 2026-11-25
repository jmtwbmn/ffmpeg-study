#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string>
#include<cstring>

int main()
{
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

    //3.开始监听
    if(listen(server_fd,SOMAXCONN)==-1)
    {
        std::cerr<<"监听失败"<<std::endl;
        return -1;
    }
    std::cout<<"服务端启动,正在监听8888端口"<<std::endl;

    //4.阻塞等待客户端连接(三次握手)
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
    if(client_fd == -1)
    {
        std::cerr<<"接受连接失败"<<std::endl;
        return -1;
    }
    std::cout<<"客户端已连接"<<std::endl;

    //5.接收数据
    char buffer[1024] = {0};
    while(true)
    {
        int bytes_received = recv(client_fd,buffer,sizeof(buffer) - 1, 0);
        if(bytes_received <= 0)
        {
            std::cout<<"客户端断开连接"<<std::endl;
            break;
        }
        std::cout<<"["<<bytes_received<<"B]";
        std::cout.write(buffer,bytes_received);
        std::cout<<std::endl;
        memset(buffer,0,sizeof(buffer));    //清空缓冲区
    }

    //6.关闭socket(四次挥手)
    close(client_fd);
    close(server_fd);
    return 0;
}
