#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h> // For seeding random

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

    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    int key = 1234567;  // Example key

    while (1) {
        int n, len = sizeof(cliaddr);
        int received_number;

        n = recvfrom(sockfd, &received_number, sizeof(received_number), 0, (struct sockaddr *)&cliaddr, &len);
        if (n > 0) {
            printf("Received Number: %d\n", received_number);
            int response = (key + received_number) % 100000000;
            sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&cliaddr, len);
        }
    }

    close(sockfd);
    return 0;
}
