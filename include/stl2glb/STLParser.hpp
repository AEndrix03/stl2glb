#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

namespace stl2glb {

    struct Triangle {
        float normal[3];
        float vertex1[3];
        float vertex2[3];
        float vertex3[3];
        uint16_t attributeByteCount;

        Triangle() : attributeByteCount(0) {
            std::memset(normal, 0, sizeof(normal));
            std::memset(vertex1, 0, sizeof(vertex1));
            std::memset(vertex2, 0, sizeof(vertex2));
            std::memset(vertex3, 0, sizeof(vertex3));
        }
    };

#pragma pack(push, 1)
    struct STLTriangleRaw {
        float normal[3];
        float vertex1[3];
        float vertex2[3];
        float vertex3[3];
        uint16_t attributeByteCount;
    } __attribute__((packed));
#pragma pack(pop)

    static_assert(sizeof(STLTriangleRaw) == 50, "STLTriangleRaw must be exactly 50 bytes");
}
