FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    cmake \
    ninja-build \
    curl \
    unzip \
    pkg-config \
    libssl-dev \
    ca-certificates \
    python3 \
    python3-pip \
    wget

WORKDIR /project

# Clona vcpkg se manca e bootstrap
RUN if [ ! -d external/vcpkg ]; then \
      git clone https://github.com/microsoft/vcpkg.git external/vcpkg && \
      external/vcpkg/bootstrap-vcpkg.sh; \
    else \
      echo "vcpkg already present"; \
    fi

ENV VCPKG_ROOT=external/vcpkg
ENV CMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake

COPY . /project

RUN mkdir -p /project/build

WORKDIR /project/build

RUN cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE -G Ninja
RUN cmake --build .

CMD ["./stl2glb_exec"]
