FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    libcurl4-openssl-dev \
    libssl-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN git clone https://github.com/yhirose/cpp-httplib.git external/cpp-httplib && \
    git clone https://github.com/syoyo/tinygltf.git external/tinygltf && \
    git clone https://github.com/minio/minio-cpp.git external/minio-cpp && \
    git clone https://github.com/nlohmann/json.git external/json

RUN cmake -Bbuild -DCMAKE_BUILD_TYPE=Release . && cmake --build build --target stl2glb_server -j$(nproc)

CMD ["./build/stl2glb_server"]
