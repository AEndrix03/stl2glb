FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    git \
    build-essential cmake ninja-build curl unzip pkg-config wget \
    ca-certificates python3 python3-pip

WORKDIR /project

# Clona le repo sotto external solo se non esistono
RUN mkdir -p external && \
    [ -d external/json ] || git clone https://github.com/nlohmann/json.git external/json && \
    [ -d external/cpp-httplib ] || git clone https://github.com/yhirose/cpp-httplib.git external/cpp-httplib && \
    [ -d external/tinygltf ] || git clone https://github.com/syoyo/tinygltf.git external/tinygltf && \
    [ -d external/minio-cpp ] || git clone https://github.com/minio/minio-cpp.git external/minio-cpp

# Bootstrap vcpkg se non presente
RUN if [ ! -d external/vcpkg ]; then \
      git clone https://github.com/microsoft/vcpkg.git external/vcpkg && \
      external/vcpkg/bootstrap-vcpkg.sh; \
    else echo "vcpkg already present"; fi

# Installa pacchetti vcpkg se non presenti
RUN if [ ! -d external/vcpkg/installed/x64-linux ]; then \
      external/vcpkg/vcpkg install openssl tinygltf minio-cpp; \
    else echo "vcpkg packages already installed"; fi

# Copia il resto del progetto
COPY . /project

ENV STL2GLB_PORT=9000
ENV STL2GLB_STL_BUCKET_NAME=printer-model
ENV STL2GLB_GLB_BUCKET_NAME=printer-glb-model
ENV STL2GLB_MINIO_ENDPOINT=minio.aredegalli.it:9000
ENV STL2GLB_MINIO_ACCESS_KEY=admin
ENV STL2GLB_MINIO_SECRET_KEY=k9ef8g74IlGXiTlO114vKU79fOAy6aPU

EXPOSE ${STL2GLB_PORT}

WORKDIR /project/build

# Build
RUN mkdir -p /project/build && cd /project/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/project/external/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja && \
    cmake --build .

CMD ["./stl2glb"]
