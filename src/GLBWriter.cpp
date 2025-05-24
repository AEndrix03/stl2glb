#include "stl2glb/GLBWriter.hpp"
#include "../external/tinygltf/tiny_gltf.h"
#include <stdexcept>

namespace stl2glb {

    void GLBWriter::write(const std::vector<Triangle>& triangles,
                          const std::string& outputPath) {
        tinygltf::Model model;
        tinygltf::Scene scene;
        scene.nodes.push_back(0);
        model.scenes.push_back(scene);
        model.defaultScene = 0;

        tinygltf::Node node;
        node.mesh = 0;
        model.nodes.push_back(node);

        std::vector<float> vertices;
        std::vector<float> normals;

        for (const auto& tri : triangles) {
            for (int i = 0; i < 3; ++i) {
                vertices.insert(vertices.end(), tri.vertex1, tri.vertex1 + 3);
                vertices.insert(vertices.end(), tri.vertex2, tri.vertex2 + 3);
                vertices.insert(vertices.end(), tri.vertex3, tri.vertex3 + 3);

                for (int j = 0; j < 3; ++j) {
                    normals.insert(normals.end(), tri.normal, tri.normal + 3);
                    normals.insert(normals.end(), tri.normal, tri.normal + 3);
                    normals.insert(normals.end(), tri.normal, tri.normal + 3);
                }
            }
        }

        tinygltf::Buffer buffer;
        std::vector<unsigned char> data;

        auto appendBuffer = [&](const std::vector<float>& source) {
            size_t start = data.size();
            const unsigned char* src = reinterpret_cast<const unsigned char*>(source.data());
            data.insert(data.end(), src, src + source.size() * sizeof(float));
            return start;
        };

        size_t vertex_offset = appendBuffer(vertices);
        size_t normal_offset = appendBuffer(normals);

        buffer.data = std::move(data);
        model.buffers.push_back(buffer);

        // BufferViews
        tinygltf::BufferView vertexView, normalView;
        vertexView.buffer = 0;
        vertexView.byteOffset = vertex_offset;
        vertexView.byteLength = vertices.size() * sizeof(float);
        vertexView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(vertexView);
        int vertexViewIndex = model.bufferViews.size() - 1;

        normalView.buffer = 0;
        normalView.byteOffset = normal_offset;
        normalView.byteLength = normals.size() * sizeof(float);
        normalView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(normalView);
        int normalViewIndex = model.bufferViews.size() - 1;

        // Accessors
        tinygltf::Accessor vertexAccessor, normalAccessor;
        vertexAccessor.bufferView = vertexViewIndex;
        vertexAccessor.byteOffset = 0;
        vertexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        vertexAccessor.count = vertices.size() / 3;
        vertexAccessor.type = TINYGLTF_TYPE_VEC3;
        model.accessors.push_back(vertexAccessor);
        int vertexAccessorIndex = model.accessors.size() - 1;

        normalAccessor.bufferView = normalViewIndex;
        normalAccessor.byteOffset = 0;
        normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        normalAccessor.count = normals.size() / 3;
        normalAccessor.type = TINYGLTF_TYPE_VEC3;
        model.accessors.push_back(normalAccessor);
        int normalAccessorIndex = model.accessors.size() - 1;

        // Primitive
        tinygltf::Primitive prim;
        prim.attributes["POSITION"] = vertexAccessorIndex;
        prim.attributes["NORMAL"] = normalAccessorIndex;
        prim.mode = TINYGLTF_MODE_TRIANGLES;

        tinygltf::Mesh mesh;
        mesh.primitives.push_back(prim);
        model.meshes.push_back(mesh);

        // Export
        tinygltf::TinyGLTF gltf;
        std::string err, warn;
        bool result = gltf.WriteGltfSceneToFile(&model, outputPath, true, true, true, true);

        if (!result) {
            throw std::runtime_error("Failed to write GLB file: " + err + " " + warn);
        }
    }

} // namespace stl2glb
