#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "httpserver.h"
#include "dbclient.h"
#include "logger.h"

bool SimpleHttpServer::init() {
    // Create a socket file descriptor (IPv4, TCP stream)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        Logger::getInstance().log(LogLevel::ERROR, "socket creation failed: %s", strerror(errno));
        return false;
        }
    
    // Allow immediate reuse of local address and port
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Define host properties
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces (0.0.0.0)
    address.sin_port = htons(port);       // Host-to-network short byte ordering
    
    // Bind the socket to our address structures
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::getInstance().log(LogLevel::ERROR, "bind failed: %s", strerror(errno));
        return false;
    }

    // Start listening with a maximum pending connection backlog queue of 10
    if (listen(server_fd, 10) < 0) {
        Logger::getInstance().log(LogLevel::ERROR, "listen failed: %s", strerror(errno));
        return false;
    }

    LOG_INFO("Starting rate limiter");
    rateLimiter = new RateLimiter();
    if (rateLimiter) {
        if (rateLimiter->start() == false) {
            LOG_ERROR("Unable to start rate limiter.");
        }
    } else {
        LOG_ERROR("Unable to allocate rate limiter.");
    }

    return true;
}

// operator()()
//     Accept TCP connetions and then spawn a client handler.

void SimpleHttpServer::operator()() {
    Logger::getInstance().log(LogLevel::INFO, "Server successfully listening on port %d  socket descriptor= %d", port, server_fd);

    std::vector<std::thread *> spawnedThreads;  // keeps track of client handler threads so we can free resources.
    
    while (true) {
        LOG_INFO("Waiting for a new connection...");
            
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            LOG_ERROR(strerror(errno));
            continue;
        }

        // Clean up any client threads that have stopped.
        for (auto it = spawnedThreads.begin(); it != spawnedThreads.end(); ) {
            if ((*it)->joinable()) {
                (*it)->join();
                delete *it;
                it = spawnedThreads.erase(it);
                Logger::getInstance().log(LogLevel::INFO, "Joined and then freed thread memory.");
            } else {
                it++;
            }
                
        }
        
        // Handle the connected user's HTTP request
        ClientResponder clientResponder;
        if (clientResponder.start(client_socket, rateLimiter) == false) {
            LOG_ERROR("Unable to start the client responder.");
        } else {
            spawnedThreads.push_back(clientResponder.getMyThread());
        }
    }
}

bool ClientResponder::fetchpeername(int client_fd, std::string &peername, AddressType &type) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    
    // Get the remote address associated with the socket
    if (getpeername(client_fd, (struct sockaddr*)&addr, &addr_len) == -1) {
        Logger::getInstance().log(LogLevel::ERROR, "getpeername failed: %s", strerror(errno));
        return false;
    }

    // Deal with both IPv4 and IPv6 protocols
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in* s = (struct sockaddr_in*)&addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
        peername = ip_str;
        type = v4;
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
        char ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
        peername = ip_str;
        type = v6;
    }
    return true;
}

void ClientResponder::operator()()
{
    char buffer[30000] = {0};
    bool isRateLimited = false;
    std::string peername;
    AddressType type;

    if (fetchpeername(clientSocket, peername, type) == false) {
        return;
    }

    // Read incoming client request data
    long bytes_read = read(clientSocket, buffer, 30000);
    if (bytes_read < 0) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed reading from client socket: %s", strerror(errno));
        return;
    }

    // It would be more efficient to store the peernames as numerical IP addresses but
    // converting the addresses to strings made the implementation easier.
    
    isRateLimited = rateLimiter->checkLimit(peername, type);
    
    // Basic HTTP response payload
    std::ostringstream response;
    if (isRateLimited) {
        std::string body = "Too many requests.\n";
        response << "HTTP/1.1 429 OK\r\n"
                 << "Content-Type: text/ascii\r\n"
                 << "Content-Length: " << body.length() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;
    } else {
        Dbclient dbclient;
        std::string body = dbclient.getRecords();
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/json\r\n"
                 << "Content-Length: " << body.length() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;
    }

    // Write the response back to the client socket
    if (send(clientSocket, response.str().c_str(), response.str().length(), 0) < 0) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed sending to client socket: %s", strerror(errno));
    }
        
    close(clientSocket);
}
