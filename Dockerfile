###
### STAGE 1: builder (usa il tuo build.sh per compilare)
###
FROM ubuntu:22.04 AS builder

# 1. Installa le dipendenze di build (adatta qui di seguito se il tuo build.sh necessita
#    di altri pacchetti: gcc, g++, make, cmake, ninja, vcpkg, git, librerie di sviluppo, ecc.)
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      ninja-build \
      wget \
      ca-certificates \
      git \
    && rm -rf /var/lib/apt/lists/*

# 2. Crea la working directory e copia TUTTO il tuo progetto (incl. build.sh)
WORKDIR /src
COPY . .

# 3. Rendi eseguibile build.sh e lancialo
#    (assicurati che, nel repository, build.sh abbia già i permessi corretti
#     oppure il chmod +x qui glieli mette)
RUN chmod +x ./build.sh \
    && ./build.sh

#  A questo punto ci deve essere un eseguibile “/src/build/stl2glb” creato da build.sh.
#  Per sicurezza, controlla:
RUN ls -l /src/build/stl2glb


###
### STAGE 2: runtime (solo le dipendenze di esecuzione + binario)
###
FROM ubuntu:22.04 AS runtime

# 4. Installa SOLO le librerie di runtime necessarie al tuo stl2glb
#    (adatta pure qui: libssl, libstdc++, ca-certificates, ecc. secondo le dipendenze econometriche)
RUN apt-get update && apt-get install -y --no-install-recommends \
      libssl3 \
      ca-certificates \
      wget \
    && rm -rf /var/lib/apt/lists/*

# 5. Crea un utente non-root (per ragioni di sicurezza)
RUN groupadd -g 1000 appgroup \
    && useradd -m -u 1000 -g appgroup appuser

# 6. Prepara /app, copia il binario dal builder e assicurati che sia +x
WORKDIR /app
RUN chown -R appuser:appgroup /app

COPY --from=builder /src/build/stl2glb /app/stl2glb
RUN chmod +x /app/stl2glb \
    && chown appuser:appgroup /app/stl2glb

# 7. Switch all’utente non-root
USER appuser

# 8. Espone la porta (se il tuo stl2glb serve su 8080)
EXPOSE 8080

# 9. Definisce l’entrata diretta: lancia il binario appena copiato
ENTRYPOINT ["/app/stl2glb"]
