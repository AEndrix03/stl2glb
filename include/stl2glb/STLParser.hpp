#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace stl2glb {

    #pragma pack(push, 1)
    struct Triangle {
        float normal[3];
        float vertex1[3];
        float vertex2[3];
        float vertex3[3];
        uint16_t attributeByteCount; // Per compatibilità STL binario

        Triangle() : attributeByteCount(0) {
            // Inizializzazione inline per performance
            normal[0] = normal[1] = normal[2] = 0.0f;
        }
    } __attribute__((packed));
    #pragma pack(pop)

    static_assert(sizeof(Triangle) == 50, "Triangle struct must be exactly 50 bytes for STL binary format");

    class STLParser {
    public:
        static std::vector<Triangle> parse(const std::string& path);
    };

} // namespace stl2glb
