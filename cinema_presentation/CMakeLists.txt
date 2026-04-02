cmake_minimum_required(VERSION 3.21)
project(cinema_presentation VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Define the executable
add_executable(cinema_presentation
    src/main.cpp
)

# Link against Windows libraries
target_link_libraries(cinema_presentation PRIVATE user32 gdi32 msimg32)

# Set subsystem to WINDOWS for GUI app
add_definitions(-D_MBCS)
set_target_properties(cinema_presentation PROPERTIES
    WIN32_EXECUTABLE TRUE
)
