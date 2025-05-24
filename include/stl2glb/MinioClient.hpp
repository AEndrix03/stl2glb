#pragma once
#include <string>

namespace stl2glb {

    class MinioClient {
    public:
        static void download(const std::string& bucket,
                             const std::string& objectName,
                             const std::string& localPath);

        static void upload(const std::string& bucket,
                           const std::string& objectName,
                           const std::string& localPath);
    };

} // namespace stl2glb
