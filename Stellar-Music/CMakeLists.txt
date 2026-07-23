cmake_minimum_required(VERSION 3.16)
project(HeroMusic VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Try Qt6 first, fall back to Qt5 ──────────────────────────────────────────
find_package(Qt6 QUIET COMPONENTS Core Widgets Multimedia)
if(Qt6_FOUND)
    message(STATUS "Using Qt6")
    set(QT_LIBS Qt6::Core Qt6::Widgets Qt6::Multimedia)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Multimedia)
    message(STATUS "Using Qt5")
    set(QT_LIBS Qt5::Core Qt5::Widgets Qt5::Multimedia)
endif()

set(SOURCES
    main.cpp
    audioplayer.cpp
    playlistwindow.cpp
    configwindow.cpp
    settings.cpp
)

set(HEADERS
    audioplayer.h
    playlistwindow.h
    configwindow.h
    settings.h
)

add_executable(HeroMusic ${SOURCES} ${HEADERS})
target_link_libraries(HeroMusic PRIVATE ${QT_LIBS})

# Install alongside the CSS and setting.json
install(TARGETS HeroMusic DESTINATION bin)
install(FILES dark_theme.css setting.json DESTINATION bin OPTIONAL)
install(DIRECTORY icons DESTINATION bin OPTIONAL)
