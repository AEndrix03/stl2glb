#!/bin/bash
# fast-docker-build.sh - Build ottimizzata con strategie di cache

set -e

# Colori
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🚀 Fast Docker Build for STL2GLB${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Opzioni
USE_EXTERNAL_VCPKG=false
DOCKERFILE="Dockerfile-tiny"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --external-vcpkg)
            USE_EXTERNAL_VCPKG=true
            DOCKERFILE="Dockerfile-tiny-fast"
            shift
            ;;
        --clean)
            echo -e "${YELLOW}🧹 Cleaning Docker cache...${NC}"
            docker builder prune -f
            shift
            ;;
        *)
            shift
            ;;
    esac
done

# Fix STLParser
if ! grep -q "#include <cmath>" src/STLParser.cpp 2>/dev/null; then
    echo -e "${YELLOW}🔧 Applying Linux fix...${NC}"
    sed -i '/#include <atomic>/a #include <cmath>' src/STLParser.cpp
fi

# Build con strategia appropriata
if [ "$USE_EXTERNAL_VCPKG" = true ] && [ -d "/opt/vcpkg" ]; then
    echo -e "${GREEN}📦 Using external vcpkg from /opt/vcpkg${NC}"

    # Crea un build context temporaneo con vcpkg
    TEMP_DIR=$(mktemp -d)
    cp -r . "$TEMP_DIR/"
    cp -r /opt/vcpkg "$TEMP_DIR/vcpkg_host"

    # Build con context esteso
    docker build \
        -f "$DOCKERFILE" \
        --build-arg VCPKG_HOST_DIR=/vcpkg_host \
        -t stl2glb:tiny \
        "$TEMP_DIR"

    # Cleanup
    rm -rf "$TEMP_DIR"
else
    echo -e "${GREEN}📦 Building with internal vcpkg (slower first time)${NC}"

    # Abilita BuildKit per cache migliore
    export DOCKER_BUILDKIT=1

    # Build standard con cache layer
    docker build \
        -f "$DOCKERFILE" \
        -t stl2glb:tiny \
        --build-arg BUILDKIT_INLINE_CACHE=1 \
        .
fi

# Mostra info immagine
echo ""
echo -e "${GREEN}✅ Build completed!${NC}"
docker images stl2glb:tiny --format "table {{.Repository}}\t{{.Tag}}\t{{.Size}}"

# Test veloce
echo ""
echo -e "${BLUE}🧪 Testing executable...${NC}"
docker run --rm stl2glb:tiny /app/stl2glb_exec --version || echo "No --version flag"

echo ""
echo -e "${YELLOW}📝 To run:${NC}"
echo "  docker run -d --name stl2glb_tiny -p 9002:8080 --env-file .env stl2glb:tiny"
echo ""
echo -e "${YELLOW}📝 For faster rebuilds with external vcpkg:${NC}"
echo "  $0 --external-vcpkg"