# ------------------------ STAGE 1: builder ------------------------
FROM ubuntu:22.04 AS builder

# Installa ciò che serve per buildare (git, cmake, ninja, vcpkg, gcc, ecc.)
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      ninja-build \
      wget \
      ca-certificates \
      git \
    && rm -rf /var/lib/apt/lists/*

# (1) Monta il tuo sorgente e VCPKG come bind‐mount quando usi docker-compose
#     oppure, se hai VCPKG in .git sotto progetto, cambia di conseguenza.
#    L’idea è che, con docker-compose, arriverà un bind‐mount su /src e /vcpkg.

WORKDIR /src
# Nel tuo docker-compose mounterai:
#   - ./:/src:ro
#   - /home/vcpkg:/vcpkg:ro
#
# A questo punto, puoi lanciare i comandi cmake + ninja all’interno di /src.

# Prepara la cartella di build
RUN mkdir -p /build
WORKDIR /build

# Configura CMake per usare VCPKG
# (assumo che all’interno di /vcpkg ci sia la toolchain di VCPKG)
ARG VCPKG_ROOT=/vcpkg
ARG VCPKG_TRIPLET=x64-linux

# Crea progetto con CMake + VCPKG
# Nota: puoi modificare i flag di CMake come serve a te
RUN cmake -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
      -DVCPKG_TARGET_TRIPLET="${VCPKG_TRIPLET}" \
      -DVCPKG_INSTALLED_DIR="${VCPKG_ROOT}/installed" \
      -S /src \
      -B /build

# Builda e strippa
RUN cmake --build /build --config Release --parallel \
    && strip /build/stl2glb

# Alla fine di questo stage, il binario “/build/stl2glb” è pronto e già eseguibile.


# --------------------- STAGE 2: runtime image ----------------------
FROM ubuntu:22.04 AS runtime

# Installa SOLO le dipendenze di runtime (quelle che servono al tuo stl2glb)
RUN apt-get update && apt-get install -y --no-install-recommends \
      libssl3 \
      ca-certificates \
      wget \
    && rm -rf /var/lib/apt/lists/*

# Crea utente non‐root
RUN groupadd -g 1000 appgroup \
    && useradd -m -u 1000 -g appgroup appuser

# Crea working‐directory e assicura i permessi
WORKDIR /app
RUN chown -R appuser:appgroup /app

# Copia il binario compilato dallo stage “builder”
# (il nome “builder” è esattamente l’alias del primo FROM)
COPY --from=builder /build/stl2glb /app/stl2glb

# Chiunque apra /app/stl2glb lo vede già +x:
RUN chmod +x /app/stl2glb \
    && chown appuser:appgroup /app/stl2glb

USER appuser
EXPOSE 8080

# Facoltativo: healthcheck
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD wget --no-verbose --tries=1 --spider http://localhost:8080/health || exit 1

# Infine, facciamo diventare direttamente “/app/stl2glb” l’entrypoint,
# senza passare da bash o da uno script intermedio.
ENTRYPOINT ["/app/stl2glb"]
