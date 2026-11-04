# Host tests for 2026 firmware: each sketch is compiled for this computer
# against the fake Arduino core in fakes/, then driven by GoogleTest.
# Run with `python scripts/test.py` from the repo root.

cmake_minimum_required(VERSION 3.16)
project(firmware_2026_tests CXX)

# The boards build with -std=gnu++11; GoogleTest needs at least C++14.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
  URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Use the same C runtime as the tests on Windows (MSVC).
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(fake_hardware STATIC fakes/fake_hardware.cpp)
# Listed first so <Arduino.h>, <CAN.h>, etc. resolve to the fakes.
target_include_directories(fake_hardware PUBLIC fakes)

include(GoogleTest)
enable_testing()

# Adds a test executable for one sketch folder. The test .cpp includes the
# sketch's .ino directly, so the real firmware is what gets tested.
function(add_sketch_test name sketch_dir)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE ${sketch_dir})
  target_link_libraries(${name} PRIVATE fake_hardware GTest::gtest_main)
  if(MSVC)
    target_compile_options(${name} PRIVATE /W4)
  else()
    target_compile_options(${name} PRIVATE -Wall -Wextra)
  endif()
  # Each test runs in its own process, so sketch globals start fresh.
  gtest_discover_tests(${name})
endfunction()

add_sketch_test(control_board_test
  ${CMAKE_CURRENT_SOURCE_DIR}/../ControlBoard
  ControlBoard/control_board_test.cpp)
