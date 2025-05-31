#include "stl2glb/EnvironmentHandler.hpp"
#include <cstdlib>
#include <stdexcept>

namespace stl2glb {

    EnvironmentHandler& EnvironmentHandler::instance() {
        static EnvironmentHandler instance;
        return instance;
    }

    void EnvironmentHandler::init() {
        /*const char* stl_bucket = std::getenv("STL2GLB_STL_BUCKET_NAME");
        const char* glb_bucket = std::getenv("STL2GLB_GLB_BUCKET_NAME");
        const char* endpoint = std::getenv("STL2GLB_MINIO_ENDPOINT");
        const char* accessKey = std::getenv("STL2GLB_MINIO_ACCESS_KEY");
        const char* secretKey = std::getenv("STL2GLB_MINIO_SECRET_KEY");

        if (!stl_bucket || !glb_bucket || !endpoint || !accessKey || !secretKey) {
            throw std::runtime_error("Missing one or more required environment variables.");
        }

        stlBucketName = stl_bucket;
        glbBucketName = glb_bucket;
        minioEndpoint = endpoint;
        minioAccessKey = accessKey;
        minioSecretKey = secretKey;*/

        stlBucketName = "printer_model";
        glbBucketName = "printer_glb_model";
        minioEndpoint = "http://minio.aredegalli.it:7800";
        minioAccessKey = "admin";
        minioSecretKey = "k9ef8g74IlGXiTlO114vKU79fOAy6aPU";
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
