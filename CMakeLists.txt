cmake_minimum_required(VERSION 3.16)
project(FPS_SYSTEM)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED raylib)
find_package(GTest REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIRS}
)

# Main game executable
add_executable(FPS_SYSTEM 
    main.cpp
    include/ecs/registry.h
    include/ecs/systems.h
    include/ecs/entity_utils.h
    include/world/room.h
    include/world/hallway.h
    include/world/anchor.h
    include/world/doorway.h
    include/render/draw_utils.h
    src/render/draw_utils.cpp
    include/textures/managed_texture.h
)

target_link_libraries(FPS_SYSTEM ${RAYLIB_LIBRARIES})

# Test executable
add_executable(ecs_tests 
    tests/test_ecs.cpp
    include/ecs/registry.h
    include/ecs/systems.h
    include/ecs/entity_utils.h
    include/world/room.h
    include/world/hallway.h
    include/world/anchor.h
    include/world/doorway.h
    include/render/draw_utils.h
    src/render/draw_utils.cpp
    include/textures/managed_texture.h
)

target_link_libraries(ecs_tests 
    GTest::gtest_main
    GTest::gtest
    ${RAYLIB_LIBRARIES}
    pthread
)

enable_testing()
add_test(NAME ECS_Tests COMMAND ecs_tests)