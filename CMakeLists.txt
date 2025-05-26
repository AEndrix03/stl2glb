cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Trova le librerie con vcpkg
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(miniocpp CONFIG REQUIRED)

# TinyGLTF è header only, trova path include
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h" PATH_SUFFIXES tinygltf)
if(NOT TINYGLTF_INCLUDE_DIRS)
    message(FATAL_ERROR "tinygltf headers not found")
endif()

# Raccogli sorgenti cpp dalla cartella src
file(GLOB SOURCES "src/*.cpp")

add_library(stl2glb ${SOURCES})

# Imposta include directory pubbliche (include root e tinygltf)
target_include_directories(stl2glb PUBLIC
        include
        ${TINYGLTF_INCLUDE_DIRS}
)

# Linka le librerie trovate
target_link_libraries(stl2glb PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        httplib::httplib
        miniocpp::miniocpp
)

# Eseguibile principale con main.cpp
add_executable(stl2glb_exec src/main.cpp)
target_link_libraries(stl2glb_exec PRIVATE stl2glb)
