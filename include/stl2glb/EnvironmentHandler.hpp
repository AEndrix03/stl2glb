#pragma once
#include <string>

namespace stl2glb {

    class EnvironmentHandler {
    public:
        static EnvironmentHandler& instance();

        void init();

        int getPort() const;
        const std::string& getStlBucketName() const;
        const std::string& getGlbBucketName() const;

        const std::string& getMinioEndpoint() const;
        const std::string& getMinioAccessKey() const;
        const std::string& getMinioSecretKey() const;

    private:
        EnvironmentHandler() = default;

        int port = 8000;
        std::string stlBucketName;
        std::string glbBucketName;

        std::string minioEndpoint;
        std::string minioAccessKey;
        std::string minioSecretKey;
    };

} // namespace stl2glb
