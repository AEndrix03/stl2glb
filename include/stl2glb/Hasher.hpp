#pragma once
#include <string>

namespace stl2glb {

    class Hasher {
    public:
        static std::string sha256_file(const std::string& path);
    };

} // namespace stl2glb
