cmake_minimum_required(VERSION 3.16)
project(StellarTexto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)

add_executable(StellarTexto
    main.cpp
    mainwindow.cpp
    codeeditor.cpp
    syntaxhighlighter.cpp
    finddialog.cpp
    resources.qrc
)

target_link_libraries(StellarTexto PRIVATE Qt6::Widgets Qt6::PrintSupport)
