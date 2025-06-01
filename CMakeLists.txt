cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 2.0.0 LANGUAGES CXX)

# Standard C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()

# Flags ottimizzate per Alpine/container
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-Wl,--gc-sections -s")

# Trova librerie - approccio semplificato
find_package(Threads REQUIRED)

# OpenSSL - prova diversi metodi
find_package(OpenSSL QUIET)
if(NOT OPENSSL_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(OPENSSL openssl)
    endif()
endif()

# nlohmann-json
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
    if(NLOHMANN_JSON_INCLUDE_DIR)
        add_library(nlohmann_json INTERFACE)
        target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
        set(nlohmann_json_FOUND TRUE)
    endif()
endif()

# httplib
find_package(httplib CONFIG QUIET)
if(NOT httplib_FOUND)
    find_path(HTTPLIB_INCLUDE_DIR httplib.h)
    if(HTTPLIB_INCLUDE_DIR)
        add_library(httplib INTERFACE)
        target_include_directories(httplib INTERFACE ${HTTPLIB_INCLUDE_DIR})
        add_library(httplib::httplib ALIAS httplib)
        set(httplib_FOUND TRUE)
    endif()
endif()

# Draco opzionale
find_package(draco CONFIG QUIET)

# Headers matematici
include(CheckIncludeFileCXX)
check_include_file_cxx("cmath" HAVE_CMATH)

# Raccogli sorgenti
file(GLOB SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*MinioClient\\.cpp$")
list(APPEND SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SimpleMinioClient.cpp"
        "external/tinygltf/tiny_gltf.cc"
)

# Crea libreria
add_library(stl2glb ${SOURCES})

# Definizioni
target_compile_definitions(stl2glb PRIVATE
        TINYGLTF_NO_STB_IMAGE_WRITE
        TINYGLTF_NO_STB_IMAGE
        TINYGLTF_NO_EXTERNAL_IMAGE
        TINYGLTF_USE_CPP14
        NDEBUG
)

# Include directories
target_include_directories(stl2glb PUBLIC
        include
        external/tinygltf
)

# Link librerie base
target_link_libraries(stl2glb PUBLIC
        Threads::Threads
        pthread
        dl
)

# Link nlohmann-json
if(nlohmann_json_FOUND)
    target_link_libraries(stl2glb PUBLIC nlohmann_json::nlohmann_json)
    message(STATUS "✅ nlohmann-json found")
else()
    message(FATAL_ERROR "❌ nlohmann-json not found")
endif()

# Link httplib
if(httplib_FOUND)
    target_link_libraries(stl2glb PUBLIC httplib::httplib)
    message(STATUS "✅ httplib found")
else()
    message(FATAL_ERROR "❌ httplib not found")
endif()

# Link OpenSSL - metodo robusto
if(OPENSSL_FOUND)
    if(TARGET OpenSSL::SSL)
        target_link_libraries(stl2glb PUBLIC OpenSSL::SSL OpenSSL::Crypto)
        message(STATUS "✅ OpenSSL found (targets)")
    elseif(OPENSSL_LIBRARIES)
        target_link_libraries(stl2glb PUBLIC ${OPENSSL_LIBRARIES})
        target_include_directories(stl2glb PUBLIC ${OPENSSL_INCLUDE_DIR})
        message(STATUS "✅ OpenSSL found (libraries)")
    endif()
else()
    # Fallback: cerca manualmente
    find_library(SSL_LIB ssl)
    find_library(CRYPTO_LIB crypto)
    if(SSL_LIB AND CRYPTO_LIB)
        target_link_libraries(stl2glb PUBLIC ${SSL_LIB} ${CRYPTO_LIB})
        message(STATUS "✅ OpenSSL found (manual)")
    else()
        message(FATAL_ERROR "❌ OpenSSL not found")
    endif()
endif()

# Link Draco se disponibile
if(draco_FOUND)
    target_link_libraries(stl2glb PUBLIC draco::draco)
    target_compile_definitions(stl2glb PRIVATE USE_DRACO)
    message(STATUS "✅ Draco found")
else()
    message(STATUS "⚠️ Draco not found - compression disabled")
endif()

# Eseguibile
add_executable(stl2glb_exec src/main.cpp)
target_link_libraries(stl2glb_exec PRIVATE stl2glb)

# Strip per ridurre dimensioni
add_custom_command(TARGET stl2glb_exec POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:stl2glb_exec>
        COMMENT "Stripping executable"
)

# Debug output
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Build Summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenSSL: ${OPENSSL_FOUND}")
message(STATUS "  nlohmann-json: ${nlohmann_json_FOUND}")
message(STATUS "  httplib: ${httplib_FOUND}")
message(STATUS "  Draco: ${draco_FOUND}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")