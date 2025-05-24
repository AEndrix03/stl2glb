#include "stl2glb/MinioClient.hpp"
#include "stl2glb/EnvironmentHandler.hpp"
#include <minio/minio.hpp>
#include <fstream>
#include <stdexcept>
#include <cstdlib>

using namespace minio;

namespace stl2glb {

    static std::shared_ptr<s3::Client> createClient() {
        auto& env = EnvironmentHandler::instance();

        s3::BaseUrl baseUrl{env.getMinioEndpoint()};
        s3::Credentials credentials{env.getMinioAccessKey(), env.getMinioSecretKey()};
        return std::make_shared<s3::Client>(baseUrl, credentials);
    }

    void MinioClient::download(const std::string& bucket,
                               const std::string& objectName,
                               const std::string& localPath) {
        auto client = createClient();
        s3::GetObjectRequest request{bucket, objectName};

        std::ofstream out(localPath, std::ios::binary);
        if (!out) throw std::runtime_error("Failed to open file for writing: " + localPath);

        auto result = client->GetObject(request, [&](const char* data, size_t size) {
            out.write(data, size);
            return true;
        });

        if (result.IsError()) {
            throw std::runtime_error("MinIO download error: " + result.ToString());
        }

        out.close();
    }

    void MinioClient::upload(const std::string& bucket,
                             const std::string& objectName,
                             const std::string& localPath) {
        auto client = createClient();

        std::ifstream in(localPath, std::ios::binary | std::ios::ate);
        if (!in) throw std::runtime_error("Failed to open file for reading: " + localPath);

        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);

        std::string content_type = "model/gltf-binary";

        s3::PutObjectRequest request{bucket, objectName, content_type, static_cast<uint64_t>(size)};
        auto result = client->PutObject(request, [&](size_t offset, size_t length, const s3::BufferCallback& cb) {
            std::vector<uint8_t> buffer(length);
            in.read(reinterpret_cast<char*>(buffer.data()), length);
            cb(buffer.data(), in.gcount());
            return true;
        });

        if (result.IsError()) {
            throw std::runtime_error("MinIO upload error: " + result.ToString());
        }

        in.close();
    }

} // namespace stl2glb
