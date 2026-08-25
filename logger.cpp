#include "logger.h"

int main() {
    // 1. Setup the logger options
    Logger::getInstance().initFile("app.log");
    Logger::getInstance().setLogLevel(LogLevel::DEBUG); // Allow debug logs

    // 2. Log messages via macros
    LOG_INFO("Application initialized successfully.");
    LOG_DEBUG("Database connection pool size set to 10.");
    LOG_WARN("High memory consumption detected.");
    LOG_ERROR("Failed to write data packet to port 8080.");

    return 0;
}
