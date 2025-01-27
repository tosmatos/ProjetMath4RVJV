cmake_minimum_required(VERSION 3.12)
project(ProjetMath4RVJV VERSION 1.0)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vcpkg toolchain (important for finding packages)
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# Find required packages
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# Add executable, builds exe from src/main.cpp
add_executable(ProjetMath4RVJV 
    src/main.cpp
 "include/Vertex.h" "include/Polygon.hpp" "src/Polygon.cpp")

# Link libraries
target_link_libraries(ProjetMath4RVJV 
    PRIVATE 
    glfw 
    glad::glad
)

# Include directories (if needed)
target_include_directories(ProjetMath4RVJV 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

if(MINGW)
    target_compile_options(ProjetMath4RVJV PRIVATE 
        -Wall           # Enable all warnings
        -Wextra         # Enable extra warnings
        -fexceptions    # Enable exception handling
    )
endif()