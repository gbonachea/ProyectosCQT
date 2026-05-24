#!/bin/bash
# Script para compilar y ejecutar StellarVPN (C++/Qt5)
set -e
chmod +x "$0"

# ── Verificar dependencias ──────────────────────────────────────────────────
echo "[INFO] Verificando dependencias..."

if ! command -v cmake &>/dev/null; then
    echo "[INFO] Instalando cmake..."
    sudo apt update && sudo apt install -y cmake
fi

if ! dpkg -l | grep -q qt6-base-dev; then
    echo "[INFO] Instalando Qt6 dev..."
    sudo apt update && sudo apt install -y qt6-base-dev cmake g++
fi

if ! command -v openvpn &>/dev/null; then
    echo "[INFO] Instalando OpenVPN..."
    sudo apt update && sudo apt install -y openvpn
fi

# ── Compilar ────────────────────────────────────────────────────────────────
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "[INFO] Configurando con CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "[INFO] Compilando..."
make -j"$(nproc)"

cd ..

# ── Crear directorio ovpn si no existe ──────────────────────────────────────
mkdir -p ovpn

# ── Ejecutar ─────────────────────────────────────────────────────────────────
echo "[INFO] Iniciando StellarVPN..."
"$BUILD_DIR/StellarVPN"
