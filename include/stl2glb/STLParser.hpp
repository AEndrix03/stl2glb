#pragma once
#include <string>
#include <vector>

namespace stl2glb {

    struct Triangle {
        float normal[3];
        float vertex1[3];
        float vertex2[3];
        float vertex3[3];
    };

    class STLParser {
    public:
        static std::vector<Triangle> parse(const std::string& path);
    };

} // namespace stl2glb
