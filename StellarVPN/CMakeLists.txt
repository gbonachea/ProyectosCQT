cmake_minimum_required(VERSION 3.16)
project(StellarVPN VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(StellarVPN
    main.cpp
    vpn_client.cpp
    vpn_client.h
)

target_link_libraries(StellarVPN
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

# Copy ovpn and icons directories to build directory
add_custom_command(TARGET StellarVPN POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/ovpn
            ${CMAKE_BINARY_DIR}/ovpn
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/icons
            ${CMAKE_BINARY_DIR}/icons
    COMMENT "Copying ovpn and icons directories to build directory"
)

install(TARGETS StellarVPN
    RUNTIME DESTINATION bin
)

# Install ovpn and icons directories
install(DIRECTORY ${CMAKE_SOURCE_DIR}/ovpn
    DESTINATION .
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/icons
    DESTINATION .
)
