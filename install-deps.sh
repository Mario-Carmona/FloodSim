#!/bin/bash

# =================================================================
# Script: install-deps.sh
# Description: Installs system dependencies for vcpkg and project 
#              compilation on Ubuntu/Debian systems.
# Usage: ./install-deps.sh [--server | --desktop]
# =================================================================

# Exit immediately if a command exits with a non-zero status
set -e

# Disable interactive prompts during installation
export DEBIAN_FRONTEND=noninteractive

# Function to display usage instructions
usage() {
    echo "Usage: $0 [--server | --desktop]"
    echo "  --server  : Install base dependencies only"
    echo "  --desktop : Install base + graphical dependencies"
    exit 1
}

# Check for correct number of arguments
if [ "$#" -ne 1 ]; then usage; fi

echo "STATUS: Starting system dependencies installation..."

# ---------------------------------------------------------
# PHASE 1: Install CMake >= 4.1.1 (Custom Installation)
# ---------------------------------------------------------
REQUIRED_CMAKE_VER="4.1.1"

# Safely check if cmake exists on the system
if ! command -v cmake &> /dev/null; then
    CURRENT_CMAKE_VER="0"
else
    # If it exists, extract the version without risk of errors
    CURRENT_CMAKE_VER=$(cmake --version | head -n1 | cut -d' ' -f3)
fi

echo "STATUS: Current CMake version: $CURRENT_CMAKE_VER"

# Compare versions using sort -V (Natural Version Sort)
# If the current version is older than required, the required version will be the last in the sorted list
if [ "$(printf '%s\n%s' "$REQUIRED_CMAKE_VER" "$CURRENT_CMAKE_VER" | sort -V | head -n1)" != "$REQUIRED_CMAKE_VER" ]; then
    echo "STATUS: CMake version is lower than $REQUIRED_CMAKE_VER. Installing CMake 4.1.1..."
    
    CMAKE_SCRIPT="cmake-4.1.1-linux-x86_64.sh"
    curl -L -O https://github.com/Kitware/CMake/releases/download/v4.1.1/$CMAKE_SCRIPT
    
    sudo chmod +x $CMAKE_SCRIPT
    sudo mkdir -p /opt/cmake
    # --skip-license: Accept license automatically
    # --prefix: Installation directory
    sudo ./$CMAKE_SCRIPT --skip-license --prefix=/opt/cmake
    
    # Create symbolic links to prioritize this version in the PATH
    sudo ln -sf /opt/cmake/bin/cmake /usr/local/bin/cmake
    sudo ln -sf /opt/cmake/bin/ctest /usr/local/bin/ctest
    sudo ln -sf /opt/cmake/bin/cpack /usr/local/bin/cpack
    
    rm $CMAKE_SCRIPT
    echo "STATUS: CMake 4.1.1 installed successfully at /opt/cmake"
else
    echo "STATUS: CMake $CURRENT_CMAKE_VER is already installed and meets the requirement (>= $REQUIRED_CMAKE_VER)."
fi

# ---------------------------------------------------------
# PHASE 2: Essential Build Tools
# ---------------------------------------------------------
CORE_DEPS="build-essential ninja-build tar curl zip unzip g++-12 gcc-12"

# ---------------------------------------------------------
# PHASE 3: Core Library Dependencies (Always required)
# ---------------------------------------------------------
# linux-libc-dev -> Required for: openssl
CORE_DEPS="$CORE_DEPS linux-libc-dev"

# ---------------------------------------------------------
# PHASE 4: Graphical & UI Dependencies (Optional)
# ---------------------------------------------------------
GUI_DEPS=""
if [ "$1" == "--desktop" ]; then
    echo "STATUS: Configuring Graphical support..."
    # libxinerama-dev -> Required for: glfw3 
    # libxcursor-dev -> Required for: glfw3 
    # xorg-dev -> Required for: glfw3 
    # libglu1-mesa-dev -> Required for: glfw3 
    # pkg-config -> Required for: glfw3
    GUI_DEPS="libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev pkg-config"
elif [ "$1" == "--server" ]; then
    echo "STATUS: Configuring Server-only..."
else
    usage
fi

# ---------------------------------------------------------
# PHASE 5: Execution
# ---------------------------------------------------------
echo "STATUS: Updating package lists..."
sudo -E apt-get update

echo "STATUS: Installing identified packages..."
sudo -E apt-get install -y $CORE_DEPS $GUI_DEPS

echo "STATUS: All system dependencies installed successfully."