#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<cstring>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<sys/mman.h>

int main(int argc, char const *argv[])
{
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

    //拿file_fd,fstat拿文件大小
    int file_fd = open("bigfile",O_RDONLY);
    if(file_fd == -1)
    {
        perror("open");
        return -1;
    }
    struct stat st;
    if(fstat(file_fd,&st)== -1)
    {
        perror("fstat");
        return -1;
    }

    int mode = argc > 1 ? atoi(argv[1]) : 0;

    if(mode ==0 )   //默认
    {
        char buf[64 * 1024];
        ssize_t n;
        while((n=read(file_fd,buf,sizeof(buf)))>0)
        write(client_fd,buf,n);
    }
    else if(mode == 1)     //sendfile
    {
        off_t offset =0;
        while(sendfile(client_fd,file_fd,&offset,64 * 1024)>0){}
    }
    else if(mode ==2 )      //mmap
    {
        char* p = (char*)mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,file_fd,0);
        write(client_fd,p,st.st_size);
        munmap(p,st.st_size);
    }
    close(server_fd);
    close(file_fd);
    close(client_fd);
    return 0;
}