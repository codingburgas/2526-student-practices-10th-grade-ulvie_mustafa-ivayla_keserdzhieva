#!/bin/bash
# Build script for Cinema Presentation with Slint on Linux/macOS
# This script builds the project using CMake

set -e  # Exit on error

echo "========================================"
echo "Cinema Presentation Build Script"
echo "========================================"
echo ""

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed"
    echo "Please install CMake from https://cmake.org/download/"
    exit 1
fi

# Check if Git is installed
if ! command -v git &> /dev/null; then
    echo "Error: Git is not installed"
    echo "Please install Git from https://git-scm.com"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

echo "[1/3] Running CMake configuration..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

echo ""
echo "[2/3] Building the project..."
cmake --build . --config Debug

echo ""
echo "[3/3] Build completed successfully!"
echo ""
echo "Executable location: ./Debug/cinema_presentation (or ./cinema_presentation on Linux)"
echo ""

cd ..

# Ask if user wants to run the application
read -p "Do you want to run the application now? (y/n): " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Running application..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
        ./build/Debug/cinema_presentation
    else
        ./build/cinema_presentation
    fi
fi

echo ""
echo "Build script completed!"
