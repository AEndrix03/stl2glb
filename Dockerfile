# Stage 1: build
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y build-essential cmake ninja-build git curl unzip pkg-config

# Clona vcpkg e bootstrap
RUN git clone https://github.com/microsoft/vcpkg.git /external/vcpkg
RUN /external/vcpkg/bootstrap-vcpkg.sh

# Installa dipendenze vcpkg
RUN /external/vcpkg/vcpkg install openssl nlohmann-json cpp-httplib minio-cpp

WORKDIR /project
COPY . /project

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/external/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja && \
    cmake --build .

# Stage 2: runtime
FROM ubuntu:22.04

WORKDIR /project

COPY --from=builder /project/build/stl2glb_exec .

ENV STL2GLB_PORT=8080

EXPOSE ${STL2GLB_PORT}

CMD ["./stl2glb_exec"]
