#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
DESKTOP_FILE="$PROJECT_DIR/StellarTexto.desktop"
ICON_SRC="$PROJECT_DIR/icons/stellartexto.png"

APP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"

echo "==> Compilando..."
mkdir -p "$BUILD_DIR" && cmake -S "$PROJECT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"

echo "==> Instalando .desktop..."
mkdir -p "$APP_DIR" "$ICON_DIR"

sed "s|@BINDIR@|$BUILD_DIR|g" "$DESKTOP_FILE" > "$APP_DIR/StellarTexto.desktop"
chmod +x "$APP_DIR/StellarTexto.desktop"

echo "==> Instalando icono..."
cp "$ICON_SRC" "$ICON_DIR/stellartexto.png"

echo "==> Actualizando base de datos..."
update-desktop-database "$APP_DIR" 2>/dev/null || true
gtk-update-icon-cache "$HOME/.local/share/icons/hicolor" 2>/dev/null || true

echo ""
echo "Hecho. Ahora puedes:"
echo "  - Abrir Stellar Texto desde el menú"
echo "  - Asociar archivos de texto (clic derecho -> Abrir con -> Stellar Texto)"
echo "  - Ejecutar: $BUILD_DIR/StellarTexto"
