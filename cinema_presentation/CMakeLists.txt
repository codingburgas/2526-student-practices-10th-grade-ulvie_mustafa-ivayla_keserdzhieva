cmake_minimum_required(VERSION 3.21)
project(cinema_presentation VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# Add Slint as a dependency
include(FetchContent)
FetchContent_Declare(Slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    GIT_TAG release/1.6
    SOURCE_SUBDIR internal/cmake
)
FetchContent_MakeAvailable(Slint)

# Define the executable
add_executable(cinema_presentation
    src/main.cpp
    src/UIStateManager.cpp
    src/CinemaUIController.cpp
)

# Compile the Slint file
slint_target_sources(cinema_presentation src/cinema.slint)

# Link against the Slint library
target_link_libraries(cinema_presentation PRIVATE Slint::Slint)

# Include directories from logic and business layers
target_include_directories(cinema_presentation PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../cinema_logic/include
    ${CMAKE_SOURCE_DIR}/../cinema_business/include
)
