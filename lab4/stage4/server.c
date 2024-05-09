#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENTS 5
#define READ_SIZE 10

int main() {
    int sockfd, epoll_fd;
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

    // Set stdin to non-blocking
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    // Add sockfd to epoll
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
        perror("epoll_ctl: sockfd");
        exit(EXIT_FAILURE);
    }

    // Add stdin to epoll
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
        perror("epoll_ctl: stdin");
        exit(EXIT_FAILURE);
    }

    int key = 1234567;  // Example key
    int probability = 50; // Example probability
    char cmd_buffer[READ_SIZE];
    int nfds, n;

    while (1) {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sockfd) {
                int len = sizeof(cliaddr);
                int received_number;
                n = recvfrom(sockfd, &received_number, sizeof(received_number), 0, (struct sockaddr *)&cliaddr, &len);
                if (n > 0) {
                    printf("Received Number: %d\n", received_number);
                    // Simulate probability
                    if (rand() % 100 < probability) {
                        int response = (key + received_number) % 100000000;
                        sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&cliaddr, len);
                    }
                }
            } else if (events[i].data.fd == STDIN_FILENO) {
                n = read(STDIN_FILENO, cmd_buffer, READ_SIZE);
                if (n > 0) {
                    // Process command
                    if (strncmp(cmd_buffer, "U\n", 2) == 0 && probability <= 90) {
                        probability += 10;
                    } else if (strncmp(cmd_buffer, "D\n", 2) == 0 && probability >= 10) {
                        probability -= 10;
                    } else if (strncmp(cmd_buffer, "P\n", 2) == 0) {
                        printf("Key: %d, Probability: %d\n", key, probability);
                    } else if (strncmp(cmd_buffer, "E\n", 2) == 0) {
                        goto shutdown;
                    }
                }
            }
        }
    }

shutdown:
    close(sockfd);
    close(epoll_fd);
    return 0;
}
