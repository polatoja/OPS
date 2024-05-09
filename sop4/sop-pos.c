#include "l4-common.h"
#define BACKLOG 3
#define MESS_SIZE 4
#define MAX_CLIENTS 4
#define MAX_EVENTS 16
#define CITIES_COUNT 20

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
    (void)sig;
    do_work = 0;
}

void doServer(int tcp_listen_socket)
{
    char cities[CITIES_COUNT];
    for (int i = 0; i < CITIES_COUNT; i++)
    {
        cities[i] = 'g';
    }

    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;

    event.data.fd = tcp_listen_socket;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, tcp_listen_socket, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i] = -1;
    }
    int nfds;
    char data[5];
    ssize_t size;
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    int success = 0;
    int client_socket;
    while (do_work)
    {
        if ((nfds = epoll_pwait(epoll_descriptor, events, MAX_EVENTS, -1, &oldmask)) > 0)
        {
            for (int n = 0; n < nfds; n++)
            {
                if (events[n].data.fd == tcp_listen_socket)
                {
                    client_socket = add_new_client(events[n].data.fd);
                    success = 0;
                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (clients[i] == -1)
                        {
                            event.events = EPOLLIN;
                            event.data.fd = client_socket;
                            if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, client_socket, &event) == -1)
                            {
                                perror("epoll_ctl: listen_sock");
                                exit(EXIT_FAILURE);
                            }
                            success = 1;
                            clients[i] = client_socket;
                            break;
                        }
                    }

                    if (!success)
                    {
                        if ((close(client_socket)) < 0)
                            ERR("close");
                    }
                }
                else
                {
                    if ((size = bulk_read(events[n].data.fd, (char*)data, MESS_SIZE)) < 0)
                        ERR("read:");
                    if (!size)
                    {
                        if ((close(events[n].data.fd)) < 0)
                            ERR("close");
                        for (int i = 0; i < MAX_CLIENTS; i++)
                        {
                            if (events[n].data.fd == clients[i])
                            {
                                clients[i] = -1;
                            }
                        }
                    }
                    int city;
                    if (size == MESS_SIZE)
                    {
                        printf("%s", data);
                        if (data[0] == 'p' || data[0] == 'g')
                        {
                            if (data[1] == '0')
                            {
                                city = data[2] - '0';
                            }
                            else
                            {
                                city = 10 * (data[1] - '0') + (data[2] - '0');
                            }
                            if (city < 1 || city > 20)
                            {
                                if ((close(events[n].data.fd)) < 0)
                                    ERR("close");
                                for (int i = 0; i < MAX_CLIENTS; i++)
                                {
                                    if (events[n].data.fd == clients[i])
                                    {
                                        clients[i] = -1;
                                    }
                                }
                                continue;
                                // city = 1;
                            }

                            if (data[0] != cities[city])
                            {
                                cities[city - 1] = data[0];
                                for (int i = 0; i < MAX_CLIENTS; i++)
                                {
                                    if (clients[i] != -1 && events[n].data.fd != clients[i])
                                    {
                                        if ((size = bulk_write(clients[i], (char*)data, MESS_SIZE)) < 0)
                                        {
                                            if (errno == EPIPE)
                                            {
                                                if ((close(events[n].data.fd)) < 0)
                                                    ERR("close");
                                                for (int j = 0; j < MAX_CLIENTS; j++)
                                                {
                                                    if (events[n].data.fd == clients[j])
                                                    {
                                                        clients[j] = -1;
                                                    }
                                                }
                                            }
                                            else
                                                ERR("read:");
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if ((close(events[n].data.fd)) < 0)
                                ERR("close");
                            for (int i = 0; i < MAX_CLIENTS; i++)
                            {
                                if (events[n].data.fd == clients[i])
                                {
                                    clients[i] = -1;
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (errno == EINTR)
                continue;
            ERR("epoll_pwait");
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] != -1)
        {
            if ((close(clients[i])) < 0)
                ERR("close");
        }
    }
    if ((close(epoll_descriptor)) < 0)
        ERR("close");
    for (int i = 0; i < CITIES_COUNT; i++)
    {
        printf("miasto %d nalezy do %c\n", i + 1, cities[i]);
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char** argv)
{
    if (sethandler(sigint_handler, SIGINT))
        ERR("Seting SIGINT:");
    if (argc != 2)
    {
        usage(argv[0]);
    }

    int tcp_listen_socket = bind_tcp_socket(atoi(argv[1]), BACKLOG);
    int new_flags = fcntl(tcp_listen_socket, F_GETFL) | O_NONBLOCK;
    fcntl(tcp_listen_socket, F_SETFL, new_flags);

    doServer(tcp_listen_socket);

    if ((close(tcp_listen_socket)) < 0)
        ERR("close");
    return 0;
}
