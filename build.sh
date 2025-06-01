#!/bin/bash
# Build script per Linux con VCPKG

set -e

echo "==================================="
echo "STL2GLB Linux Build Script"
echo "==================================="

# Cerca VCPKG
if [ -f "/home/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    VCPKG_ROOT="/home/vcpkg"
elif [ -f "$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    VCPKG_ROOT="$HOME/vcpkg"
elif [ -n "$VCPKG_ROOT" ]; then
    echo "Using VCPKG_ROOT from environment: $VCPKG_ROOT"
else
    echo "ERROR: VCPKG not found!"
    echo "Please install VCPKG or set VCPKG_ROOT environment variable"
    exit 1
fi

echo "Found VCPKG at: $VCPKG_ROOT"

# Crea directory build
mkdir -p build
cd build

# Configura con CMake
echo ""
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -GNinja

# Build
echo ""
echo "Building..."
cmake --build . --config Release --parallel $(nproc)

# Strip dell'eseguibile
if [ -f "stl2glb" ]; then
    echo "Stripping executable..."
    strip stl2glb
fi

echo ""
echo "==================================="
echo "Build completed successfully!"
echo "Executable: build/stl2glb"
echo "==================================="

cd ..