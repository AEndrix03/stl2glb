#include "stl2glb/Server.hpp"
#include "stl2glb/Converter.hpp"
#include "stl2glb/Logger.hpp"
#include <httplib.h>

#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace stl2glb {

    void Server::start(int port) {
        httplib::Server svr;

        std::cout << "[stl2glb] Server started on port " << port << std::endl;

        // Health check endpoint
        svr.Get("/health", [](const httplib::Request& req, httplib::Response& res) {
            res.set_content("{\"status\":\"healthy\",\"service\":\"stl2glb\"}", "application/json");
        });

        // Convert endpoint
        svr.Post("/convert", [](const httplib::Request& req, httplib::Response& res) {
            stl2glb::Logger::info("Received /convert POST request");
            try {
                auto body = json::parse(req.body);
                std::string stl_hash = body.at("stl_hash");

                std::string glb_hash = Converter::run(stl_hash);

                res.set_content(json{{"glb_hash", glb_hash}}.dump(), "application/json");
            } catch (const std::exception& e) {
                stl2glb::Logger::error(std::string("Error in /convert: ") + e.what());
                res.status = 400;
                res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            }
        });

        std::cout << "[stl2glb] Server listening on port " << port << std::endl;
        svr.listen("0.0.0.0", port);
    }

} // namespace stl2glb