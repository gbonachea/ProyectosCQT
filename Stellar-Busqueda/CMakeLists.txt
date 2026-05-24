cmake_minimum_required(VERSION 3.16)
project(StellarBusqueda VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/filesearcher.cpp
    src/queryparser.cpp
    src/previewwidget.cpp
)

set(HEADERS
    src/searchresult.h
    src/mainwindow.h
    src/filesearcher.h
    src/queryparser.h
    src/previewwidget.h
)

set(RESOURCES
    resources.qrc
)

add_executable(stellarbusqueda ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(stellarbusqueda PRIVATE
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
)

target_include_directories(stellarbusqueda PRIVATE ${CMAKE_SOURCE_DIR}/src)

set_target_properties(stellarbusqueda PROPERTIES
    INSTALL_RPATH "$ORIGIN/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)
