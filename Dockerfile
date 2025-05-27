FROM ubuntu:22.04

RUN apt-get update && apt-get install -y build-essential cmake git

COPY . /workdir
WORKDIR /workdir

RUN cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/root/vcpkg/scripts/buildsystems/vcpkg.cmake
RUN cmake --build build --config Release

CMD ["./build/stl2glb_exec"]
