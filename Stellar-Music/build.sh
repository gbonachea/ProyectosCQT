#!/bin/bash
# ─────────────────────────────────────────────────────────
# build.sh – Compila y ejecuta Hero Music (C++/Qt)
# ─────────────────────────────────────────────────────────
set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo -e "${YELLOW}Verificando dependencias...${NC}"

# ── Detect Qt version ────────────────────────────────────
QT_VERSION=""
if pkg-config --exists Qt6Widgets Qt6Multimedia 2>/dev/null; then
    QT_VERSION=6
    echo -e "${GREEN}Qt6 detectado${NC}"
elif pkg-config --exists Qt5Widgets Qt5Multimedia 2>/dev/null; then
    QT_VERSION=5
    echo -e "${GREEN}Qt5 detectado${NC}"
else
    echo -e "${RED}No se encontró Qt5 ni Qt6. Instalando Qt6...${NC}"
    sudo apt-get update
    sudo apt-get install -y \
        qt6-base-dev \
        qt6-multimedia-dev \
        cmake \
        build-essential
    QT_VERSION=6
fi

# ── cmake ────────────────────────────────────────────────
if ! command -v cmake &>/dev/null; then
    echo -e "${YELLOW}Instalando cmake...${NC}"
    sudo apt-get install -y cmake build-essential
fi

# ── Build ────────────────────────────────────────────────
echo -e "${YELLOW}Compilando...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"

# Copy assets next to the binary
cp "$SCRIPT_DIR/dark_theme.css" "$BUILD_DIR/" 2>/dev/null || true
cp "$SCRIPT_DIR/setting.json"   "$BUILD_DIR/" 2>/dev/null || true
[ -d "$SCRIPT_DIR/icons" ] && cp -r "$SCRIPT_DIR/icons" "$BUILD_DIR/" 2>/dev/null || true

echo -e "${GREEN}Compilación exitosa. Iniciando Hero Music...${NC}"
"$BUILD_DIR/HeroMusic"
