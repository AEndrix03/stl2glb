# Dockerfile che usa VCPKG montato dall'host
# Richiede: docker build --build-arg VCPKG_DIR=/path/to/vcpkg -v /home/vcpkg:/vcpkg:ro

# Stage 1: Builder
FROM alpine:3.19 AS builder

RUN apk add --no-cache \
    build-base \
    cmake \
    ninja \
    linux-headers \
    && rm -rf /var/cache/apk/*

WORKDIR /build

# Copia sorgenti
COPY . .

# Fix per Linux
RUN if [ -f "src/STLParser.cpp" ]; then \
    grep -q "#include <cmath>" src/STLParser.cpp || \
    sed -i '/#include <atomic>/a #include <cmath>' src/STLParser.cpp; \
    fi

# Copia script di build
COPY <<'EOF' /build/build.sh
#!/bin/sh
set -e

VCPKG_DIR="${1:-/vcpkg}"
BUILD_TYPE="${2:-MinSizeRel}"

echo "Checking VCPKG at: $VCPKG_DIR"
if [ ! -f "$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" ]; then
    echo "ERROR: VCPKG not found at $VCPKG_DIR"
    echo "Contents of $VCPKG_DIR:"
    ls -la "$VCPKG_DIR" 2>/dev/null || echo "Directory not accessible"
    exit 1
fi

echo "VCPKG found, starting build..."
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DVCPKG_INSTALLED_DIR=$VCPKG_DIR/installed \
    -GNinja

cmake --build build --config $BUILD_TYPE --parallel $(nproc)
strip build/stl2glb
echo "Build completed successfully!"
EOF

RUN chmod +x /build/build.sh

# Il build effettivo avviene nel docker-compose con volume montato
CMD ["/build/build.sh", "/vcpkg", "MinSizeRel"]

# Stage 2: Runtime
FROM alpine:3.19 AS runtime

RUN apk add --no-cache \
    libstdc++ \
    libgcc \
    openssl \
    ca-certificates \
    && rm -rf /var/cache/apk/* \
    && adduser -D -u 1000 -g 1000 appuser

WORKDIR /app

# Nota: il file viene copiato dal builder tramite docker-compose
COPY --from=builder --chown=appuser:appuser /build/build/stl2glb /app/stl2glb || true

USER appuser
EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://localhost:8080/health || exit 1

ENTRYPOINT ["/app/stl2glb"]