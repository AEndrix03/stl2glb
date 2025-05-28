#include "stl2glb/MinioClient.hpp"
#include "stl2glb/EnvironmentHandler.hpp"
#include "stl2glb/Logger.hpp"
#include <miniocpp/client.h>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <memory>
#include <iostream>

using namespace minio;

namespace stl2glb {

    struct ClientWrapper {
        std::shared_ptr<minio::creds::StaticProvider> credentials;
        std::shared_ptr<minio::s3::Client> client;
    };

    static ClientWrapper createClient() {
        auto& env = EnvironmentHandler::instance();

        auto credentials = std::make_shared<minio::creds::StaticProvider>(
                env.getMinioAccessKey(),
                env.getMinioSecretKey()
        );

        minio::s3::BaseUrl baseUrl{env.getMinioEndpoint()};

        auto client = std::make_shared<minio::s3::Client>(baseUrl, credentials.get());

        return ClientWrapper{credentials, client};
    }

    void MinioClient::download(const std::string& bucket,
                               const std::string& objectName,
                               const std::string& localPath) {
        auto wrapper = createClient();
        auto& client = wrapper.client;

        s3::DownloadObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;
        args.filename = localPath;
        args.overwrite = true;

        auto result = client->DownloadObject(args);
        if (!result) {
            stl2glb::Logger::error("Download da MinIO fallito: " + result.message);
            throw std::runtime_error("Errore nel download da MinIO: " + result.message);
        }

        stl2glb::Logger::info(std::string("Download riuscito: ") + objectName);
    }

    void MinioClient::upload(const std::string& bucket,
                             const std::string& objectName,
                             const std::string& localPath) {
        auto wrapper = createClient();
        auto& client = wrapper.client;

        s3::UploadObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;
        args.filename = localPath;
        args.content_type = "model/gltf-binary";

        auto result = client->UploadObject(args);
        if (!result) {
            stl2glb::Logger::error("Upload su MinIO fallito: " + result.message);
            throw std::runtime_error("Errore durante l'upload su MinIO: " + result.message);
        }

        stl2glb::Logger::info(std::string("Upload riuscito: ") + objectName);
    }

} // namespace stl2glb
