#include "stl2glb/STLParser.hpp"
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

    // Parser binario ottimizzato con SIMD dove possibile
    class OptimizedBinaryParser {
    private:
        const uint8_t* data;
        size_t size;

        // Legge float con gestione endianness
        inline float readFloat(size_t offset) const {
            float value;
            std::memcpy(&value, data + offset, sizeof(float));
            return value;
        }

        // Legge uint32 con gestione endianness
        inline uint32_t readUint32(size_t offset) const {
            uint32_t value;
            std::memcpy(&value, data + offset, sizeof(uint32_t));
            return value;
        }

    public:
        OptimizedBinaryParser(const void* fileData, size_t fileSize)
                : data(static_cast<const uint8_t*>(fileData)), size(fileSize) {}

        std::vector<Triangle> parse() {
            if (size < 84) {
                throw std::runtime_error("STL file too small");
            }

            // Leggi numero di triangoli
            uint32_t numTriangles = readUint32(80);

            // Validazione
            size_t expectedSize = 84 + (numTriangles * 50);
            if (size < expectedSize) {
                throw std::runtime_error("STL file truncated");
            }

            std::vector<Triangle> triangles;
            triangles.reserve(numTriangles);

            // Parse parallelo per file grandi
            const size_t CHUNK_SIZE = 10000;
            if (numTriangles > CHUNK_SIZE * 4) {
                // Usa thread multipli
                unsigned int numThreads = std::thread::hardware_concurrency();
                if (numThreads == 0) numThreads = 4;

                std::vector<std::thread> threads;
                std::vector<std::vector<Triangle>> threadResults(numThreads);
                std::atomic<size_t> nextChunk(0);

                for (unsigned int t = 0; t < numThreads; ++t) {
                    threads.emplace_back([&, t]() {
                        std::vector<Triangle>& localTriangles = threadResults[t];
                        localTriangles.reserve(numTriangles / numThreads + CHUNK_SIZE);

                        size_t chunkIdx;
                        while ((chunkIdx = nextChunk.fetch_add(1)) * CHUNK_SIZE < numTriangles) {
                            size_t start = chunkIdx * CHUNK_SIZE;
                            size_t end = std::min(start + CHUNK_SIZE, static_cast<size_t>(numTriangles));

                            for (size_t i = start; i < end; ++i) {
                                size_t offset = 84 + i * 50;
                                Triangle tri;

                                // Leggi normale
                                tri.normal[0] = readFloat(offset);
                                tri.normal[1] = readFloat(offset + 4);
                                tri.normal[2] = readFloat(offset + 8);

                                // Leggi vertici
                                tri.vertex1[0] = readFloat(offset + 12);
                                tri.vertex1[1] = readFloat(offset + 16);
                                tri.vertex1[2] = readFloat(offset + 20);

                                tri.vertex2[0] = readFloat(offset + 24);
                                tri.vertex2[1] = readFloat(offset + 28);
                                tri.vertex2[2] = readFloat(offset + 32);

                                tri.vertex3[0] = readFloat(offset + 36);
                                tri.vertex3[1] = readFloat(offset + 40);
                                tri.vertex3[2] = readFloat(offset + 44);

                                // Valida e correggi normale se necessario
                                validateAndFixNormal(tri);

                                localTriangles.push_back(tri);
                            }
                        }
                    });
                }

                // Attendi tutti i thread
                for (auto& thread : threads) {
                    thread.join();
                }

                // Combina risultati
                for (const auto& threadResult : threadResults) {
                    triangles.insert(triangles.end(), threadResult.begin(), threadResult.end());
                }
            } else {
                // Parse sequenziale per file piccoli
                for (uint32_t i = 0; i < numTriangles; ++i) {
                    size_t offset = 84 + i * 50;
                    Triangle tri;

                    // Copia diretta della struttura (più veloce)
                    std::memcpy(&tri.normal, data + offset, 12);
                    std::memcpy(&tri.vertex1, data + offset + 12, 12);
                    std::memcpy(&tri.vertex2, data + offset + 24, 12);
                    std::memcpy(&tri.vertex3, data + offset + 36, 12);

                    validateAndFixNormal(tri);
                    triangles.push_back(tri);
                }
            }

            return triangles;
        }

    private:
        inline void validateAndFixNormal(Triangle& tri) const {
            float normalLen = std::sqrt(tri.normal[0] * tri.normal[0] +
                                        tri.normal[1] * tri.normal[1] +
                                        tri.normal[2] * tri.normal[2]);

            if (normalLen < 0.1f) {
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

                float len = std::sqrt(tri.normal[0] * tri.normal[0] +
                                      tri.normal[1] * tri.normal[1] +
                                      tri.normal[2] * tri.normal[2]);

                if (len > 0) {
                    tri.normal[0] /= len;
                    tri.normal[1] /= len;
                    tri.normal[2] /= len;
                }
            }
        }
    };

    // Parser ASCII ottimizzato
    class OptimizedASCIIParser {
    private:
        const char* data;
        size_t size;

        // Fast float parsing
        inline bool fastParseFloat(const char*& ptr, float& value) {
            char* end;
            value = std::strtof(ptr, &end);
            if (end == ptr) return false;
            ptr = end;
            return true;
        }

        // Skip whitespace
        inline void skipWhitespace(const char*& ptr) {
            while (*ptr && std::isspace(*ptr)) ++ptr;
        }

    public:
        OptimizedASCIIParser(const void* fileData, size_t fileSize)
                : data(static_cast<const char*>(fileData)), size(fileSize) {}

        std::vector<Triangle> parse() {
            std::vector<Triangle> triangles;
            triangles.reserve(size / 200); // Stima approssimativa

            const char* ptr = data;
            const char* end = data + size;

            Triangle currentTriangle;
            int vertexCount = 0;
            bool inFacet = false;

            while (ptr < end) {
                skipWhitespace(ptr);

                // Cerca keywords
                if (std::strncmp(ptr, "facet normal", 12) == 0) {
                    ptr += 12;
                    skipWhitespace(ptr);

                    // FIX: Usa variabili temporanee per evitare binding di campi packed
                    float nx, ny, nz;
                    fastParseFloat(ptr, nx);
                    fastParseFloat(ptr, ny);
                    fastParseFloat(ptr, nz);

                    currentTriangle.normal[0] = nx;
                    currentTriangle.normal[1] = ny;
                    currentTriangle.normal[2] = nz;

                    inFacet = true;
                    vertexCount = 0;
                } else if (std::strncmp(ptr, "vertex", 6) == 0 && inFacet) {
                    ptr += 6;
                    skipWhitespace(ptr);

                    float x, y, z;
                    if (fastParseFloat(ptr, x) && fastParseFloat(ptr, y) && fastParseFloat(ptr, z)) {
                        switch (vertexCount) {
                            case 0:
                                currentTriangle.vertex1[0] = x;
                                currentTriangle.vertex1[1] = y;
                                currentTriangle.vertex1[2] = z;
                                break;
                            case 1:
                                currentTriangle.vertex2[0] = x;
                                currentTriangle.vertex2[1] = y;
                                currentTriangle.vertex2[2] = z;
                                break;
                            case 2:
                                currentTriangle.vertex3[0] = x;
                                currentTriangle.vertex3[1] = y;
                                currentTriangle.vertex3[2] = z;
                                break;
                        }
                        vertexCount++;
                    }
                } else if (std::strncmp(ptr, "endfacet", 8) == 0 && inFacet) {
                    if (vertexCount == 3) {
                        validateAndFixNormal(currentTriangle);
                        triangles.push_back(currentTriangle);
                    }
                    inFacet = false;
                    ptr += 8;
                }

                // Vai alla prossima linea
                while (ptr < end && *ptr != '\n') ++ptr;
                if (ptr < end) ++ptr;
            }

            return triangles;
        }

    private:
        void validateAndFixNormal(Triangle& tri) const {
            // Stessa logica del parser binario
            float normalLen = std::sqrt(tri.normal[0] * tri.normal[0] +
                                        tri.normal[1] * tri.normal[1] +
                                        tri.normal[2] * tri.normal[2]);

            if (normalLen < 0.1f) {
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

                float len = std::sqrt(tri.normal[0] * tri.normal[0] +
                                      tri.normal[1] * tri.normal[1] +
                                      tri.normal[2] * tri.normal[2]);

                if (len > 0) {
                    tri.normal[0] /= len;
                    tri.normal[1] /= len;
                    tri.normal[2] /= len;
                }
            }
        }
    };

    // Rileva se il file è ASCII controllando solo l'inizio
    bool isASCII(const void* data, size_t size) {
        if (size < 6) return false;

        const char* charData = static_cast<const char*>(data);

        // Se inizia con "solid "
        if (std::strncmp(charData, "solid ", 6) == 0) {
            // Controlla i primi 1KB per caratteri non-ASCII
            size_t checkSize = std::min(size, size_t(1024));
            for (size_t i = 0; i < checkSize; ++i) {
                unsigned char c = static_cast<unsigned char>(charData[i]);
                if (c > 127 || (c < 32 && c != '\r' && c != '\n' && c != '\t')) {
                    return false;
                }
            }
            return true;
        }

        return false;
    }

    std::vector<Triangle> STLParser::parse(const std::string& path) {
        try {
            // Usa memory mapping per performance ottimale
            MemoryMappedFile mmFile(path);

            if (isASCII(mmFile.getData(), mmFile.getSize())) {
                OptimizedASCIIParser parser(mmFile.getData(), mmFile.getSize());
                return parser.parse();
            } else {
                OptimizedBinaryParser parser(mmFile.getData(), mmFile.getSize());
                return parser.parse();
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to parse STL file: " + std::string(e.what()));
        }
    }

} // namespace stl2glb