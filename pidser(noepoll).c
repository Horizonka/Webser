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

#define PORT 13476
#define MAX_EVENTS 1024



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
        close(c);
        return;
    }

    printf("接收到请求：\n%s\n", buff);
    char* filename = get_filename(buff);
    if (!filename) {
        send(c, "HTTP/1.1 400 Bad Request\r\n\r\n", 28, 0);
        close(c);
        continue;
        //return;
    }

    int filesize;
    int fd = open_file(filename, &filesize);
    if (fd == -1) {
        int fd404 = open("/home/ykh/study/Webser/my404.html", O_RDONLY);
        if (fd404 == -1) {
            printf("404 页面缺失\n");
            close(c);
            continue;
           // return;
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



    // 初始化线程池（线程数 4，最大任务数 128）
    threadpool_t* pool = threadpool_create(4, 128);
    assert(pool != NULL);

    printf("服务器已启动，监听端口 %d...\n", PORT);

    while (1) {
        struct sockaddr_in caddr;
        socklen_t len = sizeof(caddr);
        int c = accept(sockfd, (struct sockaddr*)&caddr, &len);
        if (c < 0) continue;

        int* pclient = malloc(sizeof(int));
        *pclient = c;

        threadpool_add(pool, handle_client, pclient); // 加入线程池任务
    }

   
    close(sockfd);
    threadpool_destroy(pool); // 不太可能执行到这里
   
    return 0;
}


