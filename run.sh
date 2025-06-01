#!/bin/bash

# Script completo per build e run stl2glb
# Prepara vcpkg, lancia docker-compose e pulisce

set -e

VCPKG_HOST_PATH="/opt/vcpkg"
VCPKG_BUILD_PATH="./vcpkg"
COMPOSE_FILE="docker-compose-tiny.yml"

echo "🚀 Build e avvio completo stl2glb"
echo "================================="

# Funzione di cleanup
cleanup() {
    if [ -d "${VCPKG_BUILD_PATH}" ]; then
        echo "🧹 Cleanup vcpkg build context..."
        rm -rf "${VCPKG_BUILD_PATH}"
    fi
}

# Cleanup automatico in caso di errore o uscita
trap cleanup EXIT

# 1. Verifica vcpkg nell'host
echo "🔍 Verifica vcpkg nell'host..."
if [ ! -d "${VCPKG_HOST_PATH}" ] || [ ! -f "${VCPKG_HOST_PATH}/vcpkg" ]; then
    echo "❌ vcpkg non trovato o non inizializzato in ${VCPKG_HOST_PATH}"
    echo "🔧 Esegui prima: ./setup-vcpkg.sh"
    exit 1
fi

# 2. Verifica dipendenze
echo "📦 Verifica dipendenze..."
cd "${VCPKG_HOST_PATH}"
REQUIRED_PACKAGES=("nlohmann-json" "httplib" "openssl")

for package in "${REQUIRED_PACKAGES[@]}"; do
    if ! ./vcpkg list | grep -q "^${package}:x64-linux"; then
        echo "❌ Pacchetto mancante: ${package}"
        echo "🔧 Esegui prima: ./setup-vcpkg.sh"
        exit 1
    fi
done

cd - > /dev/null
echo "✅ Tutte le dipendenze sono disponibili"

# 3. Prepara build context
echo "📁 Preparazione build context..."
if [ -d "${VCPKG_BUILD_PATH}" ]; then
    rm -rf "${VCPKG_BUILD_PATH}"
fi

echo "📋 Copia vcpkg (questo potrebbe richiedere alcuni secondi)..."
cp -r "${VCPKG_HOST_PATH}" "${VCPKG_BUILD_PATH}"

if [ ! -f "${VCPKG_BUILD_PATH}/vcpkg" ]; then
    echo "❌ Errore nella copia di vcpkg"
    exit 1
fi

echo "✅ Build context preparato"

# 4. Build e avvio con docker-compose
echo "🐳 Build e avvio con docker-compose..."
export DOCKER_BUILDKIT=1
export COMPOSE_DOCKER_CLI_BUILD=1

docker-compose -f "${COMPOSE_FILE}" up --build -d

echo ""
echo "✅ Servizio avviato con successo!"
echo "🌐 Applicazione disponibile su: http://localhost:9002"
echo ""
echo "📋 Comandi utili:"
echo "   docker-compose -f ${COMPOSE_FILE} logs -f     # Log in tempo reale"
echo "   docker-compose -f ${COMPOSE_FILE} stop        # Ferma servizio"
echo "   docker-compose -f ${COMPOSE_FILE} down        # Rimuovi container"
echo "   docker-compose -f ${COMPOSE_FILE} restart     # Riavvia servizio"

# Il cleanup avverrà automaticamente grazie al trap EXIT