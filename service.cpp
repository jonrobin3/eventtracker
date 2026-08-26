#include <thread>

#include <signal.h>
#include <unistd.h>

#include "dbclient.h"
#include "fetcher.h"
#include "httpserver.h"
#include "logger.h"

Fetcher *fetcher = nullptr;

// The custom signal handler function
void handle_sigterm(int signum) {
    if (signum == SIGTERM) {
        fetcher->stop();
    }
}

bool initSignalHandler() {
    struct sigaction sa;

    sa.sa_handler = handle_sigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error registering SIGTERM handler");
        return false;
    }

    return true;
}

int main()
{
    Logger::getInstance().initFile("app.log");
    Logger::getInstance().setLogLevel(LogLevel::DEBUG); // Allow debug logs
    LOG_INFO("Service up and running...");

    if (initSignalHandler() == false) {
        exit(1);
    }
    
    Dbclient dbclient;
    if (dbclient.createTable() == false) {
        exit(1);
    }
    
    fetcher = new Fetcher();
    if (fetcher->start() == false) {
        exit(1);
    }

    SimpleHttpServer server(8080);
    if (server.init()) {
        if (server.start() == false) {
            exit(1);
        }
    } else {
        LOG_ERROR("Unable to start http server.");
        fetcher->stop();
        fetcher->getMyThread()->join();
        exit(1);
    }


    fetcher->getMyThread()->join();
    return 0;
}
