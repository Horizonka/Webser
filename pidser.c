#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<pthread.h>
#include<fcntl.h>
#include <assert.h>
#include"threadpool.h"
int create_socket()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1)
    {
        return -1;
    }

    struct sockaddr_in  saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(13476);
   // saddr.sin_addr.s_addr=inet_addr("192.168.6.119");
    saddr.sin_addr.s_addr=inet_addr("0.0.0.0");

    int res=bind(sockfd,(struct sockaddr *)&saddr,sizeof(saddr));
    if(res==-1)
    {
        return -1;
    }

    res=listen(sockfd,5);
    if(res==-1)
    {
        return -1;
    }

    return sockfd;
}


//打开文件,调用open_file得到文件描述符fd和文件的大小filesize;
int open_file(char* filename,int* filesize)
{
	if(filename==NULL||filesize==NULL)
	{
		return -1;
	}
    

	//char path[128]={"/home/stu/quzijie/class02/test34"};
    char path[128]={"/home/ykh/study/Webser"};
	if(strcmp(filename,"/")==0)
	{
		strcat(path,"/index.html");
	}
	else
	{
		strcat(path,filename);
	}

	int fd = open(path,O_RDONLY);
    if(fd==-1)
	{
       *filesize=0;
       		return -1;
	}
        
      *filesize=lseek(fd,0,SEEK_END);
       lseek(fd,0,SEEK_SET);

        return fd;	
}
char* get_filename(char buff[])
{
    if(buff==NULL)
    {
        return NULL;
    }
    char* ptr=NULL;
    char* s=strtok_r(buff," ",&ptr);
    if(s==NULL)
    {
        return NULL;
    }
    printf("请求方法：%s\n",s);
    
    s=strtok_r(NULL," ",&ptr);
    return s;
}
void* work_fun(void* arg)
{
	int c=*(int *)arg;

	while(1)
	{
		char buff[1024]={0};
		int n=recv(c,buff,1023,0);
        if(n<=0)
		{
			break;
		}
        printf("n=%d,read:\n",n);
        printf("%s\n",buff);

		char* filename=get_filename(buff);
		if(filename==NULL)
		{
			send(c,"404",3,0);
			break;
		}
		printf("filename:%s\n",filename);

		int filesize = 0;
		int fd =open_file(filename,&filesize);
		if(fd==-1)
		{
			// send(c,"404",3,0);
			// break;
             int fd404=open("/home/ykh/study/Webser/my404.html",O_RDONLY);
            if(fd404==-1){
                printf("my404打开失败\n");
                close(c);
                continue;
            }
            int len_404=lseek(fd404,0,SEEK_END);
            lseek(fd404,0,SEEK_SET);

             // 发送404报头
        char  sendbuff_404[512]={0};
        strcpy(sendbuff_404,"HTTP/1.1  404\r\n");
        strcat(sendbuff_404,"Server:myweb_ykh\r\n");
        //strcat(sendbuff,"Content-Length:19\r\n");
        sprintf(sendbuff_404+strlen(sendbuff_404),"Content-Length:%d\r\n",len_404);
        strcat(sendbuff_404,"\r\n");
        //strcat(sendbuff,"welcome to quzijie!");
        printf("send:\n%s\n",sendbuff_404);

        send(c,sendbuff_404,strlen(sendbuff_404),0);
        
        //发送my404.html页面内容;
        char data_404[512]={0};
        int num=0;
        while((num=read(fd404,data_404,512))>0)
        {
            send(c,data_404,num,0);
        }
         close(fd404);

		}
         char sendbuff[512]={0};
        strcpy(sendbuff,"HTTP/1.1  200 OK\r\n");
		strcat(sendbuff,"Server:myweb_ykh\r\n");
		sprintf(sendbuff + strlen(sendbuff),"Content-Length:%d\r\n",filesize);
		strcat(sendbuff,"\r\n");

		printf("send:\n%s\n",sendbuff);
		send(c,sendbuff,strlen(sendbuff),0);

		
		char data[512]={0};
        int num=0;
		while((num=read(fd,data,512))>0)
		{
			send(c,data,num,0);
		}

		close(fd);
	}
	close(c);
	printf("client close\n");
}
int main()
{
    int sockfd = create_socket( );
    assert(sockfd!=-1);
    //threadpool_t* pool = threadpool_create(4, 100);
    struct sockaddr_in  caddr;
    int len=-1;
    int c=-1;

    while( 1 )
    {
        len = sizeof(caddr);
        printf("wait client...\n");
        c = accept(sockfd,(struct sockaddr*)&caddr,&len);
        if( c<0 )
        {
            continue;
        }
        
        printf("accept client c=%d\n",c);

        pthread_t id;
	    pthread_create(&id,NULL,work_fun,&c);
    }
}



// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <assert.h>
// #include <pthread.h>
// #include "threadpool.h" // 引入你自己的线程池头文件

// #define PORT 13476

// // ==== 工具函数：读取文件 ====
// int open_file(char* filename, int* filesize) {
//     if (!filename || !filesize) return -1;

//     char path[128] = "/home/webser";
//     if (strcmp(filename, "/") == 0) {
//         strcat(path, "/index.html");
//     } else {
//         strcat(path, filename);
//     }

//     int fd = open(path, O_RDONLY);
//     if (fd == -1) {
//         *filesize = 0;
//         return -1;
//     }

//     *filesize = lseek(fd, 0, SEEK_END);
//     lseek(fd, 0, SEEK_SET);
//     return fd;
// }

// char* get_filename(char buff[]) {
//     if (!buff) return NULL;
//     char* ptr = NULL;
//     char* s = strtok_r(buff, " ", &ptr);
//     if (!s) return NULL;
//     s = strtok_r(NULL, " ", &ptr);
//     return s;
// }

// // ==== 任务函数：处理一个客户端 ====
// void handle_client(void* arg) {
//     int c = *(int*)arg;
//     free(arg);

//     char buff[1024] = {0};
//     int n = recv(c, buff, sizeof(buff)-1, 0);
//     if (n <= 0) {
//         close(c);
//         return;
//     }

//     printf("接收到请求：\n%s\n", buff);
//     char* filename = get_filename(buff);
//     if (!filename) {
//         send(c, "HTTP/1.1 400 Bad Request\r\n\r\n", 28, 0);
//         close(c);
//         return;
//     }

//     int filesize;
//     int fd = open_file(filename, &filesize);
//     if (fd == -1) {
//         int fd404 = open("/home/webser/my404.html", O_RDONLY);
//         if (fd404 == -1) {
//             printf("404 页面缺失\n");
//             close(c);
//             return;
//         }

//         int len_404 = lseek(fd404, 0, SEEK_END);
//         lseek(fd404, 0, SEEK_SET);

//         char header[256];
//         snprintf(header, sizeof(header),
//                  "HTTP/1.1 404 Not Found\r\n"
//                  "Content-Length: %d\r\n"
//                  "Server: myweb\r\n\r\n", len_404);
//         send(c, header, strlen(header), 0);

//         char buf[512];
//         int r;
//         while ((r = read(fd404, buf, sizeof(buf))) > 0) {
//             send(c, buf, r, 0);
//         }
//         close(fd404);
//     } else {
//         char header[256];
//         snprintf(header, sizeof(header),
//                  "HTTP/1.1 200 OK\r\n"
//                  "Content-Length: %d\r\n"
//                  "Server: myweb\r\n\r\n", filesize);
//         send(c, header, strlen(header), 0);

//         char buf[512];
//         int r;
//         while ((r = read(fd, buf, sizeof(buf))) > 0) {
//             send(c, buf, r, 0);
//         }
//         close(fd);
//     }

//     close(c);
//     printf("已处理客户端连接。\n");
// }

// // ==== 主函数 ====
// int main() {
//     int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     assert(sockfd != -1);

//      // 设置 SO_REUSEADDR 解决端口占用问题
//     int opt = 1;
//     setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


//     struct sockaddr_in saddr;
//     saddr.sin_family = AF_INET;
//     saddr.sin_port = htons(PORT);
//     saddr.sin_addr.s_addr = INADDR_ANY;
//     assert(bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr)) != -1);
//     assert(listen(sockfd, 5) != -1);

//     // 初始化线程池（线程数 4，最大任务数 128）
//     threadpool_t* pool = threadpool_create(4, 128);
//     assert(pool != NULL);

//     printf("服务器已启动，监听端口 %d...\n", PORT);

//     while (1) {
//         struct sockaddr_in caddr;
//         socklen_t len = sizeof(caddr);
//         int c = accept(sockfd, (struct sockaddr*)&caddr, &len);
//         if (c < 0) continue;

//         int* pclient = malloc(sizeof(int));
//         *pclient = c;

//         threadpool_add(pool, handle_client, pclient); // 加入线程池任务
//     }

//     threadpool_destroy(pool); // 不太可能执行到这里
//     close(sockfd);
//     return 0;
// }


