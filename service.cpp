#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>
#include <jansson.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dbclient.h"
#include "fetcher.h"
#include "httpserver.h"
#include "logger.h"
#include "pieces.h"

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
        return 1;
    }
    
    Dbclient dbclient;
    dbclient.createTable();
    
    fetcher = new Fetcher();
    fetcher->start();

    SimpleHttpServer server(8080);
    if (server.init()) {
        server.start();
    } else {
        LOG_ERROR("Unable to start http server.");
        fetcher->stop();
        return 1;
    }


    fetcher->getMyThread()->join();
    return 0;
}
