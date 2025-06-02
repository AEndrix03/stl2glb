#include "stl2glb/STLParser.hpp"
#include "stl2glb/Logger.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <thread>
#include <vector>
#include <atomic>


// Platform-specific includes per memory mapping
#ifdef _WIN32
#include <windows.h>

#else
#include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace stl2glb {

    // Classe per memory-mapped file access
    class MemoryMappedFile {
    private:
        void* data = nullptr;
        size_t size = 0;

#ifdef _WIN32
        HANDLE fileHandle = INVALID_HANDLE_VALUE;
        HANDLE mappingHandle = nullptr;
#else
        int fd = -1;
#endif

    public:
        MemoryMappedFile(const std::string& path) {
#ifdef _WIN32
            fileHandle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                     nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle == INVALID_HANDLE_VALUE) {
                throw std::runtime_error("Failed to open file: " + path);
            }

            LARGE_INTEGER fileSize;
            if (!GetFileSizeEx(fileHandle, &fileSize)) {
                CloseHandle(fileHandle);
                throw std::runtime_error("Failed to get file size");
            }
            size = static_cast<size_t>(fileSize.QuadPart);

            mappingHandle = CreateFileMappingA(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
            if (!mappingHandle) {
                CloseHandle(fileHandle);
                throw std::runtime_error("Failed to create file mapping");
            }

            data = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, 0);
            if (!data) {
                CloseHandle(mappingHandle);
                CloseHandle(fileHandle);
                throw std::runtime_error("Failed to map file");
            }
#else
            fd = open(path.c_str(), O_RDONLY);
            if (fd == -1) {
                throw std::runtime_error("Failed to open file: " + path);
            }

            struct stat sb;
            if (fstat(fd, &sb) == -1) {
                close(fd);
                throw std::runtime_error("Failed to get file size");
            }
            size = static_cast<size_t>(sb.st_size);

            data = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (data == MAP_FAILED) {
                close(fd);
                throw std::runtime_error("Failed to map file");
            }

            // Hint al kernel per lettura sequenziale
            madvise(data, size, MADV_SEQUENTIAL);
#endif
        }

        ~MemoryMappedFile() {
#ifdef _WIN32
            if (data) UnmapViewOfFile(data);
            if (mappingHandle) CloseHandle(mappingHandle);
            if (fileHandle != INVALID_HANDLE_VALUE) CloseHandle(fileHandle);
#else
            if (data && data != MAP_FAILED) munmap(data, size);
            if (fd != -1) close(fd);
#endif
        }

        const void* getData() const { return data; }
        size_t getSize() const { return size; }
    };

    // In STLParser.cpp - Parser binario corretto
    class OptimizedBinaryParser {
    private:
        const uint8_t* data;
        size_t size;

        // Metodo corretto per leggere triangoli STL binari
        Triangle readTriangle(size_t offset) const {
            Triangle tri;

            // Usa la struttura raw per leggere esattamente 50 bytes
            STLTriangleRaw raw;
            std::memcpy(&raw, data + offset, sizeof(STLTriangleRaw));

            // Copia i dati nella struttura Triangle non-packed
            std::memcpy(tri.normal, raw.normal, sizeof(tri.normal));
            std::memcpy(tri.vertex1, raw.vertex1, sizeof(tri.vertex1));
            std::memcpy(tri.vertex2, raw.vertex2, sizeof(tri.vertex2));
            std::memcpy(tri.vertex3, raw.vertex3, sizeof(tri.vertex3));
            tri.attributeByteCount = raw.attributeByteCount;

            return tri;
        }

    public:
        OptimizedBinaryParser(const void* fileData, size_t fileSize)
                : data(static_cast<const uint8_t*>(fileData)), size(fileSize) {}

        std::vector<Triangle> parse() {
            if (size < 84) {
                throw std::runtime_error("STL file too small");
            }

            // Header STL binario (80 bytes) + numero triangoli (4 bytes)
            uint32_t numTriangles;
            std::memcpy(&numTriangles, data + 80, sizeof(uint32_t));

            Logger::info("Parsing STL with " + std::to_string(numTriangles) + " triangles");

            // Validazione dimensione file
            size_t expectedSize = 84 + (static_cast<size_t>(numTriangles) * 50);
            if (size < expectedSize) {
                throw std::runtime_error("STL file truncated. Expected " +
                                         std::to_string(expectedSize) + " bytes, got " + std::to_string(size));
            }

            std::vector<Triangle> triangles;
            triangles.reserve(numTriangles);

            // Parse sequenziale per evitare problemi di threading
            for (uint32_t i = 0; i < numTriangles; ++i) {
                size_t offset = 84 + (i * 50);
                Triangle tri = readTriangle(offset);

                // Validazione base dei dati
                if (!isValidTriangle(tri)) {
                    Logger::warn("Invalid triangle at index " + std::to_string(i) + ", skipping");
                    continue;
                }

                // Correggi normale se necessaria
                validateAndFixNormal(tri);
                triangles.push_back(tri);
            }

            Logger::info("Successfully parsed " + std::to_string(triangles.size()) + " valid triangles");
            return triangles;
        }

    private:
        bool isValidTriangle(const Triangle& tri) const {
            // Verifica che i vertici non siano NaN o infiniti
            for (int i = 0; i < 3; ++i) {
                if (!std::isfinite(tri.vertex1[i]) || !std::isfinite(tri.vertex2[i]) ||
                    !std::isfinite(tri.vertex3[i])) {
                    return false;
                }
            }

            // Verifica che il triangolo non sia degenere (area > 0)
            float v1[3] = {
                    tri.vertex2[0] - tri.vertex1[0],
                    tri.vertex2[1] - tri.vertex1[1],
                    tri.vertex2[2] - tri.vertex1[2]
            };
            float v2[3] = {
                    tri.vertex3[0] - tri.vertex1[0],
                    tri.vertex3[1] - tri.vertex1[1],
                    tri.vertex3[2] - tri.vertex1[2]
            };

            // Cross product per area
            float cross[3] = {
                    v1[1] * v2[2] - v1[2] * v2[1],
                    v1[2] * v2[0] - v1[0] * v2[2],
                    v1[0] * v2[1] - v1[1] * v2[0]
            };

            float area = std::sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
            return area > 1e-6f; // Triangolo valido se area > epsilon
        }

        void validateAndFixNormal(Triangle& tri) const {
            float normalLen = std::sqrt(
                    tri.normal[0] * tri.normal[0] +
                    tri.normal[1] * tri.normal[1] +
                    tri.normal[2] * tri.normal[2]
            );

            // Se la normale è invalida o zero, ricalcolala
            if (normalLen < 0.1f || !std::isfinite(normalLen)) {
                // Calcola normale dal prodotto vettoriale
                float v1[3] = {
                        tri.vertex2[0] - tri.vertex1[0],
                        tri.vertex2[1] - tri.vertex1[1],
                        tri.vertex2[2] - tri.vertex1[2]
                };
                float v2[3] = {
                        tri.vertex3[0] - tri.vertex1[0],
                        tri.vertex3[1] - tri.vertex1[1],
                        tri.vertex3[2] - tri.vertex1[2]
                };

                tri.normal[0] = v1[1] * v2[2] - v1[2] * v2[1];
                tri.normal[1] = v1[2] * v2[0] - v1[0] * v2[2];
                tri.normal[2] = v1[0] * v2[1] - v1[1] * v2[0];

                float len = std::sqrt(
                        tri.normal[0] * tri.normal[0] +
                        tri.normal[1] * tri.normal[1] +
                        tri.normal[2] * tri.normal[2]
                );

                if (len > 1e-6f) {
                    tri.normal[0] /= len;
                    tri.normal[1] /= len;
                    tri.normal[2] /= len;
                } else {
                    // Normale default se il triangolo è degenere
                    tri.normal[0] = 0.0f;
                    tri.normal[1] = 0.0f;
                    tri.normal[2] = 1.0f;
                }
            } else {
                // Normalizza la normale esistente
                tri.normal[0] /= normalLen;
                tri.normal[1] /= normalLen;
                tri.normal[2] /= normalLen;
            }
        }
    };

} // namespace stl2glb