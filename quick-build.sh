#!/bin/bash
# fast-build.sh - Build ultra-veloce con vcpkg pre-installato

set -e

# Colori
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}🚀 Fast Build System for STL2GLB${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Controlla se vcpkg è configurato
if [ -z "${VCPKG_HOST_DIR}" ]; then
    # Cerca in posizioni comuni
    if [ -d "/opt/vcpkg" ]; then
        export VCPKG_HOST_DIR="/opt/vcpkg"
    elif [ -d "$HOME/vcpkg" ]; then
        export VCPKG_HOST_DIR="$HOME/vcpkg"
    else
        echo -e "${RED}❌ vcpkg not found!${NC}"
        echo "Please run ./setup-vcpkg-host.sh first"
        exit 1
    fi
fi

echo -e "${GREEN}✅ Using vcpkg from: $VCPKG_HOST_DIR${NC}"

# Applica fix Linux se necessario
if ! grep -q "#include <cmath>" src/STLParser.cpp 2>/dev/null; then
    echo -e "${YELLOW}🔧 Applying Linux fix to STLParser.cpp...${NC}"
    sed -i '/#include <atomic>/a #include <cmath>' src/STLParser.cpp
fi

# Abilita BuildKit per Docker
export DOCKER_BUILDKIT=1
export COMPOSE_DOCKER_CLI_BUILD=1
export BUILDKIT_PROGRESS=plain

# Determina profilo
PROFILE="default"
if [ "$1" = "tiny" ]; then
    PROFILE="tiny"
    echo -e "${YELLOW}📦 Building TINY version for VPS...${NC}"
    COMPOSE_FILE="docker-compose-tiny-optimized.yml"
else
    echo -e "${GREEN}📦 Building standard version...${NC}"
    COMPOSE_FILE="docker-compose-optimized.yml"
fi

# Tempo di inizio
START_TIME=$(date +%s)

# Build con Docker Compose
echo -e "${GREEN}🔨 Building with Docker Compose...${NC}"
VCPKG_HOST_DIR=$VCPKG_HOST_DIR docker-compose -f $COMPOSE_FILE build --parallel

# Calcola tempo
END_TIME=$(date +%s)
BUILD_TIME=$((END_TIME - START_TIME))

echo ""
echo -e "${GREEN}✅ Build completed in ${BUILD_TIME} seconds!${NC}"
echo ""

# Avvia servizio
read -p "Start the service now? (Y/n): " START_SERVICE
if [[ ! "$START_SERVICE" =~ ^[Nn]$ ]]; then
    echo -e "${GREEN}🚀 Starting service...${NC}"
    VCPKG_HOST_DIR=$VCPKG_HOST_DIR docker-compose -f $COMPOSE_FILE up -d

    # Attendi che sia pronto
    echo -n "Waiting for service to be ready"
    for i in {1..10}; do
        if curl -s http://localhost:9002/health > /dev/null 2>&1; then
            echo -e "\n${GREEN}✅ Service is ready!${NC}"
            echo "URL: http://localhost:9002"
            break
        fi
        echo -n "."
        sleep 1
    done
fi

echo ""
echo "📊 Build Statistics:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Build time: ${BUILD_TIME}s"
echo "Image size: $(docker images stl2glb:${PROFILE:-optimized} --format '{{.Size}}')"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"