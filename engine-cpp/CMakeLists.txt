cmake_minimum_required(VERSION 3.10)
project(SchedulrX_Engine VERSION 1.0)

# Enforce C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the headers and third-party libraries (like nlohmann/json)
include_directories(include third_party)

# Grab all .cpp files in the src/ directory
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Build the executable
add_executable(engine_main ${SOURCES})