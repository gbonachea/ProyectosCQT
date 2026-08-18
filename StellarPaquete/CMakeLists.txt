cmake_minimum_required(VERSION 3.16)

project(paquete VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/packagebuilder.cpp
    src/debbuilder.cpp
    src/rpmbuilder.cpp
    src/archbuilder.cpp
    src/desktopbuilder.cpp
)

set(HEADERS
    src/mainwindow.h
    src/packagemetadata.h
    src/packagebuilder.h
    src/debbuilder.h
    src/rpmbuilder.h
    src/archbuilder.h
    src/desktopbuilder.h
)

set(RESOURCES
    resources.qrc
)

qt_add_executable(StellarPaquete
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_include_directories(StellarPaquete PRIVATE src)

target_link_libraries(StellarPaquete PRIVATE Qt6::Widgets)
