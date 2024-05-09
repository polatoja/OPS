// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h> // For struct timeval

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    // 创建 socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // 服务器信息
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // 生成随机数
    int random_number = rand() % 9000000 + 1000000; // 7位数
    printf("Generated Number: %d\n", random_number);

    // 发送数据
    sendto(sockfd, &random_number, sizeof(random_number), 0,
           (const struct sockaddr *) &servaddr, sizeof(servaddr));

    // 设置接收超时
    struct timeval tv;
    tv.tv_sec = 2;  // Set timeout to 2 seconds
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    // 接收数据
    int received_value;
    socklen_t len = sizeof(servaddr);
    if (recvfrom(sockfd, &received_value, sizeof(received_value), 0, (struct sockaddr *) &servaddr, &len) > 0) {
        printf("Received Value: %d\n", received_value);
    } else {
        printf("No response received.\n");
    }

    // 关闭 socket
    close(sockfd);
    return 0;
}
