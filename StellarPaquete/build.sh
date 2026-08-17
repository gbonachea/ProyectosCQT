#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "=== Configurando con CMake ==="
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR"

echo "=== Compilando Stellar Paquete ==="
cmake --build "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 4)"

BINARY="${BUILD_DIR}/StellarPaquete"
if [ -x "$BINARY" ]; then
    echo
    echo "Binario generado: $BINARY"
    echo "Ejecútalo con:    $BINARY"
else
    echo "Error: no se encontró el binario $BINARY" >&2
    exit 1
fi
