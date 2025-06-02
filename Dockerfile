# ---------- Dockerfile.runtime (solo runtime) ----------
FROM ubuntu:22.04

# 1) Installa soltanto le librerie di cui il tuo stl2glb ha bisogno per girare
#    (adatta l’elenco se servono altre .so)
RUN apt-get update && apt-get install -y --no-install-recommends \
      libssl3 \
      ca-certificates \
      wget \
    && rm -rf /var/lib/apt/lists/*

# 2) Crea un utente “appuser” non-root per ragioni di sicurezza
RUN groupadd -g 1000 appgroup \
    && useradd -m -u 1000 -g appgroup appuser

# 3) Fai diventare /app proprietà di appuser
WORKDIR /app
RUN chown -R appuser:appgroup /app

# 4) Copia il binario dal contesto di build
#    Attenzione: quando chiameremo "docker build" dovremo fare build dentro la cartella che contiene "build/stl2glb".
COPY build/stl2glb /app/stl2glb
RUN chmod +x /app/stl2glb \
    && chown appuser:appgroup /app/stl2glb

# 5) Passa a utente non-root
USER appuser

# 6) Espone la porta sulla quale stl2glb serve (ad esempio 8080)
EXPOSE 8080

# 7) ENTRYPOINT: esegue direttamente il binario
ENTRYPOINT ["/app/stl2glb"]
