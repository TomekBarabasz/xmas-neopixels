cmake_minimum_required(VERSION 3.16.5)
set(CMAKE_CXX_COMPILER /usr/bin/clang-14)
set(CMAKE_CXX_STANDARD 20)

project(neopixel-test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTEST REQUIRED gtest>=1.8.1)
pkg_check_modules(GMOCK REQUIRED gmock>=1.8.1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ../include ..)
link_directories(${GTEST_LIBRARY_DIRS} ${GMOCK_LIBRARY_DIRS})
add_definitions(${GTEST_CFLAGS_OTHER} ${GMOCK_CFLAGS_OTHER})

add_library(libut_gtest gmain.cpp)
target_link_libraries(libut_gtest 
    ${GTEST_LIBRARIES} 
    ${GMOCK_LIBRARIES}
)
add_library(animations SHARED 
    ../RandomWalkAnimation.cpp
    ../DigitalRainAnimation.cpp
    ../ParticlesAnimation.cpp
    ../collections.cpp
    ../math_utils.cpp
    ../value_animation.cpp
)
target_compile_options(animations PUBLIC
    -O0 -g
    -DUNIT_TEST
    #"-fsanitize=address"
)
target_link_options(animations PUBLIC
    "-Wl,--no-undefined"
    #"-fsanitize=address"
)
target_link_libraries(animations
    stdc++
)
add_executable(neopixels_ut 
    testRandomWalk.cpp
    testCollections.cpp
    testDigitalRain.cpp
    testParticles.cpp
)
target_compile_options(neopixels_ut PUBLIC
    -O0 -g
    #"-fsanitize=address"
)
target_link_options(neopixels_ut PUBLIC
    #"-fsanitize=address"
)
target_link_libraries(neopixels_ut
    animations
    libut_gtest
    pthread
    stdc++
    m
)
