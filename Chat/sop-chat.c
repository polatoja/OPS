#include "l4-common.h"

#define BACKLOG_SIZE 10
#define MAX_CLIENT_COUNT 4
#define MAX_EVENTS 10

#define NAME_OFFSET 0
#define NAME_SIZE 64
#define MESSAGE_OFFSET NAME_SIZE
#define MESSAGE_SIZE 448
#define BUFF_SIZE (NAME_SIZE + MESSAGE_SIZE)

volatile sig_atomic_t interrupted = 0;

typedef struct
{
    int fd;
    char username[NAME_SIZE];
}client_t;

void do_server(int server_fd,char*key);
int add_authorized_user(int server_fd, char * key, client_t clients[MAX_CLIENT_COUNT]);
void signal_handler(int sig){interrupted = 1;}
void usage(char*program_name);

int main(int argc, char **argv) {
    char *program_name = argv[0];
    if (argc != 3) {
        usage(program_name);
    }

    uint16_t port = atoi(argv[1]);
    if (port == 0){
        usage(argv[0]);
    }
    char *key = argv[2];
    int server_fd = bind_tcp_socket(port, BACKLOG_SIZE);
    do_server(server_fd, key);
    close(server_fd);
    return EXIT_SUCCESS;
}

void usage(char *program_name) {
    fprintf(stderr, "USAGE: %s port key\n", program_name);
    exit(EXIT_FAILURE);
}

void do_server(int server_fd,char*key)
{
    // dealing with sigint
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    if (sigaction(SIGINT, &sa, NULL)<0)
        ERR("sigaction");
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK,&mask, &oldmask);
    //
    client_t clients[MAX_CLIENT_COUNT];
    for (int i=0;i<MAX_CLIENT_COUNT;i++)
        clients[i].fd = -1;
    int epoll_fd = epoll_create1(0);
    struct epoll_event event = {};
    event.data.fd = server_fd;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd,EPOLL_CTL_ADD, server_fd, &event))
        ERR("epoll_ctl");
    while(1)
    {
        struct epoll_event ev[MAX_EVENTS];
        int nfds = epoll_pwait(epoll_fd, ev, MAX_EVENTS, -1, &oldmask);
        if (nfds<0)
        {
            if (errno == EINTR && interrupted)
                break;
            ERR("epoll_pwait");
        }
        for (int i=0;i<nfds;i++)
        {
            if (ev[i].events & EPOLLIN)
            {
                int fd = ev[i].data.fd;
                if (fd == server_fd)
                {
                    int client_fd = add_authorized_user(server_fd, key,clients);
                    if (client_fd>=0)
                    {
                        event.data.fd = client_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event)<0)
                            ERR("epoll_ctl");
                    }
                }
                else
                {
                    char buf[BUFF_SIZE];
                    int ret = bulk_read(fd, buf, BUFF_SIZE);
                    if (ret >0)
                    {
                        printf("---------\n%.*s: %s\n",NAME_SIZE,buf,buf+NAME_SIZE);
                        for (int ind=0;ind<MAX_CLIENT_COUNT;ind++)
                        {
                            if (clients[ind].fd!=-1 && clients[ind].fd!=fd)
                            {
                                if (bulk_write(clients[ind].fd, buf, BUFF_SIZE)<0)
                                    ERR("write");
                            }
                        }
                    }   
                    else if (ret==0)
                    {
                        if (epoll_ctl(epoll_fd,EPOLL_CTL_DEL, fd, NULL)<0)
                            ERR("epoll_ctl");
                        for(int j=0;j<MAX_CLIENT_COUNT;j++)
                        {
                            if (clients[j].fd==fd)
                            {
                                close(fd);
                                clients[j].fd = -1;
                                printf("%s has disconnected\n", clients[j].username);
                                break;
                            }
                        }
                    }    
                    else if (errno!=EAGAIN)
                        ERR("recv"); 
                }
            }
            
        }
    }
    for (int i=0;i<MAX_CLIENT_COUNT;i++)
    {
        if (clients[i].fd!=-1)
            close(clients[i].fd);
    }

}
int add_authorized_user(int server_fd, char * key, client_t clients[MAX_CLIENT_COUNT])
{
    int client_fd = add_new_client(server_fd);
    //printf("Accepted new client\n");
    char buf[BUFF_SIZE];
    if (bulk_read(client_fd, buf, BUFF_SIZE)<0)
        ERR("read");
    char client_name[NAME_SIZE];
    strncpy(client_name, buf, NAME_SIZE);
    client_name[NAME_SIZE - 1] = '\0';
    char message[MESSAGE_SIZE];
    strncpy(message, buf+NAME_SIZE,MESSAGE_SIZE);
    message[MESSAGE_SIZE - 1] = '\0';
    printf("******\nUser:%s\nKey:%s\n***\n", client_name, message);
    if (strcmp(message, key)!=0)
    {
        printf("Not authorized, closing connection\n");
        close(client_fd);
        return -1;
    }
    printf("%s has successfully connected\n", client_name);
    for (int i=0;i<MAX_CLIENT_COUNT;i++)
        {
            if (clients[i].fd==-1)
            {
                clients[i].fd=client_fd;
                strncpy(clients[i].username, client_name, NAME_SIZE);
                break;
            }
            else if (i==MAX_CLIENT_COUNT-1)
            {
                close(client_fd);
                return -1;                   
            }
        }
    if (bulk_write(client_fd, buf,BUFF_SIZE)<0)
        ERR("write");
    return client_fd;
} 