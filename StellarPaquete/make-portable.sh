#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
PORTABLE_DIR="$SCRIPT_DIR/StellarPaquete-Portable"
BINARY="$BUILD_DIR/StellarPaquete"
LIBDIR="$PORTABLE_DIR/lib"
PLUGINS_DIR="$PORTABLE_DIR/plugins"

echo "=== 1/6 Compilando Stellar Paquete ==="
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" >/dev/null
cmake --build "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 4)"

[ -x "$BINARY" ] || { echo "Error: no se pudo compilar el binario." >&2; exit 1; }

echo "=== 2/6 Localizando librerías y plugins de Qt ==="
QMAKE="$(command -v qmake6 || command -v qmake || true)"
if [ -z "$QMAKE" ]; then
    echo "Error: no se encontró qmake (qmake6/qmake)." >&2
    exit 1
fi
QT_PLUGINS="$("$QMAKE" -query QT_INSTALL_PLUGINS)"
QT_LIBS="$("$QMAKE" -query QT_INSTALL_LIBS)"

echo "=== 3/6 Preparando la carpeta portable ==="
rm -rf "$PORTABLE_DIR"
mkdir -p "$LIBDIR"
mkdir -p "$PLUGINS_DIR"

cp -L "$BINARY" "$PORTABLE_DIR/StellarPaquete"
chmod +x "$PORTABLE_DIR/StellarPaquete"

exclude_lib() {
    case "$1" in
        linux-vdso.so.*|libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|libresolv.so.*|libutil.so.*|libanl.so.*|libBrokenLocale.so.*|libthread_db.so.*|ld-linux*.so.*)
            return 0 ;;
    esac
    return 1
}

collect_deps() {
    local f="$1"
    while read -r path; do
        [ -n "$path" ] || continue
        local base; base="$(basename "$path")"
        if exclude_lib "$base"; then continue; fi
        if [ ! -e "$LIBDIR/$base" ]; then
            if cp -L "$path" "$LIBDIR/$base" 2>/dev/null; then
                chmod +x "$LIBDIR/$base" 2>/dev/null || true
                echo "  + $base"
            fi
        fi
    done < <(ldd "$f" 2>/dev/null | awk '/=>/ {print $3}')
}

echo "=== 4/6 Copiando librerías de dependencias ==="
collect_deps "$BINARY"
for pass in 1 2 3 4 5 6; do
    for so in "$LIBDIR"/*.so*; do
        [ -e "$so" ] || continue
        collect_deps "$so"
    done
done

echo "=== 5/6 Copiando plugins de Qt ==="
for sub in platforms platforminputcontexts imageformats iconengines styles xcbglintegrations \
           generic tls accessible wayland-decoration-client wayland-graphics-integration-client \
           wayland-shell-integration egldeviceintegrations; do
    if [ -d "$QT_PLUGINS/$sub" ]; then
        mkdir -p "$PLUGINS_DIR/$sub"
        cp -L "$QT_PLUGINS/$sub/"*.so "$PLUGINS_DIR/$sub/" 2>/dev/null || true
    fi
done
for so in "$PLUGINS_DIR"/*/*.so*; do
    [ -e "$so" ] || continue
    collect_deps "$so"
done

echo "=== 6/6 Escribiendo qt.conf, lanzador y accesorios ==="
cat > "$PORTABLE_DIR/qt.conf" <<'EOF'
[Paths]
Prefix = .
Libraries = lib
Plugins = plugins
EOF

cat > "$PORTABLE_DIR/StellarPaquete.sh" <<'EOF'
#!/bin/sh
HERE=$(dirname "$(readlink -f "$0")")
export LD_LIBRARY_PATH="$HERE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$HERE/StellarPaquete" "$@"
EOF
chmod +x "$PORTABLE_DIR/StellarPaquete.sh"

if command -v patchelf >/dev/null 2>&1; then
    patchelf --set-rpath '$ORIGIN/lib' "$PORTABLE_DIR/StellarPaquete" || true
fi

cp -f "$SCRIPT_DIR/icons/stellarpaquete.png" "$PORTABLE_DIR/icono.png" 2>/dev/null || true

echo
echo "=== Portable generado en: $PORTABLE_DIR ==="
echo "Ejecútalo con: $PORTABLE_DIR/StellarPaquete.sh"
du -sh "$PORTABLE_DIR"
