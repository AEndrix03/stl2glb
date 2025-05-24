#include "stl2glb/EnvironmentHandler.hpp"
#include <cstdlib>
#include <stdexcept>

namespace stl2glb {

    EnvironmentHandler& EnvironmentHandler::instance() {
        static EnvironmentHandler instance;
        return instance;
    }

    void EnvironmentHandler::init() {
        const char* port_env = std::getenv("STL2GLB_PORT");
        const char* stl_bucket = std::getenv("STL2GLB_STL_BUCKET_NAME");
        const char* glb_bucket = std::getenv("STL2GLB_GLB_BUCKET_NAME");
        const char* endpoint = std::getenv("STL2GLB_MINIO_ENDPOINT");
        const char* accessKey = std::getenv("STL2GLB_MINIO_ACCESS_KEY");
        const char* secretKey = std::getenv("STL2GLB_MINIO_SECRET_KEY");

        if (!stl_bucket || !glb_bucket || !endpoint || !accessKey || !secretKey) {
            throw std::runtime_error("Missing one or more required environment variables.");
        }

        if (port_env) {
            port = std::stoi(port_env);
            if (port < 1 || port > 65535)
                throw std::runtime_error("Invalid port number.");
        }

        stlBucketName = stl_bucket;
        glbBucketName = glb_bucket;
        minioEndpoint = endpoint;
        minioAccessKey = accessKey;
        minioSecretKey = secretKey;
    }

    int EnvironmentHandler::getPort() const {
        return port;
    }

    const std::string& EnvironmentHandler::getStlBucketName() const {
        return stlBucketName;
    }

    const std::string& EnvironmentHandler::getGlbBucketName() const {
        return glbBucketName;
    }

    const std::string& EnvironmentHandler::getMinioEndpoint() const {
        return minioEndpoint;
    }

    const std::string& EnvironmentHandler::getMinioAccessKey() const {
        return minioAccessKey;
    }

    const std::string& EnvironmentHandler::getMinioSecretKey() const {
        return minioSecretKey;
    }

} // namespace stl2glb
