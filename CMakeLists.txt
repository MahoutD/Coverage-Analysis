cmake_minimum_required(VERSION 3.20...3.30)
project(EmbeddedCoverageAnalysis VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt MOC, RCC, and UIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg SvgWidgets)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Subdirectories
add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(cli)

# Print build configuration summary
message(STATUS "--------------------------------------------------------")
message(STATUS " Embedded C Coverage & Static Analysis Suite v${PROJECT_VERSION}")
message(STATUS " Qt Version:      ${Qt6_VERSION}")
message(STATUS " C++ Standard:    C++${CMAKE_CXX_STANDARD}")
message(STATUS " Output Path:     ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "--------------------------------------------------------")
