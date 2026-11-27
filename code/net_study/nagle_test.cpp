#include<sys/socket.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<cstdlib>
#include<cstdio>
#include<iostream>

int main(int argc, char const *argv[])
{
    //0默认，1是TCP——NODELAY，2是TCP_CORK
    int mode = (argc > 1)? atoi(argv[1]) : 0;

    int client_fd = socket(AF_INET,SOCK_STREAM,0);
    if(client_fd == -1)
    {
        std::cout<<"创建客户端失败"<<std::endl;
        return -1;
    }

    sockaddr_in client_addr{};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(8888);
    inet_pton(AF_INET,"127.0.0.1",&client_addr.sin_addr);

   if(connect(client_fd,(sockaddr*)&client_addr,sizeof(client_addr))==-1)
   {
    perror("connect");
    return -1;
   }

   int one =1;
   if(mode ==1 )
   {
    setsockopt(client_fd,IPPROTO_TCP,TCP_NODELAY,&one,sizeof(one));
   }
   else if(mode == 2)
   {
    setsockopt(client_fd,IPPROTO_TCP,TCP_CORK,&one,sizeof(one));
   }

   for(int i =0;i<20;i++)
   {
    send(client_fd,"x",1,0);
   }

   if(mode == 2)
   {
        int zero =0;
        setsockopt(client_fd,IPPROTO_TCP,TCP_CORK,&zero,sizeof(zero));
   }

   sleep(1);
   close(client_fd);
    return 0;
}
