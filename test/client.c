#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv)
{  

    struct sockaddr_in client_addr;
    bzero(&client_addr,sizeof(client_addr));        //把一段内存区的内容全部设置为0
    client_addr.sin_family = AF_INET;               //internet协议族
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);//INADDR_ANY表示自动获取本机地址
    client_addr.sin_port = htons(0);                //0表示让系统自动分配一个空闲端口


    if (argc != 3)
    {
        printf("Usage: ./%s IP port\n",argv[0]);
        exit(1);
    }

    //设置一个socket地址结构client_addr,代表客户机internet地址, 端口
    //创建用于internet的流协议(TCP)socket
    int client_socket = socket(AF_INET,SOCK_STREAM,0);
    int server_port;
    //    sprintf(&server_port,"%d", argv[2]);
    server_port = atoi(argv[2]);

    if( client_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }
    //把客户机的socket和客户机的socket地址结构联系起来
    if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
    {
        printf("Client Bind Port Failed!\n"); 
        exit(1);
    }

    //设置一个socket地址结构server_addr,代表服务器的internet地址, 端口
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if(inet_aton(argv[1],&server_addr.sin_addr) == 0) //服务器的IP地址来自程序的参数
    {
        printf("Server IP Address Error!\n");
        exit(1);
    }
    server_addr.sin_port = htons(server_port);
    socklen_t server_addr_length = sizeof(server_addr);
    //向服务器发起连接,连接成功后client_socket代表了客户机和服务器的一个socket连接
    if(connect(client_socket,(struct sockaddr*)&server_addr, server_addr_length) < 0)
    {
        printf("Can Not Connect To %s!\n",argv[1]);
        exit(1);
    }

    char buffer[100];
    bzero(buffer,100);
    printf("input some thing to server:\t");
    scanf("%s", buffer);
    //向服务器发送buffer中的数据
    send(client_socket,buffer,100,0);


    //从服务器接收数据到buffer中
    bzero(buffer,100);
    int length = 0;

    while( length = recv(client_socket,buffer,100,0))
    {

        if(length < 0)
        {
            printf("Recieve Data From Server %s Failed!\n", argv[1]);
            break;
        }
        //        int write_length = write(fp, buffer,length);
        printf("Receive from server: %s \n", buffer);
        bzero(buffer,100);    

        printf("input some thing to server: \n");
        scanf("%s", buffer);
        if (strcmp("exit", buffer) == 0) {
            exit(0);
        }
        //向服务器发送buffer中的数据
        send(client_socket,buffer,100,0);
        bzero(buffer,100);    


    }
    //关闭socket
    close(client_socket);
    return 0;
}
