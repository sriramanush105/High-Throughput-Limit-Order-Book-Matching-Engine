# CMakeLists.txt
# Build with:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j

cmake_minimum_required(VERSION 3.14)
project(matching_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(matching_engine
    src/OrderBook.cpp
    src/LobsterReplay.cpp
    src/Benchmark.cpp
    src/main.cpp
)
