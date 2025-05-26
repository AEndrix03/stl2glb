FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential git cmake curl unzip pkg-config wget \
    ca-certificates python3 python3-pip ninja-build

WORKDIR /project

# Clona solo vcpkg se mancante
RUN if [ ! -d /opt/vcpkg ]; then \
      git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg && \
      /opt/vcpkg/bootstrap-vcpkg.sh; \
    else echo "vcpkg already present"; fi

# Setta variabili ambiente per vcpkg
ENV VCPKG_ROOT=/opt/vcpkg
ENV CMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

COPY . /project

WORKDIR /project/build

RUN mkdir -p /project/build

# Qui cmake e build, ma i pacchetti li installiamo tramite volume in compose
RUN cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE -G Ninja && \
    cmake --build .

CMD ["./stl2glb_exec"]
