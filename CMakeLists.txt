cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Assicurati che vcpkg sia utilizzato correttamente
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

# Controllo per ambiente Windows
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# Trova le librerie con vcpkg - rimuoviamo miniocpp
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

# Raccogli sorgenti cpp dalla cartella src
file(GLOB SOURCES "src/*.cpp")

# Rimuovi MinioClient.cpp (useremo SimpleMinioClient invece)
list(FILTER SOURCES EXCLUDE REGEX ".*MinioClient\\.cpp$")

# Aggiungi il nuovo SimpleMinioClient.cpp
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/SimpleMinioClient.cpp")

# Aggiungi la fonte di tinygltf
list(APPEND SOURCES "external/tinygltf/tiny_gltf.cc")

add_library(stl2glb ${SOURCES})

# Imposta include directory pubbliche (include root e tinygltf)
target_include_directories(stl2glb PUBLIC
        include
        external/tinygltf
)

# Linka le librerie trovate
target_link_libraries(stl2glb PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        httplib::httplib
)

# In Windows, potrebbe essere necessario specificare esplicitamente le librerie di sistema
if(WIN32)
    target_link_libraries(stl2glb PUBLIC
            ws2_32
            crypt32
            bcrypt
            winhttp
    )
endif()

# Eseguibile principale con main.cpp
add_executable(stl2glb_exec src/main.cpp)
target_link_libraries(stl2glb_exec PRIVATE stl2glb)

# Se utilizzi un target Debug in Windows, assicurati di usare librerie debug
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(stl2glb PUBLIC _DEBUG)
endif()