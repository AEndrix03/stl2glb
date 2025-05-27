# syntax=docker/dockerfile:1.4

FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y build-essential cmake git curl unzip

WORKDIR /workdir
COPY . .

# Monta il volume vcpkg durante la build
RUN --mount=type=bind,source=/opt/vcpkg,target=/root/vcpkg \
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/root/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    cmake --build build --config Release

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y libssl-dev ca-certificates

COPY --from=builder /workdir/build/stl2glb_exec /usr/local/bin/stl2glb_exec

CMD ["/usr/local/bin/stl2glb_exec"]
