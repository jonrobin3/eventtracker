#include "ratelimiter.h"
#include "logger.h"

#include <iostream>

void RateLimiter::operator()()
{
    keep_running = true;
    LOG_INFO("Rate limiter up and running.");
    while (true) {
        std::unique_lock<std::mutex> timeoutLock(timeoutMutex);
        auto now = std::chrono::system_clock::now();
        timeoutCV.wait_until(timeoutLock, now + std::chrono::seconds(1), [this] {return keep_running == false;});

        if (keep_running == false) {
            break;
        }
 
        for (auto it = vTimeouts.begin(); it != vTimeouts.end(); ) {
            if (((*it)->requestTime + std::chrono::seconds(10)) < now) {
                Logger::getInstance().log(LogLevel::INFO, "%s: removed from rate limiting timeout", (*it)->address.c_str());
                it = vTimeouts.erase(it);
            } else {
                it++;
            }
        }
    }
}

TimeoutInfo *RateLimiter::find(AddressType type, const std::string &address)
{
     for (const auto ti : vTimeouts) {
        if (ti->type == type && ti->address == address) {
            return ti;
        }
    }
    return nullptr;
}

void RateLimiter::addTimeout(AddressType type, const std::string &address)
{
    TimeoutInfo *ti = new TimeoutInfo(type, address);
    vTimeouts.push_back(ti);    
}

bool RateLimiter::checkLimit(const std::string &peername, AddressType type)
{
   std::unique_lock<std::mutex> timeoutLock(timeoutMutex);
   TimeoutInfo *ti = find(type, peername);
   if (ti == NULL) {
       addTimeout(type, peername);
       return false;
   }
   return true;
}
