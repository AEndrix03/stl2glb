#include "stl2glb/MinioClient.hpp"
#include "stl2glb/EnvironmentHandler.hpp"
#include "stl2glb/Logger.hpp"
#include <miniocpp/client.h>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <memory>

using namespace minio;

namespace stl2glb {

    static std::shared_ptr<s3::Client> createClient() {
        auto& env = EnvironmentHandler::instance();

        s3::BaseUrl base_url{env.getMinioEndpoint()};
        auto* provider = new creds::StaticProvider(env.getMinioAccessKey(), env.getMinioSecretKey());

        return std::make_shared<s3::Client>(
                base_url,
                provider,
                "us-east-1",
                "v4",
                false
        );
    }

    void stl2glb::MinioClient::download(const std::string& bucket,
                                        const std::string& objectName,
                                        const std::string& localPath) {
        auto client = createClient();

        s3::DownloadObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;
        args.filename = localPath;
        args.overwrite = true;

        auto result = client->DownloadObject(args);

        if (!result) {
            throw std::runtime_error("Errore nel download da MinIO");
        }

        stl2glb::Logger::info(std::string("Download riuscito: ") + objectName);
    }

    void MinioClient::upload(const std::string& bucket,
                             const std::string& objectName,
                             const std::string& localPath) {
        auto client = createClient();

        s3::UploadObjectArgs args;
        args.bucket = bucket;
        args.object = objectName;
        args.filename = localPath;
        args.content_type = "model/gltf-binary";

        auto result = client->UploadObject(args);

        if (!result) {
            throw std::runtime_error("Errore durante l'upload su MinIO");
        }

        stl2glb::Logger::info(std::string("Upload riuscito: ") + objectName);
    }



} // namespace stl2glb
