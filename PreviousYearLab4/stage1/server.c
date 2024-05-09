// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // 创建 socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // 绑定 socket
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(12345);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 接收数据
    int n, len, received_number;
    len = sizeof(cliaddr);  // 客户端地址的长度

    n = recvfrom(sockfd, &received_number, sizeof(received_number), 0,
                 (struct sockaddr *)&cliaddr, &len);
    if (n < 0) {
        perror("recvfrom failed");
        exit(EXIT_FAILURE);
    }

    printf("Received Number: %d\n", received_number);

    // 关闭 socket
    close(sockfd);
    return 0;
}
