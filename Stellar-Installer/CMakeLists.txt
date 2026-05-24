cmake_minimum_required(VERSION 3.16)
project(instalador LANGUAGES CXX)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

# enable automoc/autorcc/auotouic, so we don't need to include installer.moc manually
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(instalador installer.cpp)

target_link_libraries(instalador PRIVATE Qt6::Widgets)
set_target_properties(instalador PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
)
