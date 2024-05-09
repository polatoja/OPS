#include "l4_common.h"
#include <stdio.h>
void usage(char *name) {
  fprintf(stderr, "USAGE: %s domain port  operand1 operand2 operation \n",
          name);
}

int main(int argc, char **argv) {
  if (argc != 4) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  int com_socket = connect_tcp_socket(argv[1], argv[2]);
  char request[MAX_BUFF];
  memset(request, 0, MAX_BUFF);
  strcpy(request, argv[3]);
  if (bulk_write(com_socket, request, MAX_BUFF) < 0)
    ERR("write:");
  char data[MAX_BUFF];
  fprintf(stderr, "Sent: %s\n", request);
  if (bulk_read(com_socket, data, MAX_BUFF) < 0)
    ERR("read:");
  fprintf(stderr, "Received: %s\n", data);
  while (1) {
    char message[MAX_BUFF];
    fgets(message, MAX_BUFF, stdin);
    message[strlen(message) - 1] = '\0';
    char operation[MAX_BUFF];
    memset(operation, 0, MAX_BUFF);
    strncpy(operation, argv[3], 1);
    strncpy(operation + 1,message, 1);
    strcpy(operation + 2, message+1);
    if (bulk_write(com_socket, operation, MAX_BUFF) < 0)
      ERR("write:");
    fprintf(stderr, "Sent: %s\n", operation);
    memset(data, 0, MAX_BUFF);
    if (bulk_read(com_socket, data, MAX_BUFF) < 0)
      ERR("read:");
    fprintf(stderr, "Received: %s\n", data);
  }
  if (TEMP_FAILURE_RETRY(close(com_socket)) < 0)
    ERR("close");
  return EXIT_SUCCESS;
}
