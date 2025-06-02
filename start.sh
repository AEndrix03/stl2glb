#!/usr/bin/env bash
# /app/start.sh

if [ -f /build_output/stl2glb ]; then
    cp /build_output/stl2glb /app/stl2glb
    chmod +x /app/stl2glb
    exec /app/stl2glb
else
    echo "Error: stl2glb executable not found in /build_output"
    exit 1
fi
