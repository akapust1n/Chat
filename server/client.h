#ifndef CLIENT_H
#define CLIENT_H
int newClient(int lisneter, int epollFD, int* clientsFD, int& numClients);

#endif // CLIENT_H
