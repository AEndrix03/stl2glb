cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 2.0.0 LANGUAGES CXX)

# Standard C++17 richiesto
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opzioni di build
option(USE_STATIC_LIBS "Build with static libraries" OFF)
option(USE_LTO "Enable Link Time Optimization" ON)
option(USE_NATIVE_ARCH "Optimize for native architecture" ON)

# Rileva il sistema operativo
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
endif()

# Configurazione per tipo di build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Flags di compilazione ottimizzate per piattaforma
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Flags comuni
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

    # Flags per Release
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

    # Ottimizzazioni aggressive per Release
    if(USE_NATIVE_ARCH AND CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mtune=native")
    endif()

    # Link Time Optimization
    if(USE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
    endif()

    # MinSizeRel per container tiny
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-Wl,--gc-sections -s")

elseif(MSVC)
    # Flags MSVC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /GL /DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")

    # Multi-core compilation
    add_compile_options(/MP)

    # Windows specific
    add_definitions(-D_WIN32_WINNT=0x0601)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Setup vcpkg se disponibile
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

# Preferisci librerie statiche per container piccoli
if(USE_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# Trova le librerie richieste - versione corretta per vcpkg
find_package(Threads REQUIRED)

# OpenSSL - compatibile con vcpkg
find_package(OpenSSL REQUIRED)
if(NOT TARGET OpenSSL::SSL)
    # Fallback per versioni più vecchie di CMake o vcpkg
    if(TARGET OpenSSL::OpenSSL)
        set_target_properties(OpenSSL::OpenSSL PROPERTIES IMPORTED_GLOBAL TRUE)
        add_library(OpenSSL::SSL ALIAS OpenSSL::OpenSSL)
        add_library(OpenSSL::Crypto ALIAS OpenSSL::OpenSSL)
    elseif(OPENSSL_FOUND)
        # Crea target manualmente se necessario
        if(NOT TARGET OpenSSL::SSL)
            add_library(OpenSSL::SSL UNKNOWN IMPORTED)
            set_target_properties(OpenSSL::SSL PROPERTIES
                    IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
        endif()
        if(NOT TARGET OpenSSL::Crypto)
            add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
            set_target_properties(OpenSSL::Crypto PROPERTIES
                    IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
        endif()
    endif()
endif()

# nlohmann-json - nome corretto per vcpkg
find_package(nlohmann_json CONFIG REQUIRED)

# httplib - nome corretto per vcpkg
find_package(httplib CONFIG REQUIRED)

# Draco opzionale
find_package(draco CONFIG QUIET)
if(draco_FOUND)
    message(STATUS "Draco compression library found")
    add_definitions(-DUSE_DRACO)
else()
    message(STATUS "Draco not found - compression disabled")
endif()

# Headers di sistema per Linux
if(LINUX)
    # Aggiungi include per headers matematici
    include(CheckIncludeFileCXX)
    check_include_file_cxx("cmath" HAVE_CMATH)
    if(NOT HAVE_CMATH)
        message(FATAL_ERROR "cmath header not found")
    endif()
endif()

# Raccogli sorgenti
file(GLOB SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*MinioClient\\.cpp$")
list(APPEND SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/SimpleMinioClient.cpp"
        "external/tinygltf/tiny_gltf.cc"
)

# Crea la libreria
add_library(stl2glb ${SOURCES})

# Definizioni per ottimizzazioni
target_compile_definitions(stl2glb PRIVATE
        TINYGLTF_NO_STB_IMAGE_WRITE
        TINYGLTF_NO_STB_IMAGE
        TINYGLTF_NO_EXTERNAL_IMAGE
        TINYGLTF_USE_CPP14
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:MinSizeRel>:NDEBUG>
)

# Include directories
target_include_directories(stl2glb PUBLIC
        include
        external/tinygltf
)

# Link delle librerie - versione corretta per vcpkg
target_link_libraries(stl2glb PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
        httplib::httplib
)

# OpenSSL linking - gestione robusta
if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    target_link_libraries(stl2glb PUBLIC OpenSSL::SSL OpenSSL::Crypto)
elseif(TARGET OpenSSL::OpenSSL)
    target_link_libraries(stl2glb PUBLIC OpenSSL::OpenSSL)
elseif(OPENSSL_FOUND)
    target_link_libraries(stl2glb PUBLIC ${OPENSSL_LIBRARIES})
    target_include_directories(stl2glb PUBLIC ${OPENSSL_INCLUDE_DIR})
endif()

# Aggiungi Draco se trovato
if(draco_FOUND)
    target_link_libraries(stl2glb PUBLIC draco::draco)
endif()

# Librerie di sistema platform-specific
if(WIN32)
    target_link_libraries(stl2glb PUBLIC
            ws2_32 crypt32 bcrypt winhttp
    )
elseif(LINUX)
    # Librerie Linux essenziali
    target_link_libraries(stl2glb PUBLIC
            pthread dl
    )

    # Per linking statico completo
    if(USE_STATIC_LIBS)
        target_link_libraries(stl2glb PUBLIC
                -static-libgcc -static-libstdc++
        )
    endif()
endif()

# Eseguibile principale
add_executable(stl2glb_exec src/main.cpp)
target_link_libraries(stl2glb_exec PRIVATE stl2glb)

# Ottimizzazioni post-build
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    # Strip symbols su Linux/Unix
    if(UNIX AND NOT APPLE)
        add_custom_command(TARGET stl2glb_exec POST_BUILD
                COMMAND ${CMAKE_STRIP} $<TARGET_FILE:stl2glb_exec>
                COMMENT "Stripping symbols from executable"
        )
    endif()

    # UPX compression opzionale per MinSizeRel
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        find_program(UPX_EXECUTABLE upx)
        if(UPX_EXECUTABLE)
            add_custom_command(TARGET stl2glb_exec POST_BUILD
                    COMMAND ${UPX_EXECUTABLE} --best --lzma $<TARGET_FILE:stl2glb_exec>
                    COMMENT "Compressing executable with UPX"
            )
        endif()
    endif()
endif()

# Target per test veloce
add_custom_target(test-build
        COMMAND ${CMAKE_COMMAND} -E echo "Testing build..."
        COMMAND $<TARGET_FILE:stl2glb_exec> --version || echo "Version test skipped"
        DEPENDS stl2glb_exec
        COMMENT "Testing executable"
)

# Installa target
install(TARGETS stl2glb_exec
        RUNTIME DESTINATION bin
)

# Debug: mostra informazioni su OpenSSL trovato
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Dependencies found:")
message(STATUS "  OpenSSL found: ${OPENSSL_FOUND}")
if(TARGET OpenSSL::SSL)
    message(STATUS "  OpenSSL::SSL target: YES")
endif()
if(TARGET OpenSSL::Crypto)
    message(STATUS "  OpenSSL::Crypto target: YES")
endif()
if(TARGET OpenSSL::OpenSSL)
    message(STATUS "  OpenSSL::OpenSSL target: YES")
endif()
message(STATUS "  nlohmann_json: ${nlohmann_json_FOUND}")
message(STATUS "  httplib: ${httplib_FOUND}")
message(STATUS "  Draco: ${draco_FOUND}")
message(STATUS "  Threads: ${Threads_FOUND}")

# Mostra configurazione
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Build Configuration:")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  CXX Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
message(STATUS "  Static libs: ${USE_STATIC_LIBS}")
message(STATUS "  LTO: ${USE_LTO}")
message(STATUS "  Native arch: ${USE_NATIVE_ARCH}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")