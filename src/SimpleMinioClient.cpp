#include "stl2glb/SimpleMinioClient.hpp"
#include "stl2glb/EnvironmentHandler.hpp"
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <string.h>
#include <vector>
#include <sstream>
#include <openssl/sha.h>

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

        // Log per info
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

    // Funzione helper per URL encoding
    std::string SimpleMinioClient::urlEncode(const std::string& value, bool encodeSlash) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
            std::string::value_type c = (*i);

            // Keep alphanumeric and other accepted characters intact
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || (!encodeSlash && c == '/')) {
                escaped << c;
                continue;
            }

            // Any other characters are percent-encoded
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char) c);
            escaped << std::nouppercase;
        }

        return escaped.str();
    }
    std::string SimpleMinioClient::sha256(const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data.c_str(), data.size());
        SHA256_Final(hash, &sha256);

        std::stringstream ss;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    std::string SimpleMinioClient::hmacSha256(const std::string& key, const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        unsigned int hashLen;

        HMAC(EVP_sha256(), key.c_str(), key.length(),
             (unsigned char*)data.c_str(), data.length(),
             hash, &hashLen);

        std::stringstream ss;
        for(unsigned int i = 0; i < hashLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    std::vector<unsigned char> SimpleMinioClient::hmacSha256Raw(const std::string& key, const std::string& data) {
        std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
        unsigned int hashLen;

        HMAC(EVP_sha256(), key.c_str(), key.length(),
             (unsigned char*)data.c_str(), data.length(),
             hash.data(), &hashLen);

        hash.resize(hashLen);
        return hash;
    }

    std::string SimpleMinioClient::getAmzDate() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::gmtime(&time_t);

        char buffer[17];
        std::strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%SZ", tm);
        return std::string(buffer);
    }

    std::string SimpleMinioClient::getDateStamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::gmtime(&time_t);

        char buffer[9];
        std::strftime(buffer, sizeof(buffer), "%Y%m%d", tm);
        return std::string(buffer);
    }

    httplib::Headers SimpleMinioClient::createAwsV4Headers(
            const std::string& method,
            const std::string& path,
            const std::string& payload,
            const std::string& contentType) {

        // Estrai host completo dall'endpoint (inclusa la porta per MinIO)
        std::string fullHost = endpoint;
        if (fullHost.find("http://") == 0) {
            fullHost = fullHost.substr(7);
        } else if (fullHost.find("https://") == 0) {
            fullHost = fullHost.substr(8);
        }

        httplib::Headers headers;

        // AWS V4 richiede questi headers
        std::string amzDate = getAmzDate();
        std::string dateStamp = getDateStamp();
        std::string payloadHash = sha256(payload);

        // IMPORTANTE: Per MinIO, usa l'host completo con porta
        headers.emplace("Host", fullHost);
        headers.emplace("x-amz-date", amzDate);
        headers.emplace("x-amz-content-sha256", payloadHash);
        headers.emplace("User-Agent", "SimpleMinioClient/1.0");

        if (!contentType.empty()) {
            headers.emplace("Content-Type", contentType);
        }
        if (!payload.empty()) {
            headers.emplace("Content-Length", std::to_string(payload.size()));
        }

        // URL encode del path per la canonical request
        // Mantieni il path così com'è per MinIO (non fare l'encode degli slash)
        std::string canonicalUri = path;

        // Crea la canonical request usando l'host completo con porta
        std::string canonicalHeaders = "host:" + fullHost + "\n" +
                                       "x-amz-content-sha256:" + payloadHash + "\n" +
                                       "x-amz-date:" + amzDate + "\n";

        std::string signedHeaders = "host;x-amz-content-sha256;x-amz-date";

        std::string canonicalRequest = method + "\n" +
                                       canonicalUri + "\n" +
                                       "\n" +  // query string vuoto
                                       canonicalHeaders + "\n" +
                                       signedHeaders + "\n" +
                                       payloadHash;

        // Log per info (commentare in produzione)
        Logger::info("Canonical Request:\n" + canonicalRequest);

        // Crea la string to sign
        std::string algorithm = "AWS4-HMAC-SHA256";
        std::string credentialScope = dateStamp + "/us-east-1/s3/aws4_request";
        std::string hashedCanonicalRequest = sha256(canonicalRequest);

        std::string stringToSign = algorithm + "\n" +
                                   amzDate + "\n" +
                                   credentialScope + "\n" +
                                   hashedCanonicalRequest;

        Logger::info("String to Sign:\n" + stringToSign);

        // Calcola la signing key
        std::string kSecret = "AWS4" + secretKey;
        auto kDate = hmacSha256Raw(kSecret, dateStamp);
        auto kRegion = hmacSha256Raw(std::string(kDate.begin(), kDate.end()), "us-east-1");
        auto kService = hmacSha256Raw(std::string(kRegion.begin(), kRegion.end()), "s3");
        auto kSigning = hmacSha256Raw(std::string(kService.begin(), kService.end()), "aws4_request");

        // Calcola la signature
        std::string signature = hmacSha256(std::string(kSigning.begin(), kSigning.end()), stringToSign);

        Logger::info("Signature: " + signature);

        // Crea l'authorization header
        std::string authorizationHeader = algorithm + " " +
                                          "Credential=" + accessKey + "/" + credentialScope + ", " +
                                          "SignedHeaders=" + signedHeaders + ", " +
                                          "Signature=" + signature;

        headers.emplace("Authorization", authorizationHeader);

        return headers;
    }

    void SimpleMinioClient::download(const std::string& bucket,
                                     const std::string& objectName,
                                     const std::string& localPath) {
        initialize();

        try {
            ensureDirectoryExists(localPath);

            Logger::info("Downloading object: " + objectName + " from bucket: " + bucket);

            // Verifica che il nome del bucket non contenga underscore all'inizio o alla fine
            if (bucket.empty() || bucket[0] == '_' || bucket[bucket.length()-1] == '_') {
                Logger::error("Invalid bucket name format: " + bucket);
                throw std::runtime_error("Invalid bucket name format");
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

            Logger::info("Connecting to " + host + ":" + port);

            // Crea il client HTTP
            httplib::Client cli(host, std::stoi(port));
            cli.set_connection_timeout(30);
            cli.set_read_timeout(30);

            // Prepara la richiesta con AWS V4 signature
            std::string path = "/" + bucket + "/" + objectName;
            Logger::info("Request path: " + path);

            auto headers = createAwsV4Headers("GET", path, "", "");

            // Log headers per info
            for (const auto& header : headers) {
                Logger::info("Header: " + header.first + " = " + header.second);
            }

            // Esegui la richiesta
            auto res = cli.Get(path.c_str(), headers);

            if (!res) {
                Logger::error("HTTP connection error");
                throw std::runtime_error("HTTP connection error");
            }

            if (res->status != 200) {
                Logger::error("Download failed with status: " + std::to_string(res->status));
                Logger::error("Response: " + res->body);

                // Log response headers per info
                for (const auto& header : res->headers) {
                    Logger::info("Response Header: " + header.first + " = " + header.second);
                }

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
            cli.set_connection_timeout(30);
            cli.set_read_timeout(30);

            // Prepara la richiesta con AWS V4 signature
            std::string path = "/" + bucket + "/" + objectName;
            std::string contentType = "model/gltf-binary";
            auto headers = createAwsV4Headers("PUT", path, fileContent, contentType);

            // Esegui la richiesta
            auto res = cli.Put(path.c_str(), headers, fileContent, contentType);

            if (!res) {
                Logger::error("HTTP connection error");
                throw std::runtime_error("HTTP connection error");
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
            cli.set_connection_timeout(10);

            // Prepara la richiesta con AWS V4 signature
            std::string path = "/" + bucketName;
            auto headers = createAwsV4Headers("HEAD", path, "", "");

            // Esegui la richiesta
            auto res = cli.Head(path.c_str(), headers);

            if (!res) {
                Logger::warn("HTTP connection error");
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
            cli.set_connection_timeout(10);

            // Prepara la richiesta con AWS V4 signature
            std::string path = "/" + bucketName;
            auto headers = createAwsV4Headers("PUT", path, "", "");

            // Esegui la richiesta
            auto res = cli.Put(path.c_str(), headers, "", "");

            if (!res) {
                Logger::error("HTTP connection error");
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