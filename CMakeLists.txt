cmake_minimum_required(VERSION 3.16)
project(stl2glb VERSION 2.0.0 LANGUAGES CXX)

# Standard C++17 richiesto
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Abilita ottimizzazioni per release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Ottimizzazioni aggressive per massima performance
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -flto -ffast-math -funroll-loops -fprefetch-loop-arrays")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto -s")
    elseif(MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "/O2 /GL /fp:fast /arch:AVX2")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
    endif()
endif()

# Assicurati che vcpkg sia utilizzato correttamente
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

# Controllo per ambiente Windows
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_WIN32_WINNT=0x0601)
    # Abilita multi-threading per compilazione MSVC
    if(MSVC)
        add_compile_options(/MP)
    endif()
endif()

# Trova le librerie richieste
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Prova a trovare Draco (opzionale)
find_package(draco CONFIG QUIET)
if(draco_FOUND)
    message(STATUS "Draco compression library found - enabling mesh compression")
    add_definitions(-DUSE_DRACO)
else()
    message(STATUS "Draco not found - mesh compression disabled")
endif()

# Raccogli sorgenti cpp dalla cartella src
file(GLOB SOURCES "src/*.cpp")

# Rimuovi MinioClient.cpp (useremo SimpleMinioClient invece)
list(FILTER SOURCES EXCLUDE REGEX ".*MinioClient\\.cpp$")

# Aggiungi il nuovo SimpleMinioClient.cpp
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/SimpleMinioClient.cpp")

# Aggiungi la fonte di tinygltf
list(APPEND SOURCES "external/tinygltf/tiny_gltf.cc")

# Crea la libreria
add_library(stl2glb ${SOURCES})

# Definisci macro per ottimizzazioni
target_compile_definitions(stl2glb PRIVATE
        TINYGLTF_NO_STB_IMAGE_WRITE
        TINYGLTF_NO_STB_IMAGE
        TINYGLTF_NO_EXTERNAL_IMAGE
        TINYGLTF_USE_CPP14
)

# Imposta include directory pubbliche
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
        Threads::Threads
)

# Aggiungi Draco se trovato
if(draco_FOUND)
    target_link_libraries(stl2glb PUBLIC draco::draco)
endif()

# In Windows, specifica librerie di sistema
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

# Ottimizzazioni specifiche per l'eseguibile
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Strip symbols in release
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_custom_command(TARGET stl2glb_exec POST_BUILD
                COMMAND ${CMAKE_STRIP} $<TARGET_FILE:stl2glb_exec>
        )
    endif()
endif()

# Installa target (opzionale)
install(TARGETS stl2glb_exec
        RUNTIME DESTINATION bin
)

# Se utilizzi un target Debug in Windows, assicurati di usare librerie debug
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(stl2glb PUBLIC _DEBUG)
endif()

# Mostra configurazione
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CXX Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
message(STATUS "Draco support: ${draco_FOUND}")