#include "l4_common.h"
#define BACKLOG 3
#define MAX_EVENTS 16
#define MAX_CLIENTS 8

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig) { do_work = 0; }

void usage(char *name) { fprintf(stderr, "USAGE: %s socket port\n", name); }

void write_until_sign(char *dest, char *buf, size_t size)
{
  dest[0] = '0';
  int i = 1;
  while (i < size && buf[i] != '$')
  {
    dest[i] = buf[i];
    i++;
  }
  dest[i] = '\0';
}
void AddClient(struct epoll_event event, int epoll_descriptor, int *clients)
{
  int client_socket = add_new_client(event.data.fd);
  event.events = EPOLLIN;
  event.data.fd = client_socket;
  if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, client_socket, &event) == -1)
  {
    perror("epoll_ctl: listen_sock");
    exit(EXIT_FAILURE);
  }
  int size;
  char client_request[MAX_BUFF];
  char response[MAX_BUFF];
  if ((size = bulk_read(client_socket, client_request, MAX_BUFF)) < 0)
    ERR("read:");
  if (strlen(client_request) == (int)sizeof(char))
  {
    if (!(clients[atoi(client_request) - 1] > -1))
    {
      clients[atoi(client_request) - 1] = client_socket;
      snprintf(response, MAX_BUFF, "Client %d connected\n", atoi(client_request));
      if (bulk_write(client_socket, response, sizeof(response)) < 0 && errno != EPIPE)
        ERR("write:");
      fprintf(stderr, "Client %d connected\n", atoi(client_request));
    }
    else
    {
      snprintf(response, MAX_BUFF, "Wrong client id\n");
      if (bulk_write(client_socket, response, sizeof(response)) < 0 &&
          errno != EPIPE)
        ERR("write:");
      fprintf(stderr, "Wrong client id\n");
      if (TEMP_FAILURE_RETRY(close(client_socket)) < 0)
        ERR("close");
    }
  }
}

void HandleMessage(int *clients, struct epoll_event event)
{
  char client_request[MAX_BUFF];
  int size;
  if ((size = bulk_read(event.data.fd, client_request, MAX_BUFF)) < 0)
    ERR("read:");
  char message[MAX_BUFF];
  memset(message, 0, MAX_BUFF);
  fprintf(stderr, "We read %s\nWe are sending to %d\n", client_request, atoi(client_request + 1));
  write_until_sign(message, client_request, MAX_BUFF);
  int receiver = atoi((client_request + 1));
  if (receiver == 9)
  {
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (clients[i] > -1)
      {
        bulk_write(clients[i], message, sizeof(message));
        if (errno == EINTR)
          continue;
      }
    }
  }
  else
    bulk_write(clients[receiver - 1], message, sizeof(message));
}

void doServer(int tcp_listen_socket)
{
  int epoll_descriptor;
  if ((epoll_descriptor = epoll_create1(0)) < 0)
  {
    ERR("epoll_create:");
  }
  struct epoll_event event, events[MAX_EVENTS];
  event.events = EPOLLIN;
  event.data.fd = tcp_listen_socket;
  if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, tcp_listen_socket, &event) ==
      -1)
  {
    perror("epoll_ctl: listen_sock");
    exit(EXIT_FAILURE);
  }

  int clients[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; i++)
    clients[i] = -1;
  int nfds;
  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);
  while (do_work)
  {
    if ((nfds = epoll_pwait(epoll_descriptor, events, MAX_EVENTS, -1, &oldmask)) > 0)
    {
      for (int n = 0; n < nfds; n++)
      {
        if (events[n].data.fd == tcp_listen_socket)
        {
          AddClient(events[n], epoll_descriptor, clients);
          continue;
        }
        HandleMessage(clients, events[n]);
        if (errno == EINTR)
          continue;
      }
    }
  }
  if (TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
    ERR("close");
  sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char **argv)
{
  int tcp_listen_socket;
  int new_flags;
  if (argc != 2)
  {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (sethandler(SIG_IGN, SIGPIPE))
    ERR("Seting SIGPIPE:");
  if (sethandler(sigint_handler, SIGINT))
    ERR("Seting SIGINT:");
  tcp_listen_socket = bind_tcp_socket(atoi(argv[1]), BACKLOG);
  new_flags = fcntl(tcp_listen_socket, F_GETFL) | O_NONBLOCK;
  fcntl(tcp_listen_socket, F_SETFL, new_flags);
  doServer(tcp_listen_socket);
  if (TEMP_FAILURE_RETRY(close(tcp_listen_socket)) < 0)
    ERR("close");
  if (unlink(argv[1]) < 0)
    ERR("unlink");
  fprintf(stderr, "Server has terminated.\n");
  return EXIT_SUCCESS;
}
