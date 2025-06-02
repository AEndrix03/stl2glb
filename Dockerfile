# 1. Partiamo da Ubuntu 22.04
FROM ubuntu:22.04

# 2. Installiamo SOLO il C++ runtime e OpenSSL (e ca-certificates, wget nel caso)
RUN apt-get update && apt-get install -y --no-install-recommends \
      libstdc++6 \
      libssl3 \
      ca-certificates \
      wget \
    && rm -rf /var/lib/apt/lists/*

# 3. Creiamo un utente non-root (opzionale, ma buona pratica)
RUN groupadd -g 1000 appgroup \
    && useradd -m -u 1000 -g appgroup appuser

# 4. Creiamo la cartella /app e diamo i permessi
WORKDIR /app
RUN chown -R appuser:appgroup /app

# 5. Copiamo il binario pre-buildato dentro /app/stl2glb
#    ATTENZIONE: qui si assume che il Docker build venga lanciato
#    da dentro la cartella che contiene “build/”
COPY build/stl2glb /app/stl2glb

# 6. Assicuriamoci che il binario sia eseguibile e di proprietà di appuser
RUN chmod +x /app/stl2glb \
    && chown appuser:appgroup /app/stl2glb

# 7. Switch a utente NON root
USER appuser

# 8. Esponiamo la porta 8080 (o quella che usa stl2glb)
EXPOSE 8080

# 9. ENTRYPOINT: eseguiamo direttamente il binario, senza shell wrapper
ENTRYPOINT ["/app/stl2glb"]
