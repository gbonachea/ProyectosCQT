#!/usr/bin/env bash
set -e

echo "=== Instalando herramientas para generar paquetes (.deb, .rpm, .AppImage) ==="

if [ "$(id -u)" -ne 0 ]; then
    echo "Este script debe ejecutarse como root (usa: sudo bash install-dependencies.sh)"
    exit 1
fi

DISTRO_ID=$(grep -E '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"')
DISTRO_LIKE=$(grep -E '^ID_LIKE=' /etc/os-release | cut -d= -f2 | tr -d '"' || true)

is() {
    case "$DISTRO_ID $DISTRO_LIKE" in
        *"$1"*) return 0 ;;
    esac
    return 1
}

has() {
    command -v "$1" >/dev/null 2>&1
}

install_pkg() {
    if has apt-get; then
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -y
        apt-get install -y "$@"
    elif has dnf; then
        dnf install -y "$@"
    elif has yum; then
        yum install -y "$@"
    elif has pacman; then
        pacman -Sy --noconfirm --needed "$@"
    elif has zypper; then
        zypper --non-interactive install "$@"
    else
        echo "AVISO: no se reconoce el gestor de paquetes. Instala manualmente: $*"
        return 1
    fi
}

download() {
    local url="$1"
    local out="$2"
    if has curl; then
        curl -fL -o "$out" "$url"
    elif has wget; then
        wget -q -O "$out" "$url"
    else
        echo "Se necesita curl o wget para las descargas."
        return 1
    fi
}

install_appimage_tools() {
    local tmp=/tmp/paquete-appimage
    rm -rf "$tmp" && mkdir -p "$tmp" && cd "$tmp" || return 1

    if ! has appimagetool; then
        echo "Descargando appimagetool..."
        download "https://github.com/AppImage/appimagetool/releases/latest/download/appimagetool-x86_64.AppImage" appimagetool.AppImage || return 1
        chmod +x appimagetool.AppImage
        if ./appimagetool.AppImage --appimage-extract >/dev/null 2>&1; then
            install -m 0755 squashfs-root/appimagetool /usr/local/bin/appimagetool
            rm -rf squashfs-root
        else
            install -m 0755 appimagetool.AppImage /usr/local/bin/appimagetool
        fi
        rm -f appimagetool.AppImage
        echo "appimagetool instalado."
    else
        echo "appimagetool ya está instalado."
    fi

    if ! has linuxdeploy; then
        echo "Descargando linuxdeploy..."
        download "https://github.com/linuxdeploy/linuxdeploy/releases/latest/download/linuxdeploy-x86_64.AppImage" linuxdeploy.AppImage || return 1
        chmod +x linuxdeploy.AppImage
        if ./linuxdeploy.AppImage --appimage-extract >/dev/null 2>&1; then
            install -m 0755 squashfs-root/AppRun /usr/local/bin/linuxdeploy
            rm -rf squashfs-root
        else
            install -m 0755 linuxdeploy.AppImage /usr/local/bin/linuxdeploy
        fi
        rm -f linuxdeploy.AppImage
        echo "linuxdeploy instalado."
    else
        echo "linuxdeploy ya está instalado."
    fi

    if ! has linuxdeploy-plugin-appimage; then
        echo "Descargando linuxdeploy-plugin-appimage..."
        download "https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/latest/download/linuxdeploy-plugin-appimage-x86_64.AppImage" plugin.AppImage || return 1
        chmod +x plugin.AppImage
        if ./plugin.AppImage --appimage-extract >/dev/null 2>&1; then
            install -m 0755 squashfs-root/AppRun /usr/local/bin/linuxdeploy-plugin-appimage
            rm -rf squashfs-root
        else
            install -m 0755 plugin.AppImage /usr/local/bin/linuxdeploy-plugin-appimage
        fi
        rm -f plugin.AppImage
        echo "linuxdeploy-plugin-appimage instalado."
    else
        echo "linuxdeploy-plugin-appimage ya está instalado."
    fi

    rm -rf "$tmp"
}

echo "--- dpkg-deb (.deb) ---"
if ! has dpkg-deb; then
    install_pkg dpkg-dev || install_pkg dpkg
else
    echo "dpkg-deb ya está instalado."
fi

echo "--- rpmbuild (.rpm) ---"
if ! has rpmbuild; then
    install_pkg rpm-build || install_pkg rpm || install_pkg rpm-tools
else
    echo "rpmbuild ya está instalado."
fi

echo "--- Herramientas AppImage ---"
install_appimage_tools || echo "AVISO: la instalación de las herramientas AppImage no se completó."

echo
echo "=== Verificación final ==="
for t in dpkg-deb rpmbuild appimagetool linuxdeploy; do
    if has "$t"; then
        echo "OK    $t"
    else
        echo "FALTA $t"
    fi
done
