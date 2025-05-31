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

            // Non logghiamo più le credenziali
            Logger::info("Initializing MinIO client with endpoint: " + endpoint);

            credentials = std::make_shared<minio::creds::StaticProvider>(
                    accessKey,
                    secretKey
            );

            minio::s3::BaseUrl baseUrl{endpoint};
            client = std::make_shared<minio::s3::Client>(baseUrl, credentials.get());

            // Verifica che i bucket esistano e creali se necessario
            ensureBucketExists(env.getStlBucketName());
            ensureBucketExists(env.getGlbBucketName());
        }

        void ensureBucketExists(const std::string& bucketName) {
            try {
                minio::s3::BucketExistsArgs args;
                args.bucket = bucketName;

                auto result = client->BucketExists(args);
                if (!result) {
                    Logger::error("Failed to check if bucket exists: " + result.message);
                    throw std::runtime_error("MinIO error: " + result.message);
                }

                if (!result.value) {
                    Logger::info("Creating bucket: " + bucketName);
                    minio::s3::MakeBucketArgs mkArgs;
                    mkArgs.bucket = bucketName;

                    auto mkResult = client->MakeBucket(mkArgs);
                    if (!mkResult) {
                        Logger::error("Failed to create bucket: " + mkResult.message);
                        throw std::runtime_error("MinIO error: " + mkResult.message);
                    }
                    Logger::info("Created bucket: " + bucketName);
                } else {
                    Logger::info("Bucket already exists: " + bucketName);
                }
            } catch (const std::exception& e) {
                Logger::error("Exception in ensureBucketExists: " + std::string(e.what()));
                throw;
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

                minio::s3::DownloadObjectArgs args;
                args.bucket = bucket;
                args.object = objectName;
                args.filename = localPath;
                args.overwrite = true;

                Logger::info("Downloading object: " + objectName + " from bucket: " + bucket);
                auto result = client->DownloadObject(args);
                if (!result) {
                    Logger::error("Download failed: " + result.message);
                    throw std::runtime_error("MinIO download error: " + result.message);
                }

                Logger::info("Download successful: " + objectName);
            } catch (const std::exception& e) {
                Logger::error("Exception in download: " + std::string(e.what()));
                throw;
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

                minio::s3::UploadObjectArgs args;
                args.bucket = bucket;
                args.object = objectName;
                args.filename = localPath;
                args.content_type = "model/gltf-binary";

                Logger::info("Uploading object: " + objectName + " to bucket: " + bucket);
                auto result = client->UploadObject(args);
                if (!result) {
                    Logger::error("Upload failed: " + result.message);
                    throw std::runtime_error("MinIO upload error: " + result.message);
                }

                Logger::info("Upload successful: " + objectName);
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