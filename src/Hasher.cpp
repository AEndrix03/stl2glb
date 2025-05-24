#include "stl2glb/Hasher.hpp"
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace stl2glb {

    std::string Hasher::sha256_file(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Unable to open file for hashing: " + path);
        }

        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        char buffer[4096];
        while (file.read(buffer, sizeof(buffer))) {
            SHA256_Update(&ctx, buffer, file.gcount());
        }
        // Last partial read
        if (file.gcount() > 0) {
            SHA256_Update(&ctx, buffer, file.gcount());
        }

        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &ctx);

        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }

        return oss.str();
    }

} // namespace stl2glb
