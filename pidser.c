#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <pthread.h>
#include<sys/epoll.h>
#include "threadpool.h" // 引入你自己的线程池头文件

#define PORT 18888
#define MAX_EVENTS 1024


// ==== 工具函数：设置非阻塞 ====
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

const char* get_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');  // 找到最后一个点
    if (!ext) return "application/octet-stream";

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
        return "text/html";
    if (strcmp(ext, ".css") == 0)
        return "text/css";
    if (strcmp(ext, ".js") == 0)
        return "application/javascript";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)
        return "image/gif";
    if (strcmp(ext, ".svg") == 0)
        return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0)
        return "image/x-icon";
    if (strcmp(ext, ".txt") == 0)
        return "text/plain";
    if (strcmp(ext, ".json") == 0)
        return "application/json";
    if (strcmp(ext, ".pdf") == 0)
        return "application/pdf";
    if (strcmp(ext, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(ext, ".mp4") == 0)
        return "video/mp4";

    return "application/octet-stream";  // 默认二进制流
}
// ==== 工具函数：读取文件 ====
int open_file(char* filename, int* filesize) {
    if (!filename || !filesize) return -1;

    char path[128] = "/home/ykh/study/Webser";
    if (strcmp(filename, "/") == 0) {
        strcat(path, "/index.html");
    } else {
        strcat(path, filename);
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        *filesize = 0;
        return -1;
    }

    *filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    return fd;
}

char* get_filename(char buff[]) {
    if (!buff) return NULL;
    char* ptr = NULL;
    char* s = strtok_r(buff, " ", &ptr);
    if (!s) return NULL;
    s = strtok_r(NULL, " ", &ptr);
    return s;
}

// ==== 任务函数：处理一个客户端 ====
void handle_client(void* arg) {//arg 是传递给任务函数的参数，这里是一个指向客户端套接字描述符的指针。
    int c = *(int*)arg;//将 arg 转换为整数（套接字描述符），赋值给变量 c。
    free(arg);//释放动态分配的内存（malloc 分配的 arg）。

   while(1){
    char buff[1024] = {0};
    int n = recv(c, buff, sizeof(buff)-1, 0);
    if (n <= 0) {
        //close(c);
        break;
        //continue;
    }

    printf("接收到请求：\n%s\n", buff);
    char* filename = get_filename(buff);
    if (!filename) {
        send(c, "HTTP/1.1 400 Bad Request\r\n\r\n", 28, 0);
        //close(c);
        //continue;
        break;
    }
    printf("Requested file: %s\n", filename);
    int filesize;
    int fd = open_file(filename, &filesize);

    // 添加调试日志：打印完整路径
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/home/ykh/study/Webser%s", filename);
    printf("尝试打开的文件路径: %s\n", full_path);

    if (fd == -1) {
        int fd404 = open("/home/ykh/study/Webser/my404.html", O_RDONLY);
        if (fd404 == -1) {
            printf("404 页面缺失\n");
            //close(c);
            //continue;
            break;
           
        }

        int len_404 = lseek(fd404, 0, SEEK_END);
        lseek(fd404, 0, SEEK_SET);

        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Length: %d\r\n"
                 "Server: myweb\r\n\r\n", len_404);
        send(c, header, strlen(header), 0);

        char buf[512];
        int r;
        while ((r = read(fd404, buf, sizeof(buf))) > 0) {
            send(c, buf, r, 0);
        }
        close(fd404);
    } else {
        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %d\r\n"
                 "Server: myweb\r\n\r\n", filesize);
        send(c, header, strlen(header), 0);

        char buf[512];
        int r;
        while ((r = read(fd, buf, sizeof(buf))) > 0) {
            send(c, buf, r, 0);
        }
        close(fd);
    }

   }
    close(c);
    printf("已处理客户端连接。\n");
    
}

void epoll_add(int epfd,int fd)
{
     struct epoll_event ev;
    ev.events=EPOLLIN;//只读事件
     ev.data.fd=fd;
 
     if(epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev)==-1)
     {
         perror("epoll ctl add err");
    }
 }

// ==== 主函数 ====
int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);

     // 设置 SO_REUSEADDR 解决端口占用问题
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
//在服务器重启时，如果端口仍处于 TIME_WAIT 状态，设置 SO_REUSEADDR 可以避免绑定失败。

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = INADDR_ANY;
    assert(bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr)) != -1);
    assert(listen(sockfd, 5) != -1);

    set_nonblocking(sockfd);

    int epfd = epoll_create1(0);
    assert(epfd>=0);

    struct epoll_event ev,events[MAX_EVENTS];
    epoll_add(epfd,sockfd);

    // 初始化线程池（线程数 1，最大任务数 20）
    threadpool_t* pool = threadpool_create(1, 20);
    assert(pool != NULL);

    printf("服务器已启动，监听端口 %d...\n", PORT);


    while(1){
        int n = epoll_wait(epfd,events,MAX_EVENTS,-1);
       
        for(int i = 0;i<n;i++){
            int fd = events[i].data.fd;
            if(fd == sockfd){
                struct sockaddr_in caddr;
                socklen_t len = sizeof(caddr);
                int c = accept(sockfd, (struct sockaddr*)&caddr, &len);
                if(c<0) continue;
                    set_nonblocking(c);
                    epoll_add(epfd,c);
                    printf("新客户端接入：%d\n", c);
            } else if(events[i].events & EPOLLIN){
                int client_fd = fd;
                 // 先从 epoll 中删掉，避免重复触发
            if (epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
                perror("epoll_ctl DEL");
            }
                int *pclient = malloc(sizeof(int));//将当前触发事件的 client_fd（客户端 socket）存进去。
                *pclient = client_fd;//把当前准备处理的客户端文件描述符 client_fd 的值写进刚才分配的内存中
                //把 pclient 作为参数传给了 handle_client 线程函数。而线程函数只能接受 void* 类型的参数，
                //所以你得把 client_fd 包装成一个指针，才能传进去。
               
                threadpool_add(pool, handle_client, pclient); 
                
            }
       }
    }
    close(sockfd);
    threadpool_destroy(pool); // 不太可能执行到这里
   
    return 0;
}




