#include "stl2glb/STLParser.hpp"
#include <fstream>
#include <stdexcept>
#include <cstdint>

namespace stl2glb {

    std::vector<Triangle> STLParser::parse(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Unable to open STL file: " + path);
        }

        file.ignore(80);

        uint32_t numTriangles = 0;
        file.read(reinterpret_cast<char*>(&numTriangles), sizeof(uint32_t));

        std::vector<Triangle> triangles(numTriangles);
        for (uint32_t i = 0; i < numTriangles; ++i) {
            file.read(reinterpret_cast<char*>(&triangles[i]), sizeof(Triangle));
            if (!file) {
                throw std::runtime_error("Unexpected end of file while reading triangle #" + std::to_string(i));
            }
        }

        return triangles;
    }

} // namespace stl2glb
