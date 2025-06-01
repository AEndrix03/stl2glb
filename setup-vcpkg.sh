#!/bin/bash

# Script di setup obbligatorio per vcpkg
# Deve essere eseguito PRIMA di docker-compose up

set -e

VCPKG_PATH="/home/vcpkg"
REQUIRED_PACKAGES=("nlohmann-json" "httplib" "openssl")

echo "🔧 Setup vcpkg per stl2glb"
echo "=========================="

# Controllo permessi
if [ "$EUID" -eq 0 ]; then
    echo "❌ Non eseguire questo script come root"
    echo "💡 Usa: ./setup-vcpkg.sh"
    exit 1
fi

# Controllo se vcpkg esiste già
if [ -d "${VCPKG_PATH}" ]; then
    echo "📁 vcpkg trovato in ${VCPKG_PATH}"

    # Controllo se è inizializzato
    if [ ! -f "${VCPKG_PATH}/vcpkg" ]; then
        echo "⚠️  vcpkg non inizializzato, eseguo bootstrap..."
        cd ${VCPKG_PATH}
        ./bootstrap-vcpkg.sh
    else
        echo "✅ vcpkg già inizializzato"
    fi
else
    echo "📦 Installazione vcpkg in ${VCPKG_PATH}..."

    # Crea directory e clona
    mkdir -p /home
    git clone --depth 1 https://github.com/Microsoft/vcpkg.git ${VCPKG_PATH}
    chown -R $USER:$USER ${VCPKG_PATH}

    echo "🏗️  Bootstrap vcpkg..."
    cd ${VCPKG_PATH}
    ./bootstrap-vcpkg.sh
fi

# Controllo dipendenze
echo "🔍 Controllo dipendenze richieste..."
cd ${VCPKG_PATH}

MISSING_PACKAGES=()
for package in "${REQUIRED_PACKAGES[@]}"; do
    if ! ./vcpkg list | grep -q "^${package}:x64-linux"; then
        MISSING_PACKAGES+=("$package")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -gt 0 ]; then
    echo "📦 Installazione pacchetti mancanti: ${MISSING_PACKAGES[*]}"
    ./vcpkg install --triplet=x64-linux --clean-after-build "${MISSING_PACKAGES[@]}"
else
    echo "✅ Tutte le dipendenze sono già installate"
fi

# Verifica finale
echo "🧪 Verifica finale..."
for package in "${REQUIRED_PACKAGES[@]}"; do
    if ./vcpkg list | grep -q "^${package}:x64-linux"; then
        echo "  ✅ ${package}"
    else
        echo "  ❌ ${package} - MANCANTE!"
        exit 1
    fi
done

echo ""
echo "🎉 Setup completato con successo!"
echo "📋 Riepilogo:"
echo "   📁 vcpkg path: ${VCPKG_PATH}"
echo "   📦 Pacchetti: ${REQUIRED_PACKAGES[*]}"
echo ""
echo "🚀 Ora puoi eseguire:"
echo "   docker-compose -f docker-compose-tiny.yml up --build"