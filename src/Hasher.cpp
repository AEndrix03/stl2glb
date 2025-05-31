#include "stl2glb/Hasher.hpp"
#include <openssl/evp.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <memory>

namespace stl2glb {

    std::string Hasher::sha256_file(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Unable to open file for hashing: " + path);
        }

        // Usa la moderna API EVP di OpenSSL 3.0+
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create hash context");
        }

        // Unique pointer per cleanup automatico
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctxPtr(ctx, EVP_MD_CTX_free);

        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
            throw std::runtime_error("Failed to initialize SHA256 hash");
        }

        char buffer[4096];
        while (file.read(buffer, sizeof(buffer))) {
            if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1) {
                throw std::runtime_error("Failed to update hash");
            }
        }

        // Last partial read
        if (file.gcount() > 0) {
            if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1) {
                throw std::runtime_error("Failed to update hash");
            }
        }

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLen;
        if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
            throw std::runtime_error("Failed to finalize hash");
        }

        std::ostringstream oss;
        for (unsigned int i = 0; i < hashLen; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }

        return oss.str();
    }

} // namespace stl2glb