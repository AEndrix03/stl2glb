#include "stl2glb/Converter.hpp"
#include "stl2glb/MinioClient.hpp"
#include "stl2glb/STLParser.hpp"
#include "stl2glb/GLBWriter.hpp"
#include "stl2glb/Hasher.hpp"
#include "stl2glb/Logger.hpp"
#include "stl2glb/EnvironmentHandler.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace stl2glb {

    std::string Converter::run(const std::string& stl_hash) {
        auto& env = EnvironmentHandler::instance();

        Logger::info("Start conversion for STL hash: " + stl_hash);

        std::string stl_path = "/tmp/" + stl_hash + ".stl";
        std::string glb_path = "/tmp/" + stl_hash + ".glb";

        Logger::info("Start downloading STL file with hash: " + stl_hash);
        MinioClient::download(env.getStlBucketName(), stl_hash, stl_path);
        Logger::info("STL file downloaded");

        Logger::info("STL Parsing...");
        auto triangles = STLParser::parse(stl_path);
        Logger::info("STL Parsed! GLB writing...");
        GLBWriter::write(triangles, glb_path);
        Logger::info("GLB write!");

        Logger::info("Calculating hash of converted file");
        std::string glb_hash = Hasher::sha256_file(glb_path);
        Logger::info("Hash: " + glb_hash);

        std::string final_glb = "/tmp/" + glb_hash + ".glb";
        fs::rename(glb_path, final_glb);
        Logger::info("Uploading converted file to bucket...");
        MinioClient::upload(env.getGlbBucketName(), glb_hash, final_glb);

        Logger::info("Completed conversion: GLB hash = " + glb_hash);
        return glb_hash;
    }

} // namespace stl2glb
