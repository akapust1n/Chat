#include "client.h"
#include "logger.h"

void setWritable(int fd, struct user* clients, int count);
int writeToBuffer(char* msg, struct epoll_event event);
int handleNoone(int authorSock, char* message);
int handleEpollin(int authorSock, char* message);
int sendOthers(struct user* clients, struct epoll_event event, int* count, char* message, int len);

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
}

int newClient(int listener, int epollFD, struct user* clientsFD, int* numClients)
{
    printf("NEW client\n");

    struct epoll_event event;
    socklen_t addrlen;
    event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;

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
        event.data.ptr = (struct context*)malloc(sizeof(struct context));
        if (epoll_ctl(epollFD, EPOLL_CTL_ADD, newClientSocket, &event) < 0) {
            perror("Error epoll add");
            return -ERR_EPOLL_ADD;
        };
        struct user temp = { .fd = newClientSocket, .m_context = event.data.ptr };
        clientsFD[*numClients] = temp;
        *numClients += 1;
        int sentBytes = 0;
        char* msg = "Hello\n";
        int len = strlen(msg);
        while (sentBytes < len) {
            sentBytes += send(clientsFD[*numClients - 1].fd, msg + sentBytes, len - sentBytes, 0);
        }
    } else {
        closeSocket(newClientSocket, "cant add client");
    }
}

void removeClient(int* clients, int authorSock, int* count, struct epoll_event event)
{

    int i = 0;
    for (; i < *count && clients[i] != authorSock; i++)
        ;

    if (i == count)
        return -1;
    if (event.data.ptr)
        free(event.data.ptr);
    closeSocket(authorSock, "bye");
    memmove(clients + i, clients + i + 1, *count - i - 1);
    *count -= 1;
}

int handleMessage(struct user* clients, struct epoll_event event, int* count)
{
    printf("HANDLE MESSAGE \n");
    int authorSock = event.data.fd;

    if (event.events & EPOLLRDHUP || event.events & EPOLLHUP || event.events & EPOLLERR) {
        removeClient(clients, authorSock, count, event);
        return 0;
    }

    char message[MSG_SIZE];
    bzero(message, MSG_SIZE);
    int len;

    if (event.events & EPOLLIN) {
        if ((len = handleEpollin(authorSock, message)) < 1) {
            removeClient(clients, authorSock, count, event);
            return 0;
        }

        message[len] = '\0';
    }
    if (event.events & EPOLLOUT) {
        setWritable(authorSock, clients, count);
    }
    if (*count == 1) {
        const char* msg = "noone connected\n";
        handleNoone(authorSock, msg);
        return 0;
    } else {
        writeLog(authorSock, message);
    }
    sendOthers(clients, event, count, message, len);

    return len;
}
void setWritable(int fd, struct user* clients, int count)
{
    for (int i = 0; i < count; i++) {
        if (clients[i].fd == fd) {
            clients[i].m_context->writable = true;
            break;
        }
    }
}
int writeToBuffer(char* msg, struct epoll_event event)
{
    struct cycloBuffer* buffer = (struct cycloBuffer*)event.data.ptr;
    int datasize = strlen(msg);
    append(buffer, msg, datasize);
}
int handleNoone(int authorSock, char* message)
{
    int sentBytes = 0;
    int len = strlen(message);
    while (sentBytes < len) //если не доставилось - не критично
        sentBytes += send(authorSock, message, len - sentBytes, 0);
}

int handleEpollin(int authorSock, char* message)
{

    int recvBytes = 0;
    int numBytes = 0;
    while (recvBytes < MSG_SIZE - 2) {
        numBytes = recv(authorSock, message + recvBytes, MSG_SIZE - recvBytes, 0);
        if (numBytes == -1) {
            return -1;
        }
        recvBytes += numBytes;
    }
    return recvBytes;
}
int sendOthers(struct user* clients, struct epoll_event event, int* count, char* message, int len)
{
    int authorSock = event.data.fd;
    for (int i = 0; i < *count; i++) {
        if (clients[i].fd != authorSock) {
            writeToBuffer(message, event);
            int sentBytes = 0;
            int numBytes = 0;

            while (((struct context*)clients[i].m_context)->writable) { //while writable
                numBytes = send(clients[i].fd, message + sentBytes, len - sentBytes, 0);
                if (numBytes != -1) {
                    sentBytes += numBytes;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        ((struct context*)event.data.ptr)->writable = false;
                }
            }
            moveHead((&(clients[i].m_context)->buffer), numBytes);
        }
    }
    return 0;
}
