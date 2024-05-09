#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(12345);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int key = 1234567;  // Example key
    int probability = 50; // Example probability
    char command[10];

    fd_set readfds;
    struct timeval tv;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        if (select(sockfd + 1, &readfds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                scanf("%s", command);
                if (strcmp(command, "E") == 0) {
                    break;
                } else if (strcmp(command, "P") == 0) {
                    printf("Key: %d, Probability: %d\n", key, probability);
                }
            }
            if (FD_ISSET(sockfd, &readfds)) {
                int len = sizeof(cliaddr);
                int received_number;
                int n = recvfrom(sockfd, &received_number, sizeof(received_number), 0, (struct sockaddr *)&cliaddr, &len);
                if (n > 0) {
                    printf("Received Number: %d\n", received_number);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
