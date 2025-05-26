cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Solo OpenSSL con find_package
find_package(OpenSSL REQUIRED)

# Aggiungi gli include manuali per le librerie clonate (modifica i path se serve)
target_include_directories(stl2glb PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/json/include>          # nlohmann_json
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-httplib>           # cpp-httplib
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf>              # tinygltf
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/minio-cpp/include>     # minio-cpp
        $<INSTALL_INTERFACE:include>
)

add_library(stl2glb
        Converter.cpp
        EnvironmentHandler.cpp
        GLBWriter.cpp
        Hasher.cpp
        Logger.cpp
        MinioClient.cpp
        Server.cpp
        STLParser.cpp
)

target_link_libraries(stl2glb PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
)

add_executable(stl2glb main.cpp)
target_link_libraries(stl2glb PRIVATE stl2glb)
