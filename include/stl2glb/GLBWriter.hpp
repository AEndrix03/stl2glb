#pragma once
#include <string>
#include <vector>
#include "STLParser.hpp"

namespace stl2glb {

    class GLBWriter {
    public:
        static void write(const std::vector<Triangle>& triangles,
                          const std::string& outputPath);
    };

} // namespace stl2glb
