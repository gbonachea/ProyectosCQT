#!/bin/bash
# StellarDock - Build and Install Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "============================================"
echo "  StellarDock - macOS-style Dock for Linux"
echo "============================================"
echo ""

echo "[1/4] Checking dependencies..."

# Qt6
if ! pkg-config --exists Qt6Core 2>/dev/null; then
    echo "  ERROR: Qt6 not found. Install with:"
    echo "    Ubuntu/Debian: sudo apt install qt6-base-dev qt6-base-private-dev libx11-dev cmake build-essential"
    echo "    Fedora:        sudo dnf install qt6-qtbase-devel qt6-qtbase-private-devel libX11-devel cmake"
    echo "    Arch:          sudo pacman -S qt6-base libx11 cmake base-devel"
    exit 1
fi

# Private headers check
if ! pkg-config --exists Qt6GuiPrivate 2>/dev/null; then
    echo "  WARNING: Qt6 private headers not found. Install qt6-base-private-dev"
    echo "    Ubuntu/Debian: sudo apt install qt6-base-private-dev"
fi

echo "  Dependencies OK!"

echo ""
echo "[2/4] Configuring..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release

echo ""
echo "[3/4] Building..."
make -j$(nproc)

echo ""
echo "[4/4] Installing..."
if [ "$EUID" -ne 0 ]; then
    sudo make install
else
    make install
fi

# Autostart
mkdir -p "$HOME/.config/autostart"
cp "$SCRIPT_DIR/stellardock.desktop" "$HOME/.config/autostart/"

echo ""
echo "============================================"
echo "  Done! Run: stellardock"
echo "  Autostart added to: ~/.config/autostart/"
echo "============================================"
