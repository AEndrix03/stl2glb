cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 2.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Ottimizzazioni per dimensioni ridotte
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    if(MSVC)
        # Windows/MSVC optimizations
        set(CMAKE_CXX_FLAGS_RELEASE "/O1 /Os /GL /DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF")
    else()
        # Linux/GCC/Clang optimizations
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
        set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG -ffunction-sections -fdata-sections")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -s")
    endif()
endif()

# Trova VCPKG - supporta sia ambiente Windows che Linux
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # Windows default paths
    if(WIN32)
        if(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        elseif(EXISTS "C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake")
        elseif(EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        endif()
    else()
        # Linux default paths
        if(EXISTS "/home/vcpkg/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "/home/vcpkg/scripts/buildsystems/vcpkg.cmake")
        elseif(EXISTS "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake")
        elseif(EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        endif()
    endif()
endif()

# Verifica che VCPKG sia disponibile
if(CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using VCPKG: ${CMAKE_TOOLCHAIN_FILE}")
else()
    message(WARNING "VCPKG toolchain not found. Please set CMAKE_TOOLCHAIN_FILE or VCPKG_ROOT")
endif()

# Trova le dipendenze richieste
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

# Draco è opzionale
find_package(draco CONFIG QUIET)
if(draco_FOUND)
    message(STATUS "Draco compression enabled")
    set(USE_DRACO ON)
else()
    message(STATUS "Draco not found - compression disabled")
    set(USE_DRACO OFF)
endif()

# Raccogli i sorgenti
file(GLOB_RECURSE SOURCES
        "src/*.cpp"
        "external/tinygltf/tiny_gltf.cc"
)

# Escludi MinioClient.cpp se presente
list(FILTER SOURCES EXCLUDE REGEX ".*MinioClient\\.cpp$")

# Crea la libreria
add_library(stl2glb_lib STATIC ${SOURCES})

# Definizioni preprocessore
target_compile_definitions(stl2glb_lib PRIVATE
        TINYGLTF_NO_STB_IMAGE_WRITE
        TINYGLTF_NO_STB_IMAGE
        TINYGLTF_NO_EXTERNAL_IMAGE
        TINYGLTF_USE_CPP14
        $<$<CONFIG:Release,MinSizeRel>:NDEBUG>
)

if(USE_DRACO)
    target_compile_definitions(stl2glb_lib PRIVATE USE_DRACO)
endif()

# Include directories
target_include_directories(stl2glb_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf
)

# Link delle librerie
target_link_libraries(stl2glb_lib PUBLIC
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        httplib::httplib
)

# Link Draco se disponibile
if(USE_DRACO)
    target_link_libraries(stl2glb_lib PUBLIC draco::draco)
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(stl2glb_lib PUBLIC ws2_32)
else()
    target_link_libraries(stl2glb_lib PUBLIC pthread dl)
endif()

# Crea l'eseguibile
add_executable(stl2glb_exec src/main.cpp)
set_target_properties(stl2glb_exec PROPERTIES OUTPUT_NAME stl2glb)
target_link_libraries(stl2glb_exec PRIVATE stl2glb_lib)

# Strip symbols su Linux in Release
if(NOT WIN32 AND (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel"))
    add_custom_command(TARGET stl2glb_exec POST_BUILD
            COMMAND ${CMAKE_STRIP} $<TARGET_FILE:stl2glb_exec>
            COMMENT "Stripping executable"
    )
endif()

# Installa l'eseguibile
install(TARGETS stl2glb_exec
        RUNTIME DESTINATION bin
)

# Summary
message(STATUS "=====================================")
message(STATUS "Build Configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Draco Support: ${USE_DRACO}")
message(STATUS "=====================================")