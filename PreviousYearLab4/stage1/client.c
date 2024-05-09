// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    // 创建 socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // 设置服务器地址
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_port = htons(12345); // 服务器端口
    servaddr.sin_addr.s_addr = INADDR_ANY; // 服务器地址，这里使用本地地址

    // 生成随机数
    // 确保随机数总是7位数
    int random_number = rand() % 9000000 + 1000000; // 生成一个7位随机数（1000000到9999999）
    printf("Generated Number: %d\n", random_number);

    // 发送数据
    if (sendto(sockfd, &random_number, sizeof(random_number), 0,
               (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }

    // 关闭 socket
    close(sockfd);
    return 0;
}
