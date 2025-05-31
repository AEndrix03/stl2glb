#!/bin/bash

# Script per setup iniziale di vcpkg (eseguire una sola volta)
set -e

echo "🚀 Setting up external vcpkg for fast Docker builds..."

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Funzione per log colorati
log_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

log_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

log_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

log_error() {
    echo -e "${RED}❌ $1${NC}"
}

# Controlla se Docker è in esecuzione
if ! docker info > /dev/null 2>&1; then
    log_error "Docker is not running!"
    exit 1
fi

# Setup iniziale vcpkg (solo prima volta)
log_info "Setting up vcpkg infrastructure..."
docker-compose -f docker-compose-external-vcpkg.yml --profile setup up vcpkg-setup

# Installa dipendenze (solo prima volta)
log_info "Installing C++ dependencies..."
docker-compose -f docker-compose-external-vcpkg.yml --profile setup up vcpkg-install

log_success "vcpkg setup completed!"
log_info "Now you can build the app super fast with:"
echo ""
echo "  docker-compose -f docker-compose-tiny.yml --profile app up --build"
echo ""
log_info "Subsequent builds will be ~5x faster because vcpkg is cached!"

# Mostra informazioni sui volumi
echo ""
log_info "Docker volumes created:"
docker volume ls | grep stl2glb || echo "  (no volumes found - this is normal for first run)"

# Verifica installazione
echo ""
log_info "Verifying vcpkg installation..."
if docker run --rm -v stl2glb_vcpkg_installed:/vcpkg ubuntu:22.04 ls -la /vcpkg/vcpkg > /dev/null 2>&1; then
    log_success "vcpkg binary found and ready!"
else
    log_warning "vcpkg verification failed, but this might be normal on first setup"
fi

echo ""
log_success "Setup completed! 🎉"