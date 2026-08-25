#ifndef __FETCHER_H_
#define __FETCHER_H_

#include <condition_variable>
#include <thread>
#include <mutex>
#include <vector>

#include <curl/curl.h>

#include "pieces.h"

class Fetcher {
public:
    void operator()();
    void start();
    void stop();

    std::thread *getMyThread() {return mythread;}
private:
    static size_t  WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    void parseResponse(const std::string &results_json, std::vector<Pieces *> &records);
    bool fetch(const std::string &url, std::string &readBuffer);
    
    CURL* curl;
    std::thread *mythread;
    std::mutex stopMutex;
    std::condition_variable stopCV;
};

#endif
