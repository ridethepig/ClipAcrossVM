#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
//#include "wrap.h"

const int MAXLINE = 80;
const int SERV_PORT = 8842;
const int OPEN_MAX = 1024;

int main(int argc, char *argv[]) {
  int i, j;
  int fd_listen, fd_connect, fd_socket;
  int n_ready, epoll_fd;
  ssize_t n;
  char buffer_data[MAXLINE], buffer_ip[INET_ADDRSTRLEN];
  socklen_t client_socket_len;
  int clients[OPEN_MAX];
  struct sockaddr_in addr_client, addr_server;
  struct epoll_event epoll_ev, ep[OPEN_MAX];

  fd_listen = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&addr_server, sizeof(addr_server));
  addr_server.sin_family = AF_INET;
  addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_server.sin_port = htons(SERV_PORT);
  bind(fd_listen, (struct sockaddr *)&addr_server, sizeof(addr_server));
  listen(fd_listen, 20);

  for (i = 0; i < OPEN_MAX; i++) clients[i] = -1;
  int last_client = -1;
  epoll_fd = epoll_create(OPEN_MAX);
  if (epoll_fd == -1) {
    printf("epoll fd create error.\n");
    return 1;
  }

  epoll_ev.events = EPOLLIN;
  epoll_ev.data.fd = fd_listen;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_listen, &epoll_ev) == -1) {
    printf("epoll ctl add failed on fd_listen\n");
    return 2;
  }

  printf("server started on port %d\n", SERV_PORT);

  for (;;) {
    n_ready = epoll_wait(epoll_fd, ep, OPEN_MAX, -1);
    if (n_ready == -1) {
      printf("epoll wait error.\n");
      return 3;
    }

    for (i = 0; i < n_ready; i++) {
      if (!(ep[i].events & EPOLLIN)) continue;

      if (ep[i].data.fd == fd_listen) {
        client_socket_len = sizeof(addr_client);
        fd_connect = accept(fd_listen, (struct sockaddr *)&addr_client,
                            &client_socket_len);
        printf("received from %s at PORT %d\n",
               inet_ntop(AF_INET, &addr_client.sin_addr, buffer_ip,
                         sizeof(buffer_ip)),
               ntohs(addr_client.sin_port));
        for (j = 0; j < OPEN_MAX; j++) {
          if (clients[j] < 0) {
            clients[j] = fd_connect;
            break;
          }
        }

        epoll_ev.events = EPOLLIN;
        epoll_ev.data.fd = fd_connect;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_connect, &epoll_ev) == -1) {
          printf("epoll ctl add connect fd %d failed. ignoring\n", fd_connect);
          continue;
        }

        if (j > last_client) last_client = j;
      } else {
        fd_socket = ep[i].data.fd;
        n = read(fd_socket, buffer_data, MAXLINE);

        if (n == 0) {
          for (j = 0; j <= last_client; j++) {
            if (clients[j] == fd_socket) {
              clients[j] = -1;
              break;
            }
          }

          if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_socket, NULL) == -1) {
            printf("epoll ctl del fd_socket %d failed. ignoring\n", fd_socket);
          }
          close(fd_socket);
          printf("client[%d] closed connection\n", j);
        } else {
          printf("Clipper says:");
          fflush_unlocked(stdout);
          write(1, buffer_data, n);
          puts("");
          printf("Writing into clipboard.\n");

          FILE* pp = popen("xclip -sel c", "w");
          if (!pp) {
            printf("Write clipboard error.");
          } else {
              fwrite(buffer_data, sizeof(char), n, pp);
              pclose(pp);
          }
        }
      }
    }
  }
  close(fd_listen);
  close(epoll_fd);
  return 0;
}
