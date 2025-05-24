#include "stl2glb/Server.hpp"
#include "stl2glb/EnvironmentHandler.hpp"

int main() {
    auto& env = stl2glb::EnvironmentHandler::instance();
    env.init();

    stl2glb::Server server;
    server.start(env.getPort());
    return 0;
}
