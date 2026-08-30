#!/bin/bash
set -e

cd "$(dirname "$0")/.."

BUILD_DIR="build"
JOBS=$(nproc)

case "${1:-}" in
    clean)
        rm -rf "$BUILD_DIR"
        echo "Cleaned build directory."
        ;;
    rebuild)
        rm -rf "$BUILD_DIR"
        mkdir -p "$BUILD_DIR"
        cmake -S . -B "$BUILD_DIR"
        cmake --build "$BUILD_DIR" -j"$JOBS"
        ;;
    *)
        mkdir -p "$BUILD_DIR"
        cmake -S . -B "$BUILD_DIR"
        cmake --build "$BUILD_DIR" -j"$JOBS"
        ;;
esac
