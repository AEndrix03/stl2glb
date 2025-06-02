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

# Copia lo script di avvio e rendilo eseguibile
WORKDIR /app
COPY --chown=appuser:appgroup start.sh /app/start.sh
RUN chmod +x /app/start.sh

USER appuser
EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://localhost:8080/health || exit 1

# Usa lo script di avvio
ENTRYPOINT ["/bin/bash", "/app/start.sh"]
