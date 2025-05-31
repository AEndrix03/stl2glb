#include "stl2glb/GLBWriter.hpp"
#include <tiny_gltf.h>
#include <stdexcept>
#include <unordered_map>
#include <array>
#include <cmath>
#include <limits>
#include <algorithm>
#include <memory>
#include <vector>
#include <fstream>
#include <cstring>
#include <filesystem>

// Include per compressione Draco se disponibile
#ifdef USE_DRACO
#include <draco/compression/encode.h>
#include <draco/mesh/mesh.h>
#include <draco/core/encoder_buffer.h>
#endif

namespace stl2glb {

    // Struttura ottimizzata per hash di vertici con spatial hashing
    class SpatialVertexHasher {
    private:
        static constexpr float GRID_SIZE = 0.001f; // 1mm di precisione
        static constexpr size_t HASH_TABLE_SIZE = 1 << 20; // 1M entries

        struct VertexEntry {
            std::array<float, 3> vertex;
            uint32_t index;
            VertexEntry* next;
        };

        std::vector<std::unique_ptr<VertexEntry>> entries;
        std::vector<VertexEntry*> hashTable;
        uint32_t nextIndex = 0;

        inline size_t computeHash(const std::array<float, 3>& v) const {
            int32_t x = static_cast<int32_t>(v[0] / GRID_SIZE);
            int32_t y = static_cast<int32_t>(v[1] / GRID_SIZE);
            int32_t z = static_cast<int32_t>(v[2] / GRID_SIZE);

            // MurmurHash-inspired mixing
            size_t h = x;
            h ^= y + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= z + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h & (HASH_TABLE_SIZE - 1);
        }

        inline bool vertexEqual(const std::array<float, 3>& a, const std::array<float, 3>& b) const {
            const float epsilon = GRID_SIZE * 0.5f;
            return std::abs(a[0] - b[0]) < epsilon &&
                   std::abs(a[1] - b[1]) < epsilon &&
                   std::abs(a[2] - b[2]) < epsilon;
        }

    public:
        SpatialVertexHasher() : hashTable(HASH_TABLE_SIZE, nullptr) {
            entries.reserve(100000); // Pre-alloca per performance
        }

        uint32_t addVertex(const std::array<float, 3>& vertex) {
            size_t hash = computeHash(vertex);
            VertexEntry* entry = hashTable[hash];

            // Cerca nella lista collegata
            while (entry) {
                if (vertexEqual(entry->vertex, vertex)) {
                    return entry->index;
                }
                entry = entry->next;
            }

            // Nuovo vertice
            entries.push_back(std::make_unique<VertexEntry>());
            VertexEntry* newEntry = entries.back().get();
            newEntry->vertex = vertex;
            newEntry->index = nextIndex++;
            newEntry->next = hashTable[hash];
            hashTable[hash] = newEntry;

            return newEntry->index;
        }

        uint32_t getVertexCount() const { return nextIndex; }
    };

    // Buffer writer ottimizzato per streaming
    class StreamingBufferWriter {
    private:
        std::string tempPath;
        std::ofstream file;
        size_t currentOffset = 0;

    public:
        StreamingBufferWriter(const std::string& outputPath)
                : tempPath(outputPath + ".tmp"), file(tempPath, std::ios::binary) {
            if (!file) {
                throw std::runtime_error("Failed to create temp buffer file");
            }
        }

        ~StreamingBufferWriter() {
            if (file.is_open()) {
                file.close();
            }
            // Cleanup temp file
            if (std::filesystem::exists(tempPath)) {
                std::filesystem::remove(tempPath);
            }
        }

        size_t writeData(const void* data, size_t size) {
            size_t offset = currentOffset;
            file.write(reinterpret_cast<const char*>(data), size);
            currentOffset += size;

            // Padding a 4 byte
            while (currentOffset % 4 != 0) {
                char zero = 0;
                file.write(&zero, 1);
                currentOffset++;
            }

            return offset;
        }

        void finish(std::vector<unsigned char>& buffer) {
            file.close();

            // Leggi tutto in memoria
            std::ifstream readFile(tempPath, std::ios::binary | std::ios::ate);
            if (!readFile) {
                throw std::runtime_error("Failed to read temp buffer file");
            }

            size_t size = readFile.tellg();
            buffer.resize(size);
            readFile.seekg(0);
            readFile.read(reinterpret_cast<char*>(buffer.data()), size);
            readFile.close();
        }

        size_t getCurrentOffset() const { return currentOffset; }
    };

    // Classe per gestire mesh grandi in chunks
    class ChunkedMeshProcessor {
    private:
        static constexpr size_t MAX_VERTICES_PER_CHUNK = 65000; // Lascia margine per uint16
        static constexpr size_t MAX_TRIANGLES_PER_BATCH = 10000;

        struct MeshChunk {
            std::vector<std::array<float, 3>> vertices;
            std::vector<std::array<float, 3>> normals;
            std::vector<uint32_t> indices;
            std::array<float, 3> minBounds;
            std::array<float, 3> maxBounds;

            MeshChunk() {
                minBounds = {std::numeric_limits<float>::max(),
                             std::numeric_limits<float>::max(),
                             std::numeric_limits<float>::max()};
                maxBounds = {std::numeric_limits<float>::lowest(),
                             std::numeric_limits<float>::lowest(),
                             std::numeric_limits<float>::lowest()};
            }
        };

        std::vector<MeshChunk> chunks;
        MeshChunk* currentChunk = nullptr;
        SpatialVertexHasher hasher;

        // Accumulator per normal smoothing
        std::unordered_map<uint32_t, std::pair<std::array<float, 3>, int>> normalAccumulator;

    public:
        void beginNewChunk() {
            chunks.emplace_back();
            currentChunk = &chunks.back();
            hasher = SpatialVertexHasher(); // Reset hasher per chunk
            normalAccumulator.clear();
        }

        void processTriangle(const Triangle& tri) {
            if (!currentChunk || hasher.getVertexCount() >= MAX_VERTICES_PER_CHUNK) {
                beginNewChunk();
            }

            std::array<std::array<float, 3>, 3> verts = {{
                                                                 {tri.vertex1[0], tri.vertex1[1], tri.vertex1[2]},
                                                                 {tri.vertex2[0], tri.vertex2[1], tri.vertex2[2]},
                                                                 {tri.vertex3[0], tri.vertex3[1], tri.vertex3[2]}
                                                         }};

            for (const auto& vert : verts) {
                uint32_t idx = hasher.addVertex(vert);

                // Se è un nuovo vertice
                if (idx == currentChunk->vertices.size()) {
                    currentChunk->vertices.push_back(vert);

                    // Aggiorna bounds
                    for (int i = 0; i < 3; ++i) {
                        currentChunk->minBounds[i] = std::min(currentChunk->minBounds[i], vert[i]);
                        currentChunk->maxBounds[i] = std::max(currentChunk->maxBounds[i], vert[i]);
                    }
                }

                currentChunk->indices.push_back(idx);

                // Accumula normale per smoothing
                if (normalAccumulator.find(idx) == normalAccumulator.end()) {
                    normalAccumulator[idx] = {{0, 0, 0}, 0};
                }
                auto& [normal, count] = normalAccumulator[idx];
                normal[0] += tri.normal[0];
                normal[1] += tri.normal[1];
                normal[2] += tri.normal[2];
                count++;
            }
        }

        void finalizeChunks() {
            for (auto& chunk : chunks) {
                chunk.normals.resize(chunk.vertices.size());

                // Calcola normali smooth
                for (const auto& [idx, data] : normalAccumulator) {
                    if (idx < chunk.normals.size()) {
                        auto& [normal, count] = data;
                        float len = std::sqrt(normal[0]*normal[0] +
                                              normal[1]*normal[1] +
                                              normal[2]*normal[2]);
                        if (len > 0) {
                            chunk.normals[idx] = {
                                    normal[0] / len,
                                    normal[1] / len,
                                    normal[2] / len
                            };
                        }
                    }
                }
            }
        }

        const std::vector<MeshChunk>& getChunks() const { return chunks; }
    };

#ifdef USE_DRACO
    // Funzione per comprimere mesh con Draco
    void compressMeshWithDraco(
        const std::vector<std::array<float, 3>>& vertices,
        const std::vector<std::array<float, 3>>& normals,
        const std::vector<uint32_t>& indices,
        std::vector<unsigned char>& compressedData) {

        draco::Mesh mesh;

        // Aggiungi vertici
        draco::PointAttribute posAttrib;
        posAttrib.Init(draco::GeometryAttribute::POSITION, 3, draco::DT_FLOAT32, false,
                      sizeof(float) * 3);
        int posAttId = mesh.AddAttribute(posAttrib, true, vertices.size());

        for (size_t i = 0; i < vertices.size(); ++i) {
            mesh.attribute(posAttId)->SetAttributeValue(
                draco::AttributeValueIndex(i), vertices[i].data());
        }

        // Aggiungi normali
        draco::PointAttribute normAttrib;
        normAttrib.Init(draco::GeometryAttribute::NORMAL, 3, draco::DT_FLOAT32, false,
                       sizeof(float) * 3);
        int normAttId = mesh.AddAttribute(normAttrib, true, normals.size());

        for (size_t i = 0; i < normals.size(); ++i) {
            mesh.attribute(normAttId)->SetAttributeValue(
                draco::AttributeValueIndex(i), normals[i].data());
        }

        // Aggiungi facce
        mesh.SetNumFaces(indices.size() / 3);
        for (size_t i = 0; i < indices.size(); i += 3) {
            draco::Mesh::Face face;
            face[0] = indices[i];
            face[1] = indices[i + 1];
            face[2] = indices[i + 2];
            mesh.SetFace(draco::FaceIndex(i / 3), face);
        }

        // Configura encoder
        draco::Encoder encoder;
        encoder.SetSpeedOptions(5, 5); // Bilancia velocità e compressione
        encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, 14);
        encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, 10);

        // Comprimi
        draco::EncoderBuffer buffer;
        draco::Status status = encoder.EncodeMeshToBuffer(mesh, &buffer);

        if (!status.ok()) {
            throw std::runtime_error("Draco compression failed: " + status.error_msg());
        }

        // Copia dati compressi
        compressedData.resize(buffer.size());
        std::memcpy(compressedData.data(), buffer.data(), buffer.size());
    }
#endif

    void GLBWriter::write(const std::vector<Triangle>& triangles,
                          const std::string& outputPath) {
        if (triangles.empty()) {
            throw std::runtime_error("No triangles to write");
        }

        // Usa processamento in chunks per file grandi
        ChunkedMeshProcessor processor;

        // Processa triangoli in batch per ridurre l'uso di memoria
        for (const auto& tri : triangles) {
            processor.processTriangle(tri);
        }
        processor.finalizeChunks();

        // Prepara il modello glTF
        tinygltf::Model model;
        tinygltf::Scene scene;
        model.scenes.push_back(scene);
        model.defaultScene = 0;

        // Asset info
        model.asset.version = "2.0";
        model.asset.generator = "STL2GLB Ultra-Optimized Converter";

#ifdef USE_DRACO
        // Aggiungi estensione Draco se disponibile
        model.extensionsRequired.push_back("KHR_draco_mesh_compression");
        model.extensionsUsed.push_back("KHR_draco_mesh_compression");
#endif

        // Buffer temporaneo su disco per file grandi
        StreamingBufferWriter bufferWriter(outputPath);

        // Processa ogni chunk
        const auto& chunks = processor.getChunks();
        for (size_t chunkIdx = 0; chunkIdx < chunks.size(); ++chunkIdx) {
            const auto& chunk = chunks[chunkIdx];

            // Crea nodo per questo chunk
            tinygltf::Node node;
            node.mesh = model.meshes.size();
            scene.nodes.push_back(model.nodes.size());
            model.nodes.push_back(node);

#ifdef USE_DRACO
            // Comprimi con Draco se disponibile
            std::vector<unsigned char> compressedData;
            compressMeshWithDraco(chunk.vertices, chunk.normals, chunk.indices, compressedData);

            size_t compressedOffset = bufferWriter.writeData(compressedData.data(), compressedData.size());

            // Crea buffer view per dati compressi
            tinygltf::BufferView compressedView;
            compressedView.buffer = 0;
            compressedView.byteOffset = compressedOffset;
            compressedView.byteLength = compressedData.size();
            model.bufferViews.push_back(compressedView);

            // Crea primitive con estensione Draco
            tinygltf::Primitive prim;
            prim.mode = TINYGLTF_MODE_TRIANGLES;
            prim.material = 0;

            // Aggiungi estensione Draco
            tinygltf::Value dracoExt(tinygltf::Value::Object());
            dracoExt.Get<tinygltf::Value::Object>()["bufferView"] =
                tinygltf::Value(static_cast<int>(model.bufferViews.size() - 1));

            tinygltf::Value attributes(tinygltf::Value::Object());
            attributes.Get<tinygltf::Value::Object>()["POSITION"] = tinygltf::Value(0);
            attributes.Get<tinygltf::Value::Object>()["NORMAL"] = tinygltf::Value(1);
            dracoExt.Get<tinygltf::Value::Object>()["attributes"] = attributes;

            prim.extensions["KHR_draco_mesh_compression"] = dracoExt;
#else
            // Scrivi dati non compressi
            size_t indicesOffset;
            int componentType;
            size_t indicesByteLength;

            if (chunk.vertices.size() <= 65535) {
                std::vector<uint16_t> indices16;
                indices16.reserve(chunk.indices.size());
                for (uint32_t idx : chunk.indices) {
                    indices16.push_back(static_cast<uint16_t>(idx));
                }
                indicesOffset = bufferWriter.writeData(indices16.data(),
                                                       indices16.size() * sizeof(uint16_t));
                componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
                indicesByteLength = indices16.size() * sizeof(uint16_t);
            } else {
                indicesOffset = bufferWriter.writeData(chunk.indices.data(),
                                                       chunk.indices.size() * sizeof(uint32_t));
                componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
                indicesByteLength = chunk.indices.size() * sizeof(uint32_t);
            }

            // Scrivi vertici
            size_t verticesOffset = bufferWriter.writeData(chunk.vertices.data(),
                                                           chunk.vertices.size() * sizeof(std::array<float, 3>));

            // Scrivi normali
            size_t normalsOffset = bufferWriter.writeData(chunk.normals.data(),
                                                          chunk.normals.size() * sizeof(std::array<float, 3>));

            // Crea buffer views
            int bufferViewBase = model.bufferViews.size();

            // Indices buffer view
            tinygltf::BufferView indicesView;
            indicesView.buffer = 0;
            indicesView.byteOffset = indicesOffset;
            indicesView.byteLength = indicesByteLength;
            indicesView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
            model.bufferViews.push_back(indicesView);

            // Vertices buffer view
            tinygltf::BufferView verticesView;
            verticesView.buffer = 0;
            verticesView.byteOffset = verticesOffset;
            verticesView.byteLength = chunk.vertices.size() * sizeof(std::array<float, 3>);
            verticesView.byteStride = sizeof(std::array<float, 3>);
            verticesView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            model.bufferViews.push_back(verticesView);

            // Normals buffer view
            tinygltf::BufferView normalsView;
            normalsView.buffer = 0;
            normalsView.byteOffset = normalsOffset;
            normalsView.byteLength = chunk.normals.size() * sizeof(std::array<float, 3>);
            normalsView.byteStride = sizeof(std::array<float, 3>);
            normalsView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            model.bufferViews.push_back(normalsView);

            // Crea accessors
            int accessorBase = model.accessors.size();

            // Indices accessor
            tinygltf::Accessor indicesAccessor;
            indicesAccessor.bufferView = bufferViewBase;
            indicesAccessor.byteOffset = 0;
            indicesAccessor.componentType = componentType;
            indicesAccessor.count = chunk.indices.size();
            indicesAccessor.type = TINYGLTF_TYPE_SCALAR;
            model.accessors.push_back(indicesAccessor);

            // Vertices accessor
            tinygltf::Accessor verticesAccessor;
            verticesAccessor.bufferView = bufferViewBase + 1;
            verticesAccessor.byteOffset = 0;
            verticesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            verticesAccessor.count = chunk.vertices.size();
            verticesAccessor.type = TINYGLTF_TYPE_VEC3;
            verticesAccessor.minValues = {chunk.minBounds[0], chunk.minBounds[1], chunk.minBounds[2]};
            verticesAccessor.maxValues = {chunk.maxBounds[0], chunk.maxBounds[1], chunk.maxBounds[2]};
            model.accessors.push_back(verticesAccessor);

            // Normals accessor
            tinygltf::Accessor normalsAccessor;
            normalsAccessor.bufferView = bufferViewBase + 2;
            normalsAccessor.byteOffset = 0;
            normalsAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            normalsAccessor.count = chunk.normals.size();
            normalsAccessor.type = TINYGLTF_TYPE_VEC3;
            model.accessors.push_back(normalsAccessor);

            // Crea primitive
            tinygltf::Primitive prim;
            prim.attributes["POSITION"] = accessorBase + 1;
            prim.attributes["NORMAL"] = accessorBase + 2;
            prim.indices = accessorBase;
            prim.mode = TINYGLTF_MODE_TRIANGLES;
            prim.material = 0;
#endif

            tinygltf::Mesh mesh;
            mesh.name = "STL_Mesh_Chunk_" + std::to_string(chunkIdx);
            mesh.primitives.push_back(prim);
            model.meshes.push_back(mesh);
        }

        // Crea materiale PBR ottimizzato
        tinygltf::Material material;
        material.name = "STL_Material";
        material.pbrMetallicRoughness.baseColorFactor = {0.9, 0.9, 0.9, 1.0};
        material.pbrMetallicRoughness.metallicFactor = 0.0;
        material.pbrMetallicRoughness.roughnessFactor = 0.5;
        material.doubleSided = false;
        model.materials.push_back(material);

        // Finalizza il buffer
        tinygltf::Buffer buffer;
        bufferWriter.finish(buffer.data);
        model.buffers.push_back(buffer);

        // Scrivi GLB con opzioni ottimizzate
        tinygltf::TinyGLTF gltf;
        std::string err, warn;

        bool result = gltf.WriteGltfSceneToFile(&model, outputPath,
                                                true, true, false, true);

        if (!result) {
            throw std::runtime_error("Failed to write GLB file: " + err + " " + warn);
        }
    }

} // namespace stl2glb