#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile sig_atomic_t wasSigHup = 0;

static void sigHupHandler(int signo) {
    wasSigHup = 1;
}

int create_listen_socket(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        exit(1);
    }
    printf("Server listening on port %d...\n", port);
    return sockfd;
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigHupHandler;
    sigaction(SIGHUP, &sa, NULL);

    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blockedMask, &origMask) < 0) {
        perror("sigprocmask");
        exit(1);
    }

    int listenSock = create_listen_socket(9090);
    int clientSock = -1;

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenSock, &readfds);
        int maxfd = listenSock;

        if (clientSock != -1) {
            FD_SET(clientSock, &readfds);
            if (clientSock > maxfd) maxfd = clientSock;
        }

        int ret = pselect(maxfd+1, &readfds, NULL, NULL, NULL, &origMask);
        if (ret < 0) {
            if (errno == EINTR) {
                if (wasSigHup) {
                    printf("[Server] Received SIGHUP!\n");
                    wasSigHup = 0;
                }
                continue;
            } else {
                perror("pselect");
                break;
            }
        }

        if (FD_ISSET(listenSock, &readfds)) {
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            int newSock = accept(listenSock, (struct sockaddr*)&cli_addr, &clilen);
            if (newSock < 0) {
                perror("accept");
            } else {
                printf("[Server] New connection from %s:%d\n",
                       inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                if (clientSock == -1) {
                    clientSock = newSock;
                } else {
                    printf("[Server] Already have a client. Closing new one.\n");
                    close(newSock);
                }
            }
        }

        if (clientSock != -1 && FD_ISSET(clientSock, &readfds)) {
            char buf[1024];
            ssize_t n = recv(clientSock, buf, sizeof(buf), 0);
            if (n <= 0) {
                if (n == 0) {
                    printf("[Server] Client closed connection.\n");
                } else {
                    perror("recv");
                }
                close(clientSock);
                clientSock = -1;
            } else {
                printf("[Server] Received %zd bytes of data\n", n);
            }
        }

        // Kiểm tra cờ SIGHUP
        if (wasSigHup) {
            printf("[Server] Received SIGHUP!\n");
            wasSigHup = 0;
        }
    }

    if (clientSock != -1) close(clientSock);
    close(listenSock);
    return 0;
}
