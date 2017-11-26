#include "client.h"
#include "consts.h"
int closeSocket(int socket, char* msg)
{
    send(socket, msg, strlen(msg), 0);
    shutdown(socket, SHUT_WR);
    char buffer[1000];
    for (;;) {
        int res = read(socket, buffer, 1000);
        if (res < 0) {
            perror("Error read from socket");
            return ERR_SOCK_READ;
        }
        if (!res)
            break;
    }
    close(socket);
}

int newClient(int listener, int epollFD, int* clientsFD, int& numClients)
{
    struct epoll_event event;
    socklen_t addrlen;
    event.events = EPOLLIN | EPOLLET;

    struct sockaddr clientaddr;
    int newClientSocket;
    if (newClientSocket = accept(listener, (struct sockaddr*)&clientaddr, &addrlen) < 0) {
        perror("Error accept client");
        return -ERR_ACCEPT_CLIENT;
    }
    if (fcntl(newClientSocket, F_SETFL, fcntl(newClientSocket, F_GETFD, 0) | O_NONBLOCK) < 0) {
        perror("Error non block socket");
        return -ERR_NON_BLOCK;
    };
    int reuse = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(reuse), sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return -ERR_REUSEADDR;
    }
    if (numClients + 1 < MAX_CLIENTS) {
        event.data.fd = newClientSocket;
        if (epoll_ctl(epollFD, EPOLL_CTL_ADD, newClientSocket, &event) < 0) {
            perror("Error epoll add");
            return -ERR_EPOLL_ADD;
        }
        numClients++;
        clientsFD[numClients] = newClientSocket;

    } else {
        closeSocket(newClientSocket, "cant add client");
    }
}
