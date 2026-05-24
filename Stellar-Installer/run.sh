#!/bin/bash

# Si no se ejecuta en un terminal, abrir uno
if [ ! -t 0 ]; then
    x-terminal-emulator -e bash -c "cd \"$(dirname \"$0\")\"; ./run.sh"
    exit
fi

# Script para instalar las dependencias necesarias para ejecutar el instalador Qt
# Este script instala las bibliotecas runtime de Qt6 requeridas

echo "=== Verificando dependencias del instalador ==="

# Verificar si qt6-base está instalado
if dpkg -l | grep -q "^ii.*qt6-base"; then
    echo "Qt6 base ya está instalado. Saltando instalación."
else
    echo "Instalando dependencias..."
    # Actualizar el sistema
    sudo apt update
    # Instalar Qt6 base runtime
    sudo apt install -y qt6-base
    echo "=== Instalación completada ==="
fi

echo "Ejecutando el instalador..."
# Ejecutar el instalador
./instalador
echo "Ejecutando el instalador..."

# Ejecutar el instalador
./build/instalador