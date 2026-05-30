cmake_minimum_required(VERSION 3.16)
project(dandelions_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dandelions_core
    src/geometry.cpp
    src/state.cpp
    src/canonical.cpp
    src/stats.cpp
    src/solver.cpp
    src/memo_table.cpp
    src/memo_file.cpp
    src/oracle.cpp
)

target_include_directories(dandelions_core PUBLIC src)

add_executable(dandelions_solver src/main.cpp)
target_link_libraries(dandelions_solver PRIVATE dandelions_core)

add_executable(dandelions_tests
    tests/test_geometry.cpp
    tests/test_rules.cpp
    tests/test_solver_small.cpp
)
target_link_libraries(dandelions_tests PRIVATE dandelions_core)

enable_testing()
add_test(NAME dandelions_tests COMMAND dandelions_tests)

add_executable(dandelions_play src/play_main.cpp)
target_link_libraries(dandelions_play PRIVATE dandelions_core)
