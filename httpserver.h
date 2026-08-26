#ifndef __SIMPLEHTTPSERVER_H_
#define __SIMPLEHTTPSERVER_H_

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ratelimiter.h"
#include "logger.h"

class ClientResponder {
public:
    void operator()();
    bool start(int clientSocket, RateLimiter *rateLimiter) {
        this->clientSocket = clientSocket;
        this->rateLimiter = rateLimiter;
        mythread = new std::thread(std::ref(*this));
        if (mythread == nullptr) {
            return false;
        }
        return true;
    }
    std::thread *getMyThread() {return mythread;}
    bool fetchpeername(int client_fd, std::string &peername, AddressType &type);

private:
    std::thread *mythread;
    int clientSocket;
    RateLimiter *rateLimiter;
};

class SimpleHttpServer {
private:
    int server_fd;
    int port;
    struct sockaddr_in address;
    int addrlen;

    std::thread *mythread;
    RateLimiter *rateLimiter;

public:
    SimpleHttpServer(int port_num) : port(port_num), server_fd(-1) {
        addrlen = sizeof(address);
    }

    ~SimpleHttpServer() {
        if (server_fd != -1) {
            close(server_fd);
        }
    }

    bool init();
    void operator()();
    bool start() {
        mythread = new std::thread(std::ref(*this));
        if (mythread == nullptr) {
            LOG_ERROR("Unable to allocate memory for the http server.");
            return false;
        }
        return true;
    }
     std::thread *getMyThread() {return mythread;}
};

#endif
