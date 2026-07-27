cmake_minimum_required(VERSION 3.15)
project(clash-cli CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threads library (required for background spinner std::thread)
find_package(Threads REQUIRED)

# Include directories
include_directories(
    include
    third_party
)

# Collect source files
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/clash-cli/*.hpp")

# Define target executable
add_executable(clash-cli ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(clash-cli PRIVATE Threads::Threads)
