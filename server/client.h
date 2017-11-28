#ifndef CLIENT_H
#define CLIENT_H
#include <consts.h>
int newClient(int lisneter, int epollFD, int* clientsFD, int& numClients);
int handleMessage(int* clients, int index, int& count);
#endif // CLIENT_H
