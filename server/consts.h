#ifndef CONSTS_H
#define CONSTS_H
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NUM_EVENTS 5000
#define MAX_CLIENTS 15000

enum errors {
    ERR_SOCK_CREATE = 1,
    ERR_NON_BLOCK,
    ERR_REUSEADDR,
    ERR_SOCK_BIND,
    ERR_SOCK_LISTEN,
    ERR_EPOLL_CREATE,
    ERR_ACCEPT_CLIENT,
    ERR_EPOLL_ADD,
    ERR_SOCK_READ
};

#endif // CONSTS_H
