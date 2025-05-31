#include "stl2glb/Converter.hpp"
#include "stl2glb/MinioClient.hpp"
#include "stl2glb/STLParser.hpp"
#include "stl2glb/GLBWriter.hpp"
#include "stl2glb/Hasher.hpp"
#include "stl2glb/Logger.hpp"
#include "stl2glb/EnvironmentHandler.hpp"

#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace stl2glb {

    std::string Converter::run(const std::string& stl_hash) {
        auto& env = EnvironmentHandler::instance();
        auto start_time = std::chrono::high_resolution_clock::now();

        Logger::info("Start conversion for STL hash: " + stl_hash);

        std::string stl_path = "/tmp/" + stl_hash + ".stl";
        std::string glb_path = "/tmp/" + stl_hash + ".glb";

        try {
            // Download STL
            auto download_start = std::chrono::high_resolution_clock::now();
            Logger::info("Start downloading STL file with hash: " + stl_hash);
            MinioClient::download(env.getStlBucketName(), stl_hash, stl_path);
            auto download_end = std::chrono::high_resolution_clock::now();
            auto download_ms = std::chrono::duration_cast<std::chrono::milliseconds>(download_end - download_start).count();
            Logger::info("STL file downloaded in " + std::to_string(download_ms) + "ms");

            // Get file size for logging
            auto file_size = fs::file_size(stl_path);
            Logger::info("STL file size: " + std::to_string(file_size / 1024) + " KB");

            // Parse STL
            auto parse_start = std::chrono::high_resolution_clock::now();
            Logger::info("STL Parsing...");
            auto triangles = STLParser::parse(stl_path);
            auto parse_end = std::chrono::high_resolution_clock::now();
            auto parse_ms = std::chrono::duration_cast<std::chrono::milliseconds>(parse_end - parse_start).count();
            Logger::info("STL Parsed " + std::to_string(triangles.size()) + " triangles in " + std::to_string(parse_ms) + "ms");

            // Write GLB
            auto write_start = std::chrono::high_resolution_clock::now();
            Logger::info("GLB writing...");
            GLBWriter::write(triangles, glb_path);
            auto write_end = std::chrono::high_resolution_clock::now();
            auto write_ms = std::chrono::duration_cast<std::chrono::milliseconds>(write_end - write_start).count();
            Logger::info("GLB written in " + std::to_string(write_ms) + "ms");

            // Get GLB file size
            auto glb_size = fs::file_size(glb_path);
            Logger::info("GLB file size: " + std::to_string(glb_size / 1024) + " KB");
            Logger::info("Compression ratio: " + std::to_string((float)glb_size / file_size * 100) + "%");

            // Calculate hash
            Logger::info("Calculating hash of converted file");
            std::string glb_hash = Hasher::sha256_file(glb_path);
            Logger::info("Hash: " + glb_hash);

            // Rename and upload
            std::string final_glb = "/tmp/" + glb_hash + ".glb";
            fs::rename(glb_path, final_glb);

            auto upload_start = std::chrono::high_resolution_clock::now();
            Logger::info("Uploading converted file to bucket...");
            MinioClient::upload(env.getGlbBucketName(), glb_hash, final_glb);
            auto upload_end = std::chrono::high_resolution_clock::now();
            auto upload_ms = std::chrono::duration_cast<std::chrono::milliseconds>(upload_end - upload_start).count();
            Logger::info("File uploaded in " + std::to_string(upload_ms) + "ms");

            // Cleanup temporary files
            try {
                fs::remove(stl_path);
                fs::remove(final_glb);
                Logger::info("Temporary files cleaned up");
            } catch (const std::exception& e) {
                Logger::warn("Failed to cleanup temporary files: " + std::string(e.what()));
            }

            // Log total time
            auto end_time = std::chrono::high_resolution_clock::now();
            auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            Logger::info("Completed conversion: GLB hash = " + glb_hash + " in " + std::to_string(total_ms) + "ms");

            return glb_hash;

        } catch (const std::exception& e) {
            // Cleanup on error
            try {
                if (fs::exists(stl_path)) fs::remove(stl_path);
                if (fs::exists(glb_path)) fs::remove(glb_path);
            } catch (...) {}

            throw;
        }
    }

} // namespace stl2glb