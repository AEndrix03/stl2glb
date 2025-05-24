#pragma once
#include <string>

namespace stl2glb {

    class Logger {
    public:
        static void info(const std::string& message);
        static void warn(const std::string& message);
        static void error(const std::string& message);
    };

} // namespace stl2glb
