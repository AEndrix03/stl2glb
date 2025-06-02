# Runtime image per stl2glb
FROM ubuntu:22.04

# Installa solo le dipendenze runtime necessarie
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    ca-certificates \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Crea gruppo e utente
RUN groupadd -g 1000 appgroup && \
    useradd -m -u 1000 -g appgroup appuser

# Crea directory app
WORKDIR /app

# L'eseguibile verrà montato dal volume build_output
# Non copiamo qui perché viene dal builder

USER appuser
EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://localhost:8080/health || exit 1

# Entrypoint diretto all'eseguibile
ENTRYPOINT ["/app/stl2glb"]