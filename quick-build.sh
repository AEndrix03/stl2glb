#!/bin/bash

# Script per build veloce con vcpkg esterno
set -e

echo "🚀 Fast build with external vcpkg..."

# Controlla se vcpkg è configurato
if ! docker volume ls | grep -q vcpkg_installed; then
    echo "❌ vcpkg not set up! Run './setup-vcpkg.sh' first"
    exit 1
fi

# Controlla se il fix per STLParser è applicato
if ! grep -q "#include <cmath>" src/STLParser.cpp; then
    echo "🔧 Applying STLParser fix..."
    sed -i '/^#include <atomic>$/a #include <cmath>' src/STLParser.cpp
    echo "✅ STLParser fix applied"
fi

# Build veloce solo dell'app
echo "⚡ Building app (should be ~5x faster)..."
time docker-compose -f docker-compose-external-vcpkg.yml --profile app up --build

echo ""
echo "🎉 Build completed!"
echo "App is running on http://localhost:9002"
echo ""
echo "Test with:"
echo "  curl http://localhost:9002/health"