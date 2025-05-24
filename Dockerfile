FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    curl \
    libssl-dev \
    libcurl4-openssl-dev \
    libcurlpp-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN rm -rf external/cpp-httplib && git clone https://github.com/yhirose/cpp-httplib.git external/cpp-httplib && \
    rm -rf external/tinygltf && git clone https://github.com/syoyo/tinygltf.git external/tinygltf && \
    rm -rf external/minio-cpp && git clone https://github.com/minio/minio-cpp.git external/minio-cpp && \
    rm -rf external/json && git clone https://github.com/nlohmann/json.git external/json

RUN ls -lh external/tinygltf && head -n 5 external/tinygltf/tiny_gltf.h

RUN cmake -Bbuild -DCMAKE_BUILD_TYPE=Release . && \
    cmake --build build --target stl2glb_server -j$(nproc)

CMD ["./build/stl2glb_server"]
