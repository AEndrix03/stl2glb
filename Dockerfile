# ---------- immagine di runtime per stl2glb ----------
FROM ubuntu:22.04

# Installa le sole dipendenze di runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
      libssl3 \
      ca-certificates \
      wget \
    && rm -rf /var/lib/apt/lists/*

# Crea un utente non-root
RUN groupadd -g 1000 appgroup && \
    useradd -m -u 1000 -g appgroup appuser

# Imposta la working directory (anche se non necessaria, serve per coerenza)
WORKDIR /app
RUN chown -R appuser:appgroup /app

# Switch all'utente non-root
USER appuser

EXPOSE 8080

# Punto di ingresso diretto: esegue /build_output/stl2glb (il binario montato via volume)
ENTRYPOINT ["/build_output/stl2glb"]
