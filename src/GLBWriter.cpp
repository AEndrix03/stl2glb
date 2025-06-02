// GLBWriter.cpp semplificato e corretto
#include "stl2glb/GLBWriter.hpp"
#include "stl2glb/Logger.hpp"
#include <tiny_gltf.h>
#include <stdexcept>
#include <unordered_map>
#include <array>
#include <cmath>
#include <limits>
#include <algorithm>

namespace stl2glb {

    void GLBWriter::write(const std::vector<Triangle>& triangles, const std::string& outputPath) {
        if (triangles.empty()) {
            throw std::runtime_error("No triangles to write");
        }

        Logger::info("Writing GLB with " + std::to_string(triangles.size()) + " triangles");

        // Prepara il modello glTF
        tinygltf::Model model;
        tinygltf::Scene scene;
        tinygltf::Node node;
        tinygltf::Mesh mesh;
        tinygltf::Primitive primitive;
        tinygltf::Buffer buffer;
        tinygltf::Material material;

        // Imposta asset info
        model.asset.version = "2.0";
        model.asset.generator = "STL2GLB Converter";

        // Crea il materiale di default
        material.name = "STL_Material";
        material.pbrMetallicRoughness.baseColorFactor = {0.8, 0.8, 0.8, 1.0};
        material.pbrMetallicRoughness.metallicFactor = 0.1;
        material.pbrMetallicRoughness.roughnessFactor = 0.5;
        material.doubleSided = true; // Importante per STL
        model.materials.push_back(material);

        // Prepara i dati dei vertici
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<uint32_t> indices;

        // Mappa per deduplicare i vertici
        std::map<std::array<float, 3>, uint32_t> uniqueVertices;

        // Bounds per il calcolo del bounding box
        std::array<float, 3> minBounds = {
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()
        };
        std::array<float, 3> maxBounds = {
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()
        };

        // Processa i triangoli
        for (const auto& tri : triangles) {
            // Array di vertici del triangolo
            std::array<std::array<float, 3>, 3> triVerts = {{
                                                                    {tri.vertex1[0], tri.vertex1[1], tri.vertex1[2]},
                                                                    {tri.vertex2[0], tri.vertex2[1], tri.vertex2[2]},
                                                                    {tri.vertex3[0], tri.vertex3[1], tri.vertex3[2]}
                                                            }};

            // Processa ogni vertice
            for (const auto& vert : triVerts) {
                uint32_t index;

                // Controlla se il vertice esiste già
                auto it = uniqueVertices.find(vert);
                if (it != uniqueVertices.end()) {
                    index = it->second;
                } else {
                    // Nuovo vertice
                    index = static_cast<uint32_t>(vertices.size() / 3);
                    uniqueVertices[vert] = index;

                    // Aggiungi vertice
                    vertices.push_back(vert[0]);
                    vertices.push_back(vert[1]);
                    vertices.push_back(vert[2]);

                    // Aggiungi normale (usa la normale del triangolo)
                    normals.push_back(tri.normal[0]);
                    normals.push_back(tri.normal[1]);
                    normals.push_back(tri.normal[2]);

                    // Aggiorna bounds
                    for (int i = 0; i < 3; ++i) {
                        minBounds[i] = std::min(minBounds[i], vert[i]);
                        maxBounds[i] = std::max(maxBounds[i], vert[i]);
                    }
                }

                indices.push_back(index);
            }
        }

        Logger::info("Unique vertices: " + std::to_string(vertices.size() / 3));

        // Calcola gli offset e le dimensioni
        size_t vertexByteLength = vertices.size() * sizeof(float);
        size_t normalByteLength = normals.size() * sizeof(float);
        size_t indexByteLength = indices.size() * sizeof(uint32_t);

        size_t totalByteLength = vertexByteLength + normalByteLength + indexByteLength;

        // Padding per allineamento a 4 byte
        size_t vertexPadding = (4 - (vertexByteLength % 4)) % 4;
        size_t normalPadding = (4 - (normalByteLength % 4)) % 4;

        size_t vertexOffset = 0;
        size_t normalOffset = vertexOffset + vertexByteLength + vertexPadding;
        size_t indexOffset = normalOffset + normalByteLength + normalPadding;

        // Crea il buffer binario
        buffer.data.resize(totalByteLength + vertexPadding + normalPadding);

        // Copia i dati nel buffer
        std::memcpy(buffer.data.data() + vertexOffset, vertices.data(), vertexByteLength);
        std::memcpy(buffer.data.data() + normalOffset, normals.data(), normalByteLength);
        std::memcpy(buffer.data.data() + indexOffset, indices.data(), indexByteLength);

        model.buffers.push_back(buffer);

        // Crea i buffer views
        int vertexBufferViewIndex = model.bufferViews.size();
        tinygltf::BufferView vertexBufferView;
        vertexBufferView.buffer = 0;
        vertexBufferView.byteOffset = vertexOffset;
        vertexBufferView.byteLength = vertexByteLength;
        vertexBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(vertexBufferView);

        int normalBufferViewIndex = model.bufferViews.size();
        tinygltf::BufferView normalBufferView;
        normalBufferView.buffer = 0;
        normalBufferView.byteOffset = normalOffset;
        normalBufferView.byteLength = normalByteLength;
        normalBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(normalBufferView);

        int indexBufferViewIndex = model.bufferViews.size();
        tinygltf::BufferView indexBufferView;
        indexBufferView.buffer = 0;
        indexBufferView.byteOffset = indexOffset;
        indexBufferView.byteLength = indexByteLength;
        indexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        model.bufferViews.push_back(indexBufferView);

        // Crea gli accessors
        int positionAccessorIndex = model.accessors.size();
        tinygltf::Accessor positionAccessor;
        positionAccessor.bufferView = vertexBufferViewIndex;
        positionAccessor.byteOffset = 0;
        positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        positionAccessor.count = vertices.size() / 3;
        positionAccessor.type = TINYGLTF_TYPE_VEC3;
        positionAccessor.minValues = {minBounds[0], minBounds[1], minBounds[2]};
        positionAccessor.maxValues = {maxBounds[0], maxBounds[1], maxBounds[2]};
        model.accessors.push_back(positionAccessor);

        int normalAccessorIndex = model.accessors.size();
        tinygltf::Accessor normalAccessor;
        normalAccessor.bufferView = normalBufferViewIndex;
        normalAccessor.byteOffset = 0;
        normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        normalAccessor.count = normals.size() / 3;
        normalAccessor.type = TINYGLTF_TYPE_VEC3;
        model.accessors.push_back(normalAccessor);

        int indexAccessorIndex = model.accessors.size();
        tinygltf::Accessor indexAccessor;
        indexAccessor.bufferView = indexBufferViewIndex;
        indexAccessor.byteOffset = 0;
        indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
        indexAccessor.count = indices.size();
        indexAccessor.type = TINYGLTF_TYPE_SCALAR;
        model.accessors.push_back(indexAccessor);

        // Configura la primitive
        primitive.attributes["POSITION"] = positionAccessorIndex;
        primitive.attributes["NORMAL"] = normalAccessorIndex;
        primitive.indices = indexAccessorIndex;
        primitive.material = 0;
        primitive.mode = TINYGLTF_MODE_TRIANGLES;

        // Assembla mesh, node e scene
        mesh.primitives.push_back(primitive);
        model.meshes.push_back(mesh);

        node.mesh = 0;
        model.nodes.push_back(node);

        scene.nodes.push_back(0);
        model.scenes.push_back(scene);
        model.defaultScene = 0;

        // Scrivi il file GLB
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ret = loader.WriteGltfSceneToFile(
                &model,
                outputPath,
                true,    // embedImages
                true,    // embedBuffers
                true,    // prettyPrint
                true     // writeBinary (GLB)
        );

        if (!ret) {
            throw std::runtime_error("Failed to write GLB file: " + err);
        }

        if (!warn.empty()) {
            Logger::warn("GLB write warnings: " + warn);
        }

        Logger::info("Successfully wrote GLB file: " + outputPath);
    }

} // namespace stl2glb