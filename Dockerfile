# ------------ solo runtime, assume che /build_output contenga already +x il binario ----------
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
      libssl3 \
      ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd -g 1000 appgroup && useradd -m -u 1000 -g appgroup appuser

# NON copiare nulla in /app: stiamo per lanciare il binario che viene montato via volume
WORKDIR /app
RUN chown -R appuser:appgroup /app

USER appuser
EXPOSE 8080

# ENTRYPOINT diretto, senza /bin/bash
ENTRYPOINT ["/build_output/stl2glb"]
