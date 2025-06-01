# STL2GLB Build Instructions

## Prerequisiti

### Windows
- Visual Studio 2022 con C++ workload
- CMake 3.16+
- VCPKG installato in `C:\vcpkg` o `C:\tools\vcpkg`
- Git

### Linux
- GCC/Clang con supporto C++17
- CMake 3.16+
- Ninja (opzionale ma consigliato)
- VCPKG installato in `/home/vcpkg` o `$HOME/vcpkg`

## Installazione dipendenze con VCPKG

### Windows
```powershell
vcpkg install openssl:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install cpp-httplib:x64-windows
vcpkg install draco:x64-windows  # Opzionale
```

### Linux
```bash
vcpkg install openssl:x64-linux
vcpkg install nlohmann-json:x64-linux
vcpkg install cpp-httplib:x64-linux
vcpkg install draco:x64-linux  # Opzionale
```

## Build Locale

### Windows
```batch
# Usa lo script fornito
build-windows.bat

# O manualmente
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

### Linux
```bash
# Usa lo script fornito
chmod +x build-linux.sh
./build-linux.sh

# O manualmente
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/home/vcpkg/scripts/buildsystems/vcpkg.cmake -GNinja
cmake --build .
```

## Build con Docker

### Setup iniziale
1. Copia `.env.template` in `.env` e configura le variabili
2. Assicurati che VCPKG sia installato in `/home/vcpkg` sull'host

### Build e Run
```bash
# Build e avvia i servizi
docker-compose up -d

# Solo build
docker-compose build

# Visualizza i log
docker-compose logs -f app

# Stop
docker-compose down
```

### Profili Docker Compose

```bash
# Avvia con monitoring
docker-compose --profile monitoring up -d

# Solo app principale (default)
docker-compose up -d
```

## Configurazione

### Variabili ambiente richieste (.env)
- `STL2GLB_STL_BUCKET_NAME`: Nome bucket MinIO per file STL
- `STL2GLB_GLB_BUCKET_NAME`: Nome bucket MinIO per file GLB
- `STL2GLB_MINIO_ENDPOINT`: Endpoint MinIO (es: minio.example.com:9000)
- `STL2GLB_MINIO_ACCESS_KEY`: Access key MinIO
- `STL2GLB_MINIO_SECRET_KEY`: Secret key MinIO

### Ottimizzazioni per VPS con risorse limitate

Il docker-compose è configurato con:
- Limiti CPU: 0.5 cores
- Limiti RAM: 256MB
- Filesystem read-only con tmpfs per /tmp
- Logging compresso con rotazione
- Health check ottimizzati
- DNS cache locale

## Troubleshooting

### Windows
- Se VCPKG non viene trovato, imposta la variabile `VCPKG_ROOT`
- Per Visual Studio 2019, modifica il generator in CMake

### Linux
- Se mancano permessi per `/home/vcpkg`, usa `sudo` o cambia owner
- Per Alpine Linux nel container, alcune librerie potrebbero richiedere musl-dev

### Docker
- Se il volume VCPKG non monta, verifica il path in `docker-compose.yml`
- Per debug, usa il profilo monitoring: `docker-compose --profile monitoring up`

## Performance

L'eseguibile è ottimizzato per:
- Dimensioni minime (flag -Os, strip symbols)
- Basso utilizzo memoria (MALLOC_ARENA_MAX=1)
- Parsing STL multi-threaded per file grandi
- Memory-mapped I/O per efficienza
- Compressione Draco opzionale per GLB