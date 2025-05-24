#pragma once
#include <string>

namespace stl2glb {

    class Converter {
    public:
        static std::string run(const std::string& stl_hash);
    };

} // namespace stl2glb
