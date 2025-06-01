#!/bin/bash

# Script per riinstallare vcpkg solo con librerie release
# Risolve il problema: ninja: file is missing and not created by any action: libssl.a

set -e

VCPKG_PATH="/home/vcpkg"
TRIPLET="x64-linux"

echo "🔧 Fix vcpkg - solo librerie release"
echo "====================================="

if [ ! -d "$VCPKG_PATH" ] || [ ! -f "$VCPKG_PATH/vcpkg" ]; then
    echo "❌ vcpkg non trovato in $VCPKG_PATH"
    echo "🔧 Esegui prima: ./setup-vcpkg.sh"
    exit 1
fi

cd "$VCPKG_PATH"

echo "🗑️  Rimozione librerie debug problematiche..."
if [ -d "installed/$TRIPLET/debug" ]; then
    echo "Rimuovendo directory debug..."
    rm -rf "installed/$TRIPLET/debug"
fi

echo "🔄 Reinstallazione solo release..."

# Rimuovi e reinstalla con --only-release
PACKAGES=("openssl" "nlohmann-json" "httplib" "draco")

for PKG in "${PACKAGES[@]}"; do
    echo "🔧 Processando $PKG..."
    
    # Rimuovi se installato
    if ./vcpkg list | grep -q "^${PKG}:${TRIPLET}"; then
        echo "  Rimozione $PKG..."
        ./vcpkg remove "${PKG}:${TRIPLET}" --recurse 2>/dev/null || true
    fi
    
    # Reinstalla solo release
    echo "  Installazione $PKG (solo release)..."
    if ./vcpkg install "${PKG}:${TRIPLET}" --only-release; then
        echo "  ✅ $PKG installato"
    else
        echo "  ⚠️  $PKG fallito con --only-release, provo normale..."
        ./vcpkg install "${PKG}:${TRIPLET}"
    fi
done

echo ""
echo "🔍 Verifica finale..."
echo "Librerie installate:"
./vcpkg list | grep ":$TRIPLET"

echo ""
echo "📁 Controllo directory debug:"
if [ -d "installed/$TRIPLET/debug" ]; then
    echo "❌ Directory debug ancora presente!"
    ls -la "installed/$TRIPLET/debug/"
else
    echo "✅ Nessuna directory debug - perfetto!"
fi

echo ""
echo "📁 Librerie release disponibili:"
if [ -d "installed/$TRIPLET/lib" ]; then
    ls -la "installed/$TRIPLET/lib/" | grep -E "\.(a|so)$" | head -5
else
    echo "❌ Directory lib non trovata!"
fi

echo ""
echo "✅ Fix completato!"
echo "🚀 Ora puoi eseguire: docker-compose -f docker-compose-tiny.yml up builder"