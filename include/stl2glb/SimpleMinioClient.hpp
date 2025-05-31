#pragma once

#include <string>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <httplib.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include "stl2glb/Logger.hpp"
#include <vector>

namespace stl2glb {

    class SimpleMinioClient {
    private:
        static std::string endpoint;
        static std::string accessKey;
        static std::string secretKey;
        static std::mutex clientMutex;
        static bool initialized;

        static void initialize();

        // Helper functions for AWS Signature V4
        static std::string urlEncode(const std::string& value, bool encodeSlash = true);
        static std::string sha256(const std::string& data);
        static std::string hmacSha256(const std::string& key, const std::string& data);
        static std::vector<unsigned char> hmacSha256Raw(const std::string& key, const std::string& data);
        static std::string getAmzDate();
        static std::string getDateStamp();
        static httplib::Headers createAwsV4Headers(
                const std::string& method,
                const std::string& path,
                const std::string& payload,
                const std::string& contentType = ""
        );

        static void ensureDirectoryExists(const std::string& filePath);
        static bool ensureBucketExists(const std::string& bucketName);
        static bool createBucket(const std::string& bucketName);

    public:
        static void download(const std::string& bucket,
                             const std::string& objectName,
                             const std::string& localPath);

        static void upload(const std::string& bucket,
                           const std::string& objectName,
                           const std::string& localPath);
    };

} // namespace stl2glb