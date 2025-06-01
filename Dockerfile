# Dockerfile-optimized - Build ultra-veloce con vcpkg pre-installato sull'host
# Usa cache mounting e multi-stage build ottimizzato

# Stage 1: Builder minimale (solo compilazione)
FROM ubuntu:22.04 AS builder

# Installa SOLO tools di build essenziali (no vcpkg!)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config \
    ca-certificates ccache && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Setup ccache per build incrementali veloci
ENV CCACHE_DIR=/cache/ccache
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache

# Copia sorgenti
WORKDIR /build
COPY . .

# Fix per Linux: aggiungi #include <cmath> a STLParser.cpp se manca
RUN if ! grep -q "#include <cmath>" src/STLParser.cpp; then \
    sed -i '/#include <atomic>/a #include <cmath>' src/STLParser.cpp; \
    fi

# Build con vcpkg montato dall'host (ultra-veloce!)
# Usa --mount per cache persistente e vcpkg read-only
RUN --mount=type=bind,from=vcpkg,source=/vcpkg,target=/vcpkg,ro \
    --mount=type=cache,target=/cache \
    cmake -B build -S . \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=x64-linux \
        -DVCPKG_INSTALLED_DIR=/vcpkg/installed \
        -GNinja && \
    cmake --build build --config Release --parallel $(nproc)

# Stage 2: Runtime ultra-minimale (distroless)
FROM gcr.io/distroless/cc-debian12:nonroot AS runtime

# Copia solo l'eseguibile
COPY --from=builder --chown=nonroot:nonroot /build/build/stl2glb_exec /app/stl2glb_exec

# Working directory
WORKDIR /app

# Porta
EXPOSE 8080

# Entry point
ENTRYPOINT ["/app/stl2glb_exec"]

# Stage alternativo: Runtime con shell per debug (opzionale)
FROM ubuntu:22.04 AS runtime-debug

# Installa solo runtime essenziali
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 ca-certificates && \
    apt-get clean && rm -rf /var/lib/apt/lists/* && \
    useradd -m -u 1000 appuser

WORKDIR /app
USER appuser

COPY --from=builder --chown=appuser:appuser /build/build/stl2glb_exec /app/

EXPOSE 8080
ENTRYPOINT ["/app/stl2glb_exec"]