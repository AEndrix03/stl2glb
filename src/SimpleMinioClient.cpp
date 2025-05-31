#include "stl2glb/SimpleMinioClient.hpp"
#include "stl2glb/EnvironmentHandler.hpp"
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <string.h>
#include <vector>

namespace stl2glb {

    std::string SimpleMinioClient::endpoint;
    std::string SimpleMinioClient::accessKey;
    std::string SimpleMinioClient::secretKey;
    std::mutex SimpleMinioClient::clientMutex;
    bool SimpleMinioClient::initialized = false;

    void SimpleMinioClient::initialize() {
        std::lock_guard<std::mutex> lock(clientMutex);
        if (initialized) return;

        auto& env = EnvironmentHandler::instance();

        // Ottieni endpoint e credenziali
        endpoint = env.getMinioEndpoint();
        accessKey = env.getMinioAccessKey();
        secretKey = env.getMinioSecretKey();

        // Log per debug
        Logger::info("Initializing SimpleMinioClient with endpoint: " + endpoint);
        Logger::info("Access Key length: " + std::to_string(accessKey.length()));
        Logger::info("Secret Key length: " + std::to_string(secretKey.length()));

        // Controlla se l'endpoint include già il protocollo
        if (endpoint.find("http://") != 0 && endpoint.find("https://") != 0) {
            // Se non c'è protocollo, aggiungi http:// come default
            endpoint = "http://" + endpoint;
            Logger::info("Adjusted endpoint with HTTP protocol: " + endpoint);
        }

        // Rimuovi una potenziale doppia specificazione del protocollo
        if (endpoint.find("http://http://") == 0) {
            endpoint = endpoint.substr(7);
            Logger::info("Fixed double http:// in endpoint: " + endpoint);
        } else if (endpoint.find("https://http://") == 0) {
            endpoint = endpoint.substr(8);
            Logger::info("Fixed https://http:// in endpoint: " + endpoint);
        }

        initialized = true;
    }

    void SimpleMinioClient::download(const std::string& bucket,
                                     const std::string& objectName,
                                     const std::string& localPath) {
        initialize();

        try {
            ensureDirectoryExists(localPath);

            Logger::info("Downloading object: " + objectName + " from bucket: " + bucket);

            // Assicurati che il bucket esista
            if (!ensureBucketExists(bucket)) {
                Logger::warn("Bucket does not exist: " + bucket);
                Logger::info("Attempting to create bucket: " + bucket);
                createBucket(bucket);
            }

            // Estrai host e porta dall'endpoint
            std::string host = endpoint;
            if (host.find("http://") == 0) {
                host = host.substr(7);
            } else if (host.find("https://") == 0) {
                host = host.substr(8);
            }

            // Estrai la porta se presente
            std::string port = "80";
            size_t colonPos = host.find(":");
            if (colonPos != std::string::npos) {
                port = host.substr(colonPos + 1);
                host = host.substr(0, colonPos);
            }

            // Crea il client HTTP
            httplib::Client cli(host, std::stoi(port));

            // Imposta headers
            httplib::Headers headers;
            std::string date = getISO8601Date();
            std::string signature = signRequest("GET", bucket, objectName, "", date);

            headers.emplace("Authorization", "AWS " + accessKey + ":" + signature);
            headers.emplace("Date", date);

            // Esegui la richiesta
            std::string path = "/" + bucket + "/" + objectName;
            auto res = cli.Get(path.c_str(), headers);

            if (!res) {
                Logger::error("HTTP error: " + std::to_string(res->status));
                throw std::runtime_error("HTTP error: " + std::to_string(res->status));
            }

            if (res->status != 200) {
                Logger::error("Download failed with status: " + std::to_string(res->status));
                Logger::error("Response: " + res->body);
                throw std::runtime_error("Download failed with status: " + std::to_string(res->status));
            }

            // Scrivi il file su disco
            std::ofstream outFile(localPath, std::ios::binary);
            if (!outFile) {
                throw std::runtime_error("Could not open file for writing: " + localPath);
            }

            outFile.write(res->body.c_str(), res->body.size());
            outFile.close();

            Logger::info("Download successful: " + objectName);
        } catch (const std::exception& e) {
            Logger::error("Exception in download: " + std::string(e.what()));
            throw;
        }
    }

    void SimpleMinioClient::upload(const std::string& bucket,
                                   const std::string& objectName,
                                   const std::string& localPath) {
        initialize();

        try {
            if (!std::filesystem::exists(localPath)) {
                std::string error = "File not found for upload: " + localPath;
                Logger::error(error);
                throw std::runtime_error(error);
            }

            // Assicurati che il bucket esista
            if (!ensureBucketExists(bucket)) {
                Logger::warn("Bucket does not exist for upload: " + bucket);
                Logger::info("Attempting to create bucket: " + bucket);
                createBucket(bucket);
            }

            // Log della dimensione del file
            auto fileSize = std::filesystem::file_size(localPath);
            Logger::info("Uploading file of size " + std::to_string(fileSize) +
                         " bytes to " + bucket + "/" + objectName);

            // Leggi il file
            std::ifstream inFile(localPath, std::ios::binary);
            if (!inFile) {
                throw std::runtime_error("Could not open file for reading: " + localPath);
            }

            std::ostringstream ss;
            ss << inFile.rdbuf();
            std::string fileContent = ss.str();

            // Estrai host e porta dall'endpoint
            std::string host = endpoint;
            if (host.find("http://") == 0) {
                host = host.substr(7);
            } else if (host.find("https://") == 0) {
                host = host.substr(8);
            }

            // Estrai la porta se presente
            std::string port = "80";
            size_t colonPos = host.find(":");
            if (colonPos != std::string::npos) {
                port = host.substr(colonPos + 1);
                host = host.substr(0, colonPos);
            }

            // Crea il client HTTP
            httplib::Client cli(host, std::stoi(port));

            // Imposta headers
            httplib::Headers headers;
            std::string contentType = "model/gltf-binary";
            std::string date = getISO8601Date();
            std::string signature = signRequest("PUT", bucket, objectName, contentType, date);

            headers.emplace("Authorization", "AWS " + accessKey + ":" + signature);
            headers.emplace("Date", date);
            headers.emplace("Content-Type", contentType);
            headers.emplace("Content-Length", std::to_string(fileContent.size()));

            // Esegui la richiesta
            std::string path = "/" + bucket + "/" + objectName;
            auto res = cli.Put(path.c_str(), headers, fileContent, contentType);

            if (!res) {
                Logger::error("HTTP error: " + std::to_string(res->status));
                throw std::runtime_error("HTTP error: " + std::to_string(res->status));
            }

            if (res->status != 200 && res->status != 204) {
                Logger::error("Upload failed with status: " + std::to_string(res->status));
                Logger::error("Response: " + res->body);
                throw std::runtime_error("Upload failed with status: " + std::to_string(res->status));
            }

            Logger::info("Upload successful: " + objectName);
        } catch (const std::exception& e) {
            Logger::error("Exception in upload: " + std::string(e.what()));
            throw;
        }
    }

    bool SimpleMinioClient::ensureBucketExists(const std::string& bucketName) {
        try {
            Logger::info("Checking if bucket exists: " + bucketName);

            // Estrai host e porta dall'endpoint
            std::string host = endpoint;
            if (host.find("http://") == 0) {
                host = host.substr(7);
            } else if (host.find("https://") == 0) {
                host = host.substr(8);
            }

            // Estrai la porta se presente
            std::string port = "80";
            size_t colonPos = host.find(":");
            if (colonPos != std::string::npos) {
                port = host.substr(colonPos + 1);
                host = host.substr(0, colonPos);
            }

            // Crea il client HTTP
            httplib::Client cli(host, std::stoi(port));

            // Imposta headers
            httplib::Headers headers;
            std::string date = getISO8601Date();
            std::string signature = signRequest("GET", bucketName, "", "", date);

            headers.emplace("Authorization", "AWS " + accessKey + ":" + signature);
            headers.emplace("Date", date);

            // Esegui la richiesta
            std::string path = "/" + bucketName;
            auto res = cli.Head(path.c_str(), headers);

            if (!res) {
                Logger::warn("HTTP error: " + std::to_string(res->status));
                return false;
            }

            if (res->status == 200) {
                Logger::info("Bucket exists: " + bucketName);
                return true;
            } else if (res->status == 404) {
                Logger::info("Bucket does not exist: " + bucketName);
                return false;
            } else {
                Logger::warn("Unexpected status code: " + std::to_string(res->status));
                return false;
            }
        } catch (const std::exception& e) {
            Logger::warn("Exception in ensureBucketExists: " + std::string(e.what()));
            return false;
        }
    }

    bool SimpleMinioClient::createBucket(const std::string& bucketName) {
        try {
            Logger::info("Creating bucket: " + bucketName);

            // Estrai host e porta dall'endpoint
            std::string host = endpoint;
            if (host.find("http://") == 0) {
                host = host.substr(7);
            } else if (host.find("https://") == 0) {
                host = host.substr(8);
            }

            // Estrai la porta se presente
            std::string port = "80";
            size_t colonPos = host.find(":");
            if (colonPos != std::string::npos) {
                port = host.substr(colonPos + 1);
                host = host.substr(0, colonPos);
            }

            // Crea il client HTTP
            httplib::Client cli(host, std::stoi(port));

            // Imposta headers
            httplib::Headers headers;
            std::string date = getISO8601Date();
            std::string signature = signRequest("PUT", bucketName, "", "", date);

            headers.emplace("Authorization", "AWS " + accessKey + ":" + signature);
            headers.emplace("Date", date);

            // Esegui la richiesta
            std::string path = "/" + bucketName;
            auto res = cli.Put(path.c_str(), headers, "", "");

            if (!res) {
                Logger::error("HTTP error: " + std::to_string(res->status));
                return false;
            }

            if (res->status == 200 || res->status == 204) {
                Logger::info("Successfully created bucket: " + bucketName);
                return true;
            } else {
                Logger::warn("Failed to create bucket. Status: " + std::to_string(res->status));
                Logger::warn("Response: " + res->body);
                return false;
            }
        } catch (const std::exception& e) {
            Logger::warn("Exception in createBucket: " + std::string(e.what()));
            return false;
        }
    }

    std::string SimpleMinioClient::signRequest(const std::string& method,
                                               const std::string& bucket,
                                               const std::string& objectName,
                                               const std::string& contentType,
                                               const std::string& date) {
        // Implementazione semplificata della firma S3
        std::string stringToSign = method + "\n"
                                   + "\n"  // MD5
                                   + contentType + "\n"
                                   + date + "\n"
                                   + "/" + bucket;

        if (!objectName.empty()) {
            stringToSign += "/" + objectName;
        }

        // Firma HMAC-SHA1
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digestLength = 0;

        HMAC(EVP_sha1(), secretKey.c_str(), secretKey.length(),
             (unsigned char*)stringToSign.c_str(), stringToSign.length(),
             digest, &digestLength);

        // Converti in Base64
        std::string base64Signature;
        base64Signature.resize(((digestLength + 2) / 3) * 4);

        static const char base64Chars[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        size_t i = 0, j = 0;
        for (; i < digestLength; i += 3) {
            int val = (digest[i] << 16) |
                      ((i + 1 < digestLength ? digest[i + 1] : 0) << 8) |
                      (i + 2 < digestLength ? digest[i + 2] : 0);

            base64Signature[j++] = base64Chars[(val >> 18) & 0x3F];
            base64Signature[j++] = base64Chars[(val >> 12) & 0x3F];

            if (i + 1 < digestLength) {
                base64Signature[j++] = base64Chars[(val >> 6) & 0x3F];
            } else {
                base64Signature[j++] = '=';
            }

            if (i + 2 < digestLength) {
                base64Signature[j++] = base64Chars[val & 0x3F];
            } else {
                base64Signature[j++] = '=';
            }
        }

        return base64Signature;
    }

    std::string SimpleMinioClient::getISO8601Date() {
        // Format: Wed, 28 Oct 2009 22:32:00 GMT
        std::time_t now = std::time(nullptr);
        char buf[100];
        std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
        return std::string(buf);
    }

    void SimpleMinioClient::ensureDirectoryExists(const std::string& filePath) {
        std::filesystem::path destPath(filePath);
        if (destPath.has_parent_path()) {
            std::filesystem::path parentDir = destPath.parent_path();
            if (!std::filesystem::exists(parentDir)) {
                try {
                    std::filesystem::create_directories(parentDir);
                    Logger::info("Created destination directory: " + parentDir.string());
                } catch (const std::filesystem::filesystem_error& e) {
                    Logger::error("Cannot create destination directory " + parentDir.string() + ": " + e.what());
                    throw std::runtime_error("Error creating destination directory: " + std::string(e.what()));
                }
            }
        }
    }

} // namespace stl2glb