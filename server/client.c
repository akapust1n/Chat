#include "client.h"
#include "logger.h"
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

int newClient(int listener, int epollFD, int* clientsFD, int* numClients)
{
    struct epoll_event event;
    socklen_t addrlen;
    event.events = EPOLLIN | EPOLLET;

    struct sockaddr clientaddr;
    int newClientSocket;
    if ((newClientSocket = accept(listener, (struct sockaddr*)&clientaddr, &addrlen)) < 0) {
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
    if ((*numClients + 1) < MAX_CLIENTS) {
        event.data.fd = newClientSocket;
        if (epoll_ctl(epollFD, EPOLL_CTL_ADD, newClientSocket, &event) < 0) {
            perror("Error epoll add");
            return -ERR_EPOLL_ADD;
        }
        clientsFD[*numClients] = newClientSocket;
        *numClients += 1;
        int sentBytes = 0;
        char* msg = "Hello\n";
        int len = strlen(msg);
        while (sentBytes < len) {
            sentBytes += send(clientsFD[*numClients - 1], msg + sentBytes, len - sentBytes, 0);
        }
    } else {
        closeSocket(newClientSocket, "cant add client");
    }
}

void removeClient(int* clients, int index, int* count)
{

    memmove(clients + index, clients + index + 1, *count - index - 1);
    *count -= 1;
}
int handleMessage(int* clients, int authorSock, int* count, int logger)
{
    printf("HANDLE MESSAGE \n");

    char message[MSG_SIZE];
    bzero(message, MSG_SIZE);

    int len;

    if ((len = recv(authorSock, message, MSG_SIZE, 0)) == 0) {

        if (close(authorSock) < 0) {
            return -ERR_SOCK_CLOSE;
        }
        removeClient(clients, authorSock, count);
    } else {
        if (*count == 1) {
            const char* msg = "noone connected\n";
            int sentBytes = 0;
            int len = strlen(msg);
            while (sentBytes < len)
                sentBytes += send(authorSock, msg, strlen(msg), 0);

            return 1;
        } else {
            message[len] = '\n';
            message[len + 1] = '\0';
            writeLog(logger, authorSock, message);

            for (int i = 0; i < *count; i++) {
                if (clients[i] != authorSock) {
                    int sentBytes = 0;
                    while (sentBytes < len)
                        sentBytes += send(clients[i], message + sentBytes, len - sentBytes, 0);
                }
            }
        }
    }

    return len;
}
