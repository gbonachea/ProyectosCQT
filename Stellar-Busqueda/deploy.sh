#!/bin/bash
# Empaqueta Stellar Busqueda con sus dependencias Qt5
# Uso: ./deploy.sh

set -e

APP="stellarbusqueda"
BUILD_DIR="build"
DEST="dist/StellarBusqueda"

if [ ! -f "$BUILD_DIR/$APP" ]; then
    echo "Ejecuta primero: cmake --build build"
    exit 1
fi

rm -rf "$DEST"
mkdir -p "$DEST/lib" "$DEST/plugins/platforms" "$DEST/plugins/imageformats"

# Copiar ejecutable
cp "$BUILD_DIR/$APP" "$DEST/"

# Detectar Qt5
QT_DIR=$(dirname $(find /usr/lib -name "libQt5Core.so*" 2>/dev/null | head -1) 2>/dev/null)
if [ -z "$QT_DIR" ]; then
    QT_DIR=$(dirname $(dpkg -L libqt5core5a 2>/dev/null | grep libQt5Core.so | head -1) 2>/dev/null)
fi
if [ -z "$QT_DIR" ]; then
    echo "No se encuentra Qt5. Instala: sudo apt install qtbase5-dev libqt5widgets5"
    exit 1
fi

echo "Qt detectado en: $QT_DIR"
QT_BASE=$(realpath "$QT_DIR/..")

# Librerías Qt principales
for lib in libQt5Core.so.5 libQt5Gui.so.5 libQt5Widgets.so.5; do
    find "$QT_DIR" -maxdepth 1 -name "$lib*" -exec cp -aL {} "$DEST/lib/" \;
done

# Dependencias necesarias para Qt (excluyendo libc, libstdc++, ld-linux, libm, libpthread, libdl, librt)
for dep in libicudata.so.* libicui18n.so.* libicuuc.so.* libpcre2-16.so.* \
           libpng16.so.* libz.so.* libharfbuzz.so.* libmd4c.so.* \
           libfreetype.so.* libglib-2.0.so.* libgraphite2.so.* \
           libdouble-conversion.so.* libbrotli*.so.* libzstd.so.* \
           libpcre.so.*; do
    find /usr/lib -name "$dep" -exec cp -aL {} "$DEST/lib/" \; 2>/dev/null || true
done

# Plugins Qt
PLUGINS_DIR="$QT_BASE/plugins"
if [ -d "$PLUGINS_DIR" ]; then
    cp -aL "$PLUGINS_DIR/platforms/libqxcb.so" "$DEST/plugins/platforms/" 2>/dev/null || true
    cp -aL "$PLUGINS_DIR/imageformats/libqjpeg.so" "$DEST/plugins/imageformats/" 2>/dev/null || true
    cp -aL "$PLUGINS_DIR/imageformats/libqpng.so" "$DEST/plugins/imageformats/" 2>/dev/null || true
fi

# Resolver dependencias de .so empaquetados
for so in $(find "$DEST" -name "*.so*"); do
    for dep in $(readelf -d "$so" 2>/dev/null | grep NEEDED | grep -oP '\[\K[^\]]+'); do
        # Solo copiar si no es del sistema
        case "$dep" in
            libc.so*|libstdc++*|libm.so*|libpthread*|libdl*|librt*|ld-linux*|libgcc_s*) continue ;;
        esac
        if [ ! -f "$DEST/lib/$dep" ]; then
            find /usr/lib -name "$dep" -exec cp -aL {} "$DEST/lib/" \; 2>/dev/null || true
        fi
    done
done

# Lanzador
cat > "$DEST/run.sh" << 'EOF'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$DIR/plugins"
exec "$DIR/stellarbusqueda" "$@"
EOF
chmod +x "$DEST/run.sh"

echo "✅ Empaquetado en: $DEST"
echo "   Ejecuta: $DEST/run.sh"
echo "   Tamaño: $(du -sh $DEST | cut -f1)"
