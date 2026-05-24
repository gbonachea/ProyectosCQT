# Stellar-VPN (C++/Qt6)

Cliente VPN gráfico para Linux, convertido de Python/PyQt5 a **C++/Qt6**.

## Requisitos

- Qt 6.x (`qt6-base-dev`)
- CMake ≥ 3.16  **o**  qmake6 (incluido con Qt6)
- OpenVPN
- Compilador C++17 (GCC ≥ 10 / Clang ≥ 11)

## Estructura de archivos esperada

```
StellarVPN/
├── main.cpp
├── vpn_client.cpp
├── vpn_client.h
├── CMakeLists.txt
├── StellarVPN.pro      ← alternativa qmake
├── run.sh
├── settings.json
├── icons/
│   ├── icon.png
│   ├── icons.png
│   └── menu.png
└── ovpn/
    └── *.ovpn          ← tus archivos de servidor
```

## Compilar y ejecutar

### Opción A – Script automático
```bash
chmod +x run.sh
./run.sh
```

### Opción B – CMake manual
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..
./build/StellarVPN
```

### Opción C – qmake
```bash
qmake StellarVPN.pro
make -j$(nproc)
./StellarVPN
```

## Características

| Función                        | Estado |
|-------------------------------|--------|
| Selector de servidor (.ovpn)  | ✅ |
| Conexión con autenticación sudo | ✅ |
| Desconexión VPN               | ✅ |
| Velocidad simulada en tiempo real | ✅ |
| IP / Puerto en pantalla       | ✅ |
| Icono en bandeja del sistema  | ✅ |
| Minimizar a bandeja           | ✅ |
| Ventana de ajustes (3 tabs)   | ✅ |
| Agregar / eliminar servidores | ✅ |
| Guardar ajustes en JSON       | ✅ |
| Kill Switch (UI)              | ✅ |
| Auto-start / Auto-connect (UI)| ✅ |

## Créditos
Desarrollado por B&R.Comp — StellarVPN v1.0
