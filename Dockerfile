# Multi-stage build ottimizzato per VPS limitata
FROM ubuntu:22.04 AS builder

# Installa dipendenze di build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git curl ca-certificates ninja-build \
    pkg-config zip unzip tar && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Setup vcpkg
RUN git clone --depth 1 https://github.com/Microsoft/vcpkg.git /vcpkg && \
    /vcpkg/bootstrap-vcpkg.sh -disableMetrics

# Set environment per performance ridotta
ENV VCPKG_MAX_CONCURRENCY=2
ENV MAKEFLAGS="-j2"
ENV CMAKE_BUILD_PARALLEL_LEVEL=2
ENV VCPKG_ROOT=/vcpkg
ENV PATH="$VCPKG_ROOT:$PATH"

# Copia sorgenti
COPY . /project
WORKDIR /project

# Installa dipendenze C++
RUN vcpkg install openssl nlohmann-json cpp-httplib draco --triplet x64-linux

# Build il progetto
RUN cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -GNinja && \
    cmake --build build --config Release --parallel 2

# Runtime stage minimale
FROM ubuntu:22.04 AS runtime

# Installa solo runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libcurl4 ca-certificates && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Crea user non-root
RUN useradd -m -u 1000 -s /bin/bash appuser

WORKDIR /app

# Copia solo l'eseguibile dal builder stage
COPY --from=builder /project/build/stl2glb_exec /app/

# Cambia ownership
RUN chown -R appuser:appuser /app

USER appuser

EXPOSE 8080

# Health check
HEALTHCHECK --interval=60s --timeout=5s --start-period=15s --retries=2 \
    CMD curl -f http://localhost:8080/health || exit 1

# Usa exec form per segnali corretti
ENTRYPOINT ["/app/stl2glb_exec"]