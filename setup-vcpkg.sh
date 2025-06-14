#!/bin/bash

# Setup pulito di vcpkg - risolve problemi di dipendenze debug
set -e

VCPKG_PATH="/home/vcpkg"
TRIPLET="x64-linux"

echo "🧹 Setup pulito vcpkg per stl2glb"
echo "================================="

if [ -d "$VCPKG_PATH" ]; then
    echo "🗑️  Rimozione vcpkg esistente..."
    rm -rf "$VCPKG_PATH"
fi

echo "📥 Clone vcpkg..."
git clone --depth 1 https://github.com/Microsoft/vcpkg.git "$VCPKG_PATH"
cd "$VCPKG_PATH"

echo "🏗️  Bootstrap vcpkg..."
./bootstrap-vcpkg.sh -disableMetrics

echo "📦 Installazione dipendenze solo release..."
PACKAGES=("openssl" "nlohmann-json" "cpp-httplib")

for PKG in "${PACKAGES[@]}"; do
    echo "Installing $PKG..."
    # Prova senza --only-release (non supportato in questa versione)
    if ./vcpkg install "${PKG}:${TRIPLET}"; then
        echo "✅ $PKG installato"

        # Rimuovi debug se esiste
        if [ -d "installed/${TRIPLET}/debug" ]; then
            echo "🗑️  Rimozione debug per $PKG..."
            rm -rf "installed/${TRIPLET}/debug"
        fi
    else
        echo "❌ Fallito: $PKG"
        if [[ "$PKG" == "cpp-httplib" ]]; then
            echo "⚠️  Provo con httplib..."
            if ./vcpkg install "httplib:${TRIPLET}"; then
                echo "✅ httplib installato"
            fi
        fi
    fi
done

# Draco opzionale
echo "📦 Installazione draco (opzionale)..."
if ./vcpkg install "draco:${TRIPLET}"; then
    echo "✅ Draco installato"
    if [ -d "installed/${TRIPLET}/debug" ]; then
        echo "🗑️  Rimozione debug draco..."
        rm -rf "installed/${TRIPLET}/debug"
    fi
else
    echo "⚠️  Draco failed - continuing without"
fi

# Pulizia finale di tutte le directory debug
echo "🗑️  Pulizia finale debug..."
if [ -d "installed/${TRIPLET}/debug" ]; then
    rm -rf "installed/${TRIPLET}/debug"
fi

echo ""
echo "🔍 Verifica installazione finale..."
echo "Packages installati:"
./vcpkg list | grep ":${TRIPLET}"

echo ""
echo "📁 Controllo directory problematiche:"
if [ -d "installed/${TRIPLET}/debug" ]; then
    echo "❌ Directory debug ancora presente:"
    ls -la "installed/${TRIPLET}/debug/"
    echo "🗑️  Rimozione forzata..."
    rm -rf "installed/${TRIPLET}/debug"
fi

echo "✅ Directory debug: $([ -d "installed/${TRIPLET}/debug" ] && echo "PRESENTE" || echo "RIMOSSA")"

echo ""
echo "📁 Librerie release:"
ls -la "installed/${TRIPLET}/lib/" | grep -E "\.(a|so)$" | head -5

echo ""
echo "🔧 Correzione permessi..."
chown -R $USER:$USER "$VCPKG_PATH" 2>/dev/null || true

echo ""
echo "✅ Setup pulito completato!"
echo ""
echo "🚀 Ora puoi eseguire:"
echo "   docker-compose -f docker-compose-tiny.yml up builder"