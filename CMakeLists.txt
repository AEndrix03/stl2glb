cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(tinygltf REQUIRED) # tinygltf è header-only, quindi usa find_path
find_package(miniocpp CONFIG REQUIRED)  # attenzione al nome esatto

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
        httplib::httplib
        miniocpp::miniocpp
)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h" PATH_SUFFIXES tinygltf)
if (TINYGLTF_INCLUDE_DIRS)
    target_include_directories(stl2glb PUBLIC ${TINYGLTF_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Could not find tinygltf headers")
endif()

add_executable(stl2glb_exec main.cpp)
target_link_libraries(stl2glb_exec PRIVATE stl2glb)
