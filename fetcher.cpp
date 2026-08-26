#include "fetcher.h"
#include "dbclient.h"
#include "logger.h"

#include <jansson.h>
#include <signal.h>
#include <unistd.h>

#include <chrono>
#include <iostream>

volatile sig_atomic_t keep_running = 1;

size_t Fetcher::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

void Fetcher::parseResponse(const std::string &resultsJson, std::vector<Pieces *> &records)
{
    json_t *root;
    json_error_t error;

    root = json_loads(resultsJson.c_str(), 0, &error);

    if (!root) {
        LOG_ERROR("Unable to parse json response from https://api.github.com/events");
        return;
    }

    if (json_is_array(root)) {
        size_t index;
        json_t *value;

        // Iterate through each element in the array
        json_array_foreach(root, index, value) {
            json_t *eventTypeJ = json_object_get(value, "type");
            std::string eventType;
            
            if (json_is_string(eventTypeJ)) {
                eventType = json_string_value(eventTypeJ);
            } else {
                LOG_ERROR("Attribute 'type' is not string.");
                json_decref(root);
                return;
            }

            if (eventType == "PushEvent") {
                json_t *repoJ = json_object_get(value, "repo");
                json_t *repoIdJ = json_object_get(repoJ, "id");
                uint64_t repoId;
            
                if (json_is_integer(repoIdJ)) {
                    repoId = json_integer_value(repoIdJ);
                } else {
                    LOG_ERROR("Repo identifier is not an integer.");
                    json_decref(root);
                    return;
                }

                json_t *repoUrlJ = json_object_get(repoJ, "url");
                std::string repoUrl;
                
                if (json_is_string(repoUrlJ)) {
                    repoUrl = json_string_value(repoUrlJ);
                } else {
                    LOG_ERROR("Attribute 'url' is not a string.");
                    json_decref(root);
                    return;
                }
                            
                json_t *payloadJ = json_object_get(value, "payload");
                json_t *pushIdJ = json_object_get(payloadJ, "push_id");
                uint64_t pushId;
            
                if (json_is_integer(pushIdJ)) {
                    pushId = json_integer_value(pushIdJ);
                } else {
                    LOG_ERROR("Attribute 'push_id' is not an integer.");
                    json_decref(root);
                    return;
                }

                json_t *refJ = json_object_get(payloadJ, "ref");
                std::string ref;

                if (json_is_string(refJ)) {
                    ref = json_string_value(refJ);
                } else {
                    LOG_ERROR("Attribute 'ref' is not a string.");
                    json_decref(root);
                    return;
                }
            
                json_t *headJ = json_object_get(payloadJ, "head");
                std::string head;

                if (json_is_string(headJ)) {
                    head = json_string_value(headJ);
                } else {
                    LOG_ERROR("Attribute 'head' is not a string.");
                    json_decref(root);
                    return;
                }
            
                json_t *beforeJ = json_object_get(payloadJ, "before");
                std::string before;

                if (json_is_string(beforeJ)) {
                    before = json_string_value(beforeJ);
                } else {
                    LOG_ERROR("Attribute 'before' is not a string.");
                    json_decref(root);
                    return;
                }
            
                json_t *actorJ = json_object_get(value, "actor");
                json_t *actorUrlJ = json_object_get(actorJ, "url");
                std::string actorUrl;

                if (json_is_string(actorUrlJ)) {
                    actorUrl = json_string_value(actorUrlJ);
                } else {
                    LOG_ERROR("Attribute 'actor.url' is not a string.");
                    json_decref(root);
                    return;
                }

                Pieces *newRecord = new Pieces(repoId, pushId, ref, head, before, repoUrl, actorUrl);
                if (newRecord == NULL) {
                    LOG_ERROR("Out of memory");
                    json_decref(root);
                    return;
                }
                records.push_back(newRecord);
            }
        }
    } else {
        LOG_ERROR("The json response is not an array.");
    }
        
    json_decref(root);
}

bool Fetcher::fetch(const std::string &url, std::string &readBuffer)
{
    CURLcode res;
    curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    // Set target URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Pass user agent header (required by some APIs like GitHub)
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // Set the callback function to capture the response data
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    // Perform the request
    res = curl_easy_perform(curl);
    
    if(res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    } else {
        // std::cout << "Response:\n" << readBuffer << std::endl;
    }

    // Clean up easy handle
    curl_easy_cleanup(curl);
    return true;
}

void Fetcher::operator()()
{
    CURLcode res;
    std::string readBuffer;

    // Initialize global curl environment
    curl_global_init(CURL_GLOBAL_DEFAULT);

    uint32_t iteration = 0;

    while (true) {
        std::unique_lock<std::mutex> stopLock(stopMutex);
        auto now = std::chrono::system_clock::now();

        // Request the events immediate on the first iteration. Thereafter, request events
        // once every 15 minutes so we are not constantly effected by github's rate
        // limiting protection.
        if (iteration != 0) {
            stopCV.wait_until(stopLock, now + std::chrono::seconds(1800), [this] {return keep_running == 0;});
        }
        iteration++;

        if (keep_running == 0) {
            break;
        }

        readBuffer.clear();
        
        curl = curl_easy_init();
        // Set target URL
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/events");
        // curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:4000/results.txt");
        
        // Pass user agent header (required by some APIs like GitHub)
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // Set the callback function to capture the response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);

        std::vector<Pieces *> records;
        // Check for errors
        if(res != CURLE_OK) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to fetch data from github server: ", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            continue;
        } else {
            std::cout << "Response:\n" << readBuffer << std::endl;
            parseResponse(readBuffer, records);
        }

        // Clean up easy handle
        curl_easy_cleanup(curl);

        for (const auto record : records) {
            Logger::getInstance().log(LogLevel::INFO, "Processing repo id= %ld push id= %ld.", record->repoId, record->pushId);
            
            readBuffer.clear();
            fetch(record->repoUrl, readBuffer);
            record->repoInfo = readBuffer;

            readBuffer.clear();
            fetch(record->actorUrl, readBuffer);
            record->actorInfo = readBuffer;
            
            // If we don't add the sleep, github will rate limit our fetches and we won't
            // be able to retrieve all of the desired data.
            sleep(1);
        }
        Dbclient dbclient;
        dbclient.write(records);

        // Free allocated memory.
        for (auto record: records) {
            delete record;
        }
    }

    // Global clean up
    curl_global_cleanup();
}

void Fetcher::stop()
{
    keep_running = 0;
    stopCV.notify_one();
}

bool Fetcher::start() {
    keep_running = 1;
    mythread = new std::thread(std::ref(*this));
    if (mythread == nullptr) {
        LOG_ERROR("Unable to allocate memory to run the fetcher.");
        return false;
    }
    return true;
}
