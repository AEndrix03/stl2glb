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

# Crea directory app con permessi corretti
WORKDIR /app
RUN chown -R appuser:appgroup /app

# Crea uno script di avvio che copia l'eseguibile dal volume
RUN echo '#!/bin/bash\n\
if [ -f /build_output/stl2glb ]; then\n\
    cp /build_output/stl2glb /app/stl2glb\n\
    chmod +x /app/stl2glb\n\
    exec /app/stl2glb\n\
else\n\
    echo "Error: stl2glb executable not found in /build_output"\n\
    exit 1\n\
fi' > /app/start.sh && \
    chmod +x /app/start.sh && \
    chown appuser:appgroup /app/start.sh

USER appuser
EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://localhost:8080/health || exit 1

# Usa lo script di avvio
ENTRYPOINT ["/bin/bash", "/app/start.sh"]