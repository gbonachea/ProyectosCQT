# Hero Music – C++/Qt Port

Reproductor de Audio Ligero, reescrito en **C++17 + Qt** a partir del original Python/PyQt5.

## Características
- Reproductor con barra de progreso y control de posición (seek)
- Lista de reproducción separada con drag & drop, reordenar y eliminar
- Control de volumen desplegable
- Ecualizador de 10 bandas con presets (Plano, Rock, Pop, Jazz, Clásica)
- Configuración: inicio automático, minimizar a bandeja
- Tema oscuro cargado desde `dark_theme.css`
- Persistencia de posición/tamaño de ventanas en `setting.json`
- Icono en la bandeja del sistema
- Soporte MP3 y WAV

## Dependencias

| Librería | Paquete Debian/Ubuntu |
|---|---|
| Qt6 Widgets | `qt6-base-dev` |
| Qt6 Multimedia | `qt6-multimedia-dev` |
| CMake ≥ 3.16 | `cmake` |
| GCC/Clang | `build-essential` |

> **Qt5** también funciona. El `CMakeLists.txt` lo detecta automáticamente.

## Compilar y ejecutar

```bash
chmod +x build.sh
./build.sh
```

O manualmente:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
# Copiar assets
cp ../dark_theme.css ../setting.json .
./HeroMusic
```

## Estructura del proyecto

```
hero-music-cpp/
├── main.cpp              # Punto de entrada
├── audioplayer.h/.cpp    # Ventana principal del reproductor
├── playlistwindow.h/.cpp # Ventana de lista de reproducción
├── configwindow.h/.cpp   # Diálogo de configuración + ecualizador
├── settings.h/.cpp       # Carga/guarda setting.json y dark_theme.css
├── CMakeLists.txt        # Sistema de build
├── build.sh              # Script de compilación y ejecución
├── dark_theme.css        # Tema oscuro (igual que el original)
└── setting.json          # Estado inicial de ventanas
```

## Diferencias respecto al original Python

| Aspecto | Python | C++/Qt |
|---|---|---|
| Backend de audio | pygame.mixer | QMediaPlayer + QAudioOutput |
| Ecualización | pygame channels (básico) | Señal `equalizerChanged` (extensible) |
| Build | Intérprete | Binario nativo |
| Dependencias | pip | Sistema / CMake |
