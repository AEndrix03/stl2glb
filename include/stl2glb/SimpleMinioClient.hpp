#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <httplib.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <ctime>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>

#include "stl2glb/Logger.hpp"

namespace stl2glb {

/**
 * @class SimpleMinioClient
 * @brief Implementazione semplificata di un client MinIO che usa direttamente httplib
 * 
 * Questa classe fornisce una implementazione basic del client MinIO che
 * non dipende dalla libreria ufficiale ma usa httplib per le richieste HTTP
 * e OpenSSL per la firma delle richieste.
 */
    class SimpleMinioClient {
    public:
        static void download(const std::string& bucket,
                             const std::string& objectName,
                             const std::string& localPath);

        static void upload(const std::string& bucket,
                           const std::string& objectName,
                           const std::string& localPath);

    private:
        static std::string endpoint;
        static std::string accessKey;
        static std::string secretKey;
        static std::mutex clientMutex;
        static bool initialized;

        static void initialize();
        static bool ensureBucketExists(const std::string& bucketName);
        static bool createBucket(const std::string& bucketName);

        // Helpers per le richieste S3
        static std::string signRequest(const std::string& method,
                                       const std::string& bucket,
                                       const std::string& objectName,
                                       const std::string& contentType,
                                       const std::string& date);

        static std::string getISO8601Date();
        static void ensureDirectoryExists(const std::string& filePath);
    };

} // namespace stl2glb