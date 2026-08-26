#ifndef __RATELIMITER_H_
#define __RATELIMITER_H_

#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <csignal>
#include <condition_variable>

enum AddressType {v4, v6};

struct TimeoutInfo {
    AddressType type;
    std::string address;
    std::chrono::system_clock::time_point requestTime;

    TimeoutInfo(AddressType type, const std::string &address) : type(type), address(address) {
        requestTime = std::chrono::system_clock::now();
    }
};

class RateLimiter {
public:
    void operator()();
    void addTimeout(AddressType type, const std::string &address);
    bool start() {
        mythread = new std::thread(std::ref(*this));
        if (mythread == nullptr) {
            return false;
        }
        return true;
    }
    TimeoutInfo *find(AddressType type, const std::string &address);
    bool checkLimit(const std::string &address, AddressType type);
    
private:
    std::vector<TimeoutInfo *> vTimeouts;
    std::mutex timeoutMutex;
    std::thread *mythread;
    std::condition_variable timeoutCV;
    bool keep_running;
};

#endif
