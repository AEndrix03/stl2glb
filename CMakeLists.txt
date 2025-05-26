cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpp-httplib CONFIG REQUIRED)
find_package(tinygltf CONFIG REQUIRED)
find_package(minio-cpp CONFIG REQUIRED)

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
        nlohmann_json::nlohmann_json
        cpp-httplib::cpp-httplib
        tinygltf::tinygltf
        minio-cpp::minio-cpp
)

add_executable(stl2glb_exec main.cpp)
target_link_libraries(stl2glb_exec PRIVATE stl2glb)
