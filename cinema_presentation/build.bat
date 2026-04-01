@echo off
REM Build script for Cinema Presentation with Slint on Windows
REM This script builds the project using CMake and Visual Studio

setlocal enabledelayedexpansion

REM Check if CMake is installed
where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: CMake is not installed or not in PATH
    echo Please install CMake from https://cmake.org/download/
    exit /b 1
)

REM Check if Git is installed
where git >nul 2>nul
if errorlevel 1 (
    echo Error: Git is not installed or not in PATH
    echo Please install Git from https://git-scm.com/download/win
    exit /b 1
)

echo ========================================
echo Cinema Presentation Build Script
echo ========================================
echo.

REM Create build directory
if not exist build mkdir build
cd build

echo [1/3] Running CMake configuration...
cmake .. -A x64
if errorlevel 1 (
    echo Error: CMake configuration failed
    cd ..
    exit /b 1
)

echo.
echo [2/3] Building the project...
cmake --build . --config Debug
if errorlevel 1 (
    echo Error: Build failed
    cd ..
    exit /b 1
)

echo.
echo [3/3] Build completed successfully!
echo.
echo Executable location: build\Debug\cinema_presentation.exe
echo.

cd ..

REM Ask if user wants to run the application
set /p run_app="Do you want to run the application now? (y/n): "
if /i "%run_app%"=="y" (
    echo Running application...
    .\build\Debug\cinema_presentation.exe
)

echo.
echo Build script completed!
pausе
