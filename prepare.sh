#!/bin/bash

# Script per preparare vcpkg nel build context
# Da eseguire PRIMA di docker-compose up --build

set -e

VCPKG_HOST_PATH="/opt/vcpkg"
VCPKG_BUILD_PATH="./vcpkg"

echo "🔧 Preparazione build context per stl2glb"
echo "========================================="

# Verifica vcpkg nell'host
if [ ! -d "${VCPKG_HOST_PATH}" ]; then
    echo "❌ vcpkg non trovato in ${VCPKG_HOST_PATH}"
    echo "🔧 Esegui prima: ./setup-vcpkg.sh"
    exit 1
fi

if [ ! -f "${VCPKG_HOST_PATH}/vcpkg" ]; then
    echo "❌ vcpkg non inizializzato in ${VCPKG_HOST_PATH}"
    echo "🔧 Esegui prima: ./setup-vcpkg.sh"
    exit 1
fi

# Verifica dipendenze richieste
echo "🔍 Verifica dipendenze vcpkg..."
cd "${VCPKG_HOST_PATH}"

REQUIRED_PACKAGES=("nlohmann-json" "httplib" "openssl")
MISSING_PACKAGES=()

for package in "${REQUIRED_PACKAGES[@]}"; do
    if ./vcpkg list | grep -q "^${package}:x64-linux"; then
        echo "  ✅ ${package}"
    else
        echo "  ❌ ${package} - MANCANTE"
        MISSING_PACKAGES+=("$package")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -gt 0 ]; then
    echo "🔧 Esegui prima: ./setup-vcpkg.sh"
    exit 1
fi

cd - > /dev/null

# Prepara vcpkg per il build
echo "📁 Preparazione vcpkg per build context..."

if [ -d "${VCPKG_BUILD_PATH}" ]; then
    echo "🧹 Rimozione vcpkg build precedente..."
    rm -rf "${VCPKG_BUILD_PATH}"
fi

echo "📋 Copia vcpkg nel build context..."
cp -r "${VCPKG_HOST_PATH}" "${VCPKG_BUILD_PATH}"

# Verifica copia
if [ ! -f "${VCPKG_BUILD_PATH}/vcpkg" ]; then
    echo "❌ Errore nella copia di vcpkg"
    exit 1
fi

echo "✅ vcpkg preparato per il build"
echo ""
echo "🚀 Ora puoi eseguire:"
echo "   docker-compose -f docker-compose-tiny.yml up --build"
echo ""
echo "🧹 Per pulire dopo il build:"
echo "   rm -rf ${VCPKG_BUILD_PATH}"