#include "stl2glb/MinioClient.hpp"
#include "stl2glb/EnvironmentHandler.hpp"
#include "stl2glb/Logger.hpp"
#include <miniocpp/client.h>
#include <filesystem>
#include <stdexcept>
#include <memory>
#include <mutex>

namespace stl2glb {

    class MinioClientImpl {
    private:
        std::shared_ptr<minio::creds::StaticProvider> credentials;
        std::shared_ptr<minio::s3::Client> client;
        static std::mutex initMutex;
        static std::unique_ptr<MinioClientImpl> instance;

        MinioClientImpl() {
            auto& env = EnvironmentHandler::instance();

            std::string accessKey = env.getMinioAccessKey();
            std::string secretKey = env.getMinioSecretKey();
            std::string endpoint = env.getMinioEndpoint();

            // Log per debug
            Logger::info("Initializing MinIO client with endpoint: " + endpoint);
            Logger::info("Access Key length: " + std::to_string(accessKey.length()));
            Logger::info("Secret Key length: " + std::to_string(secretKey.length()));

            // Debug: mostra i primi e gli ultimi caratteri delle chiavi
            if (!accessKey.empty()) {
                std::string firstChars = accessKey.substr(0, std::min(size_t(3), accessKey.length()));
                std::string lastChars = accessKey.length() > 3 ?
                                        accessKey.substr(accessKey.length() - 3) : "";
                Logger::info("Access Key preview: " + firstChars + "..." + lastChars);
            }

            // Controlla se l'endpoint include già il protocollo
            if (endpoint.find("http://") != 0 && endpoint.find("https://") != 0) {
                // Se non c'è protocollo, aggiungi http:// come default
                endpoint = "http://" + endpoint;
                Logger::info("Adjusted endpoint with HTTP protocol: " + endpoint);
            }

            // Rimuovi una potenziale doppia specificazione del protocollo
            // (in caso l'utente abbia configurato http://http://...)
            if (endpoint.find("http://http://") == 0) {
                endpoint = endpoint.substr(7);
                Logger::info("Fixed double http:// in endpoint: " + endpoint);
            } else if (endpoint.find("https://http://") == 0) {
                endpoint = endpoint.substr(8);
                Logger::info("Fixed https://http:// in endpoint: " + endpoint);
            }

            // Log per debug di connessione
            Logger::info("Testing connection to endpoint before initializing client...");

            try {
                // Crea le credenziali
                credentials = std::make_shared<minio::creds::StaticProvider>(
                        accessKey,
                        secretKey
                );

                // Crea il client
                minio::s3::BaseUrl baseUrl{endpoint};
                client = std::make_shared<minio::s3::Client>(baseUrl, credentials.get());

                // Test di connessione base
                minio::s3::ListBucketsArgs args;
                auto result = client->ListBuckets(args);

                if (result) {
                    Logger::info("Successfully connected to MinIO! Found " +
                                 std::to_string(result.buckets.size()) + " buckets");

                    // Stampa i nomi dei bucket trovati
                    std::string bucketList = "";
                    for (const auto& bucket : result.buckets) {
                        if (!bucketList.empty()) bucketList += ", ";
                        bucketList += bucket.name;
                    }
                    if (!bucketList.empty()) {
                        Logger::info("Available buckets: " + bucketList);
                    }
                } else {
                    Logger::error("Failed to list buckets. Error: " + result.code + " - " + result.message);

                    // Se abbiamo ricevuto un errore di accesso negato, le credenziali sono probabilmente sbagliate
                    if (result.code == "AccessDenied" || result.code == "InvalidAccessKeyId") {
                        Logger::error("Authentication failed. Please check your access key and secret key.");
                    }
                }
            } catch (const std::exception& e) {
                Logger::error("Exception during MinIO client initialization: " + std::string(e.what()));
            }

            // Verifica che i bucket esistano e creali se necessario
            // Non lanciamo eccezioni per permettere al programma di continuare
            try {
                if (!bucketExists(env.getStlBucketName())) {
                    createBucket(env.getStlBucketName());
                }

                if (!bucketExists(env.getGlbBucketName())) {
                    createBucket(env.getGlbBucketName());
                }
            } catch (const std::exception& e) {
                Logger::error("Failed to ensure buckets exist: " + std::string(e.what()));
                Logger::info("Continuing anyway...");
            }
        }

        bool bucketExists(const std::string& bucketName) {
            try {
                Logger::info("Checking if bucket exists: " + bucketName);

                minio::s3::BucketExistsArgs args;
                args.bucket = bucketName;

                auto result = client->BucketExists(args);
                if (!result) {
                    // Se il risultato non è valido, stampa il codice e il messaggio di errore
                    Logger::error("Failed to check if bucket exists. Code: " + result.code +
                                  ", Message: " + result.message);
                    return false;
                }

                // Verifica il campo exist della risposta
                if (result.exist) {
                    Logger::info("Bucket exists: " + bucketName);
                    return true;
                } else {
                    Logger::info("Bucket does not exist: " + bucketName);
                    return false;
                }
            } catch (const std::exception& e) {
                Logger::error("Exception in bucketExists: " + std::string(e.what()));
                return false;
            }
        }

        bool createBucket(const std::string& bucketName) {
            try {
                Logger::info("Creating bucket: " + bucketName);

                minio::s3::MakeBucketArgs args;
                args.bucket = bucketName;

                auto result = client->MakeBucket(args);
                if (!result) {
                    Logger::error("Failed to create bucket. Code: " + result.code +
                                  ", Message: " + result.message);
                    return false;
                }

                Logger::info("Successfully created bucket: " + bucketName);
                return true;
            } catch (const std::exception& e) {
                Logger::error("Exception in createBucket: " + std::string(e.what()));
                return false;
            }
        }

    public:
        // Singleton pattern
        static MinioClientImpl& getInstance() {
            std::lock_guard<std::mutex> lock(initMutex);
            if (!instance) {
                instance = std::unique_ptr<MinioClientImpl>(new MinioClientImpl());
            }
            return *instance;
        }

        void download(const std::string& bucket,
                      const std::string& objectName,
                      const std::string& localPath) {
            try {
                ensureDirectoryExists(localPath);

                Logger::info("Downloading object: " + objectName + " from bucket: " + bucket);

                // Verifica prima se l'oggetto esiste
                if (!objectExists(bucket, objectName)) {
                    throw std::runtime_error("Object does not exist in bucket: " + objectName);
                }

                minio::s3::DownloadObjectArgs args;
                args.bucket = bucket;
                args.object = objectName;
                args.filename = localPath;
                args.overwrite = true;

                auto result = client->DownloadObject(args);

                if (!result) {
                    std::string errorDetails = "Code: " + result.code + ", Message: " + result.message;
                    Logger::error("Download failed: " + errorDetails);

                    // Log aggiuntivo per debug
                    if (result.code == "AccessDenied") {
                        Logger::error("Access denied. Please check your credentials and bucket permissions.");
                    } else if (result.code == "NoSuchKey") {
                        Logger::error("The specified object does not exist: " + objectName);
                    } else if (result.code == "NoSuchBucket") {
                        Logger::error("The specified bucket does not exist: " + bucket);
                    }

                    throw std::runtime_error("MinIO download error: " + errorDetails);
                }

                Logger::info("Download successful: " + objectName);
            } catch (const std::exception& e) {
                Logger::error("Exception in download: " + std::string(e.what()));
                throw;
            }
        }

        bool objectExists(const std::string& bucket, const std::string& objectName) {
            try {
                Logger::info("Checking if object exists: " + objectName + " in bucket: " + bucket);

                minio::s3::StatObjectArgs args;
                args.bucket = bucket;
                args.object = objectName;

                auto result = client->StatObject(args);
                if (!result) {
                    Logger::error("Object does not exist. Code: " + result.code +
                                  ", Message: " + result.message);
                    return false;
                }

                Logger::info("Object exists: " + objectName);
                return true;
            } catch (const std::exception& e) {
                Logger::error("Exception in objectExists: " + std::string(e.what()));
                return false;
            }
        }

        void upload(const std::string& bucket,
                    const std::string& objectName,
                    const std::string& localPath) {
            try {
                if (!std::filesystem::exists(localPath)) {
                    std::string error = "File not found for upload: " + localPath;
                    Logger::error(error);
                    throw std::runtime_error(error);
                }

                // Verifica che il bucket esista, se non esiste crealo
                if (!bucketExists(bucket)) {
                    Logger::info("Bucket doesn't exist for upload, creating: " + bucket);
                    if (!createBucket(bucket)) {
                        throw std::runtime_error("Failed to create bucket for upload: " + bucket);
                    }
                }

                // Log della dimensione del file
                auto fileSize = std::filesystem::file_size(localPath);
                Logger::info("Uploading file of size " + std::to_string(fileSize) +
                             " bytes to " + bucket + "/" + objectName);

                minio::s3::UploadObjectArgs args;
                args.bucket = bucket;
                args.object = objectName;
                args.filename = localPath;
                args.content_type = "model/gltf-binary";

                Logger::info("Starting upload to bucket: " + bucket + ", object: " + objectName);
                auto result = client->UploadObject(args);

                if (!result) {
                    std::string errorDetails = "Code: " + result.code + ", Message: " + result.message;
                    Logger::error("Upload failed: " + errorDetails);

                    // Log aggiuntivo per debug
                    if (result.code == "AccessDenied") {
                        Logger::error("Access denied. Please check your credentials and bucket permissions.");
                    } else if (result.code == "NoSuchBucket") {
                        Logger::error("The specified bucket does not exist: " + bucket);
                    }

                    throw std::runtime_error("MinIO upload error: " + errorDetails);
                }

                Logger::info("Upload successful: " + objectName);

                // Verifica che l'oggetto sia stato effettivamente caricato
                if (objectExists(bucket, objectName)) {
                    Logger::info("Verified that object exists after upload: " + objectName);
                } else {
                    Logger::warn("Object not found after upload. This is unexpected: " + objectName);
                }
            } catch (const std::exception& e) {
                Logger::error("Exception in upload: " + std::string(e.what()));
                throw;
            }
        }

    private:
        void ensureDirectoryExists(const std::string& filePath) {
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
    };

// Inizializzazione delle variabili statiche
    std::mutex MinioClientImpl::initMutex;
    std::unique_ptr<MinioClientImpl> MinioClientImpl::instance;

// Implementazione delle funzioni statiche di MinioClient
    void MinioClient::download(const std::string& bucket,
                               const std::string& objectName,
                               const std::string& localPath) {
        MinioClientImpl::getInstance().download(bucket, objectName, localPath);
    }

    void MinioClient::upload(const std::string& bucket,
                             const std::string& objectName,
                             const std::string& localPath) {
        MinioClientImpl::getInstance().upload(bucket, objectName, localPath);
    }

} // namespace stl2glb