#include "stl2glb/Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace stl2glb {

    namespace {
        std::string timestamp() {
            auto now = std::chrono::system_clock::now();
            auto in_time = std::chrono::system_clock::to_time_t(now);
            std::ostringstream ss;
            ss << std::put_time(std::localtime(&in_time), "%Y-%m-%d %H:%M:%S");
            return ss.str();
        }

        void log(const std::string& level, const std::string& message) {
            std::cerr << "[" << timestamp() << "] [" << level << "] " << message << std::endl;
        }
    }

    void Logger::info(const std::string& message) {
        log("INFO", message);
    }

    void Logger::warn(const std::string& message) {
        log("WARN", message);
    }

    void Logger::error(const std::string& message) {
        log("ERROR", message);
    }

} // namespace stl2glb
