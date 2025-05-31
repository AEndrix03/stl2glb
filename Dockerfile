# Multi-stage build con vcpkg esterno
FROM ubuntu:22.04 AS builder

# Installa solo tools essenziali (vcpkg già installato esternamente)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config \
    curl ca-certificates && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Set environment per vcpkg esterno
ENV VCPKG_ROOT=/vcpkg_external
ENV PATH="$VCPKG_ROOT:$PATH"
ENV MAKEFLAGS="-j2"
ENV CMAKE_BUILD_PARALLEL_LEVEL=2

# Copia sorgenti
COPY . /project
WORKDIR /project

# Build il progetto (vcpkg già pronto)
RUN cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -GNinja && \
    cmake --build build --config Release --parallel 2

# Runtime stage minimale (identico)
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libcurl4 ca-certificates curl && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN useradd -m -u 1000 -s /bin/bash appuser

WORKDIR /app
USER appuser

COPY --from=builder --chown=appuser:appuser /project/build/stl2glb_exec /app/

RUN chmod -R 777 /app

EXPOSE 8080

HEALTHCHECK --interval=60s --timeout=5s --start-period=15s --retries=2 \
    CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["/app/stl2glb_exec"]