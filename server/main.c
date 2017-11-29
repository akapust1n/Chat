#include "client.h"
#include "consts.h"

int startListen(int port)
{
    int listener;
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error socker create");
        exit(-ERR_SOCK_CREATE);
    }
    if (fcntl(listener, F_SETFL, fcntl(listener, F_GETFD, 0) | O_NONBLOCK) < 0) {
        perror("Error non block socket");
        exit(-ERR_NON_BLOCK);
    };
    int reuse = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(reuse), sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(-ERR_REUSEADDR);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error socket bind");
        exit(-ERR_SOCK_BIND);
    }
    if (listen(listener, SOMAXCONN) < 0) {
        perror("Socket listen");
        exit(-ERR_SOCK_LISTEN);
    }
    printf("Start listen\n");
    return listener;
}
int createEpoll(int listenFD)
{
    int epollFD;

    if ((epollFD = epoll_create(1)) < 0) {
        perror("Error epoll create");
        exit(-ERR_EPOLL_CREATE);
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = listenFD;
    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, listenFD, &event) < 0) {
        perror("Epoll fd add");
        exit(-ERR_EPOLL_ADD);
    }
    return epollFD;
}
int main(int argc, char* argv[])
{
    int listener = startListen(8080);
    int epollFD = createEpoll(listener);
    int logger = open(LOGFILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    struct epoll_event events[NUM_EVENTS];
    int clientsFD[MAX_CLIENTS];
    int numFD, numClients = 0;

    while (1) {
        if ((numFD = epoll_wait(epollFD, events, NUM_EVENTS, -1)) < 0) {
            perror("Error epoll_wait");
            // exit(-9);
        }
        for (int i = 0; i < numFD; i++) {
            if (events[i].data.fd == listener) {
                if (newClient(listener, epollFD, clientsFD, &numClients) < 0) {
                    printf("NEW client\n");
                    perror("Can create new client!");
                }
            } else {
                int authorSock = events[i].data.fd;
                if (handleMessage(clientsFD, authorSock, &numClients, logger) < 0) {
                    perror("Canе handle message!");
                }
            }
        }
    }
    close(logger);
}
