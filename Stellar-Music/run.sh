#!/bin/bash

################################################################################
# HeroMusic Player - Setup and Run Script
# This script automatically installs dependencies and runs the application
################################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC} $1"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
}

print_info() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Detect Linux distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$ID"
    else
        echo "unknown"
    fi
}

# Install dependencies based on distro
install_dependencies() {
    local distro=$(detect_distro)
    
    print_header "Installing Dependencies"
    
    case "$distro" in
        ubuntu|debian)
            print_info "Debian/Ubuntu detected"
            print_info "Updating package manager..."
            sudo apt-get update -qq
            
            # Check and install dependencies
            local packages="cmake build-essential git"
            packages="$packages qt6-base-dev qt6-multimedia-dev"
            
            print_info "Installing: $packages"
            sudo apt-get install -y $packages
            ;;
            
        fedora)
            print_info "Fedora detected"
            print_info "Updating package manager..."
            sudo dnf check-update -q || true
            
            local packages="cmake gcc-c++ git"
            packages="$packages qt6-qtbase-devel qt6-qtmultimedia-devel"
            
            print_info "Installing: $packages"
            sudo dnf install -y $packages
            ;;
            
        arch|manjaro)
            print_info "Arch/Manjaro detected"
            print_info "Updating package manager..."
            sudo pacman -Sy --noconfirm
            
            local packages="cmake base-devel git"
            packages="$packages qt6-base qt6-multimedia"
            
            print_info "Installing: $packages"
            sudo pacman -S --noconfirm $packages
            ;;
            
        *)
            print_error "Unsupported distribution: $distro"
            print_info "Please install these packages manually:"
            echo "  - cmake (>= 3.16)"
            echo "  - C++ compiler (g++ or clang)"
            echo "  - Qt6 development libraries (qt6-base-dev, qt6-multimedia-dev)"
            echo ""
            echo "Then run: mkdir build && cd build && cmake .. && make && ./HeroMusic"
            exit 1
            ;;
    esac
    
    print_info "Dependencies installed successfully!"
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check dependencies
check_dependencies() {
    print_header "Checking Dependencies"
    
    local missing=0
    
    # Check CMake
    if command_exists cmake; then
        local cmake_ver=$(cmake --version | head -n1 | grep -oP '\d+\.\d+' | head -1)
        print_info "CMake $cmake_ver found"
    else
        print_warn "CMake not found"
        missing=1
    fi
    
    # Check C++ compiler
    if command_exists g++; then
        local gcc_ver=$(g++ --version | head -n1 | grep -oP '\d+\.\d+' | head -1)
        print_info "G++ $gcc_ver found"
    elif command_exists clang++; then
        local clang_ver=$(clang++ --version | head -n1 | grep -oP '\d+\.\d+' | head -1)
        print_info "Clang++ $clang_ver found"
    else
        print_warn "C++ compiler not found"
        missing=1
    fi
    
    # Check Qt6
    if pkg-config --exists "Qt6Core Qt6Widgets Qt6Multimedia" 2>/dev/null; then
        local qt_ver=$(pkg-config --modversion Qt6Core)
        print_info "Qt6 $qt_ver found"
    elif pkg-config --exists "Qt5Core Qt5Widgets Qt5Multimedia" 2>/dev/null; then
        local qt_ver=$(pkg-config --modversion Qt5Core)
        print_warn "Qt6 not found, but Qt5 $qt_ver available (will use as fallback)"
    else
        print_warn "Qt6/Qt5 not found"
        missing=1
    fi
    
    if [ $missing -eq 1 ]; then
        print_info "Missing dependencies detected. Installing..."
        install_dependencies
    else
        print_info "All dependencies are installed!"
    fi
}

# Build the project
build_project() {
    print_header "Building HeroMusic"
    
    local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "$script_dir"
    
    # Create build directory
    if [ ! -d "build" ]; then
        print_info "Creating build directory..."
        mkdir -p build
    fi
    
    cd build
    
    # Run CMake
    print_info "Running CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    # Build with make/ninja
    if command_exists ninja; then
        print_info "Building with Ninja..."
        ninja -j$(nproc)
    else
        print_info "Building with Make (using $(nproc) jobs)..."
        make -j$(nproc)
    fi
    
    print_info "Build completed successfully!"
    
    # Copy resources (icons, CSS, etc.) to build directory
    print_info "Copying application resources..."
    if [ -d "$script_dir/icons" ]; then
        cp -r "$script_dir/icons" ./
        print_info "Icons directory copied"
    fi
    if [ -f "$script_dir/dark_theme.css" ]; then
        cp "$script_dir/dark_theme.css" ./
        print_info "Theme stylesheet copied"
    fi
    
    echo ""
}

# Run the application
run_application() {
    print_header "Running HeroMusic"
    
    local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local executable="$script_dir/build/HeroMusic"
    
    if [ ! -f "$executable" ]; then
        print_error "Executable not found: $executable"
        exit 1
    fi
    
    print_info "Launching HeroMusic..."
    echo ""
    
    # Run in background to avoid terminal block
    "$executable" &
    
    # Store the PID for cleanup
    local app_pid=$!
    
    # Wait a moment for the window to open
    sleep 1
    
    # Return to normal terminal control
    wait $app_pid 2>/dev/null || true
}

# Main script
main() {
    clear
    print_header "HeroMusic Player - Setup & Run"
    echo ""
    
    # Check and install dependencies
    check_dependencies
    
    echo ""
    
    # Build project
    build_project
    
    echo ""
    
    # Run application
    run_application
}

# Trap errors
trap 'print_error "Script failed!"; exit 1' ERR

# Run main function
main
