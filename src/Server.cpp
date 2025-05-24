#include "stl2glb/Server.hpp"
#include "stl2glb/Converter.hpp"
#include "external/cpp-httplib/httplib.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace stl2glb {

    void Server::start(int port) {
        httplib::Server svr;

        svr.Post("/convert", [](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                std::string stl_hash = body.at("stl_hash");

                std::string glb_hash = Converter::run(stl_hash);

                res.set_content(json{{"glb_hash", glb_hash}}.dump(), "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            }
        });

        std::cout << "[stl2glb] Server listening on port " << port << std::endl;
        svr.listen("0.0.0.0", port);
    }

} // namespace stl2glb
