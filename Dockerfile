# Dockerfile per Linux - Build con VCPKG montato dall'host
# Ottimizzato per dimensioni minime e risorse limitate

# Stage 1: Builder
FROM alpine:3.19 AS builder

# Installa solo tools essenziali per la build
RUN apk add --no-cache \
    build-base \
    cmake \
    ninja \
    linux-headers \
    openssl-dev \
    && rm -rf /var/cache/apk/*

# Working directory
WORKDIR /build

# Copia sorgenti
COPY . .

# Fix per Linux: aggiungi #include <cmath> se necessario
RUN if [ -f "src/STLParser.cpp" ]; then \
    grep -q "#include <cmath>" src/STLParser.cpp || \
    sed -i '/#include <atomic>/a #include <cmath>' src/STLParser.cpp; \
    fi

# Build con VCPKG montato da volume esterno
# Nota: VCPKG_DIR sarà montato come volume in docker-compose
ARG VCPKG_DIR=/vcpkg
ARG BUILD_TYPE=MinSizeRel

RUN cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DVCPKG_INSTALLED_DIR=${VCPKG_DIR}/installed \
    -GNinja \
    && cmake --build build --config ${BUILD_TYPE} --parallel $(nproc) \
    && strip build/stl2glb

# Stage 2: Runtime minimale
FROM alpine:3.19 AS runtime

# Installa solo le runtime libraries essenziali
RUN apk add --no-cache \
    libstdc++ \
    libgcc \
    openssl \
    ca-certificates \
    && rm -rf /var/cache/apk/* \
    && adduser -D -u 1000 -g 1000 appuser

# Working directory
WORKDIR /app

# Copia solo l'eseguibile
COPY --from=builder --chown=appuser:appuser /build/build/stl2glb /app/stl2glb

# Usa utente non-root
USER appuser

# Porta
EXPOSE 8080

# Health check endpoint
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://localhost:8080/health || exit 1

# Entrypoint
ENTRYPOINT ["/app/stl2glb"]