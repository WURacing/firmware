# Host tests for 2026 firmware: each sketch is compiled for this computer
# against the fake Arduino core in fakes/, then driven by GoogleTest.
# Run with `python scripts/test.py` from the repo root.

cmake_minimum_required(VERSION 3.16)
project(firmware_2026_tests CXX)

# The boards build with -std=gnu++11; GoogleTest needs at least C++14.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# arduino_preprocess.py prepares each sketch for the build.
find_package(Python3 REQUIRED COMPONENTS Interpreter)

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
# The library's global `CAN` object, for sketches that don't declare their own.
add_library(fake_can_object STATIC fakes/can_object.cpp)
target_link_libraries(fake_can_object PUBLIC fake_hardware)

include(GoogleTest)
enable_testing()

# Adds a test executable for one sketch folder. The test .cpp includes the
# sketch's .ino directly, so the real firmware is what gets tested.
#   add_sketch_test(<name> SKETCH_DIR <dir> SOURCES <test.cpp>...
#                   [SKETCH_DEFINES_CAN])
# Pass SKETCH_DEFINES_CAN when the sketch declares its own `CANSAME5x CAN;`.
function(add_sketch_test name)
  cmake_parse_arguments(TEST "SKETCH_DEFINES_CAN" "SKETCH_DIR" "SOURCES"
                        ${ARGN})
  add_executable(${name} ${TEST_SOURCES})
  # The test includes <Sketch>.ino; it gets a copy prepared the way the
  # Arduino builder prepares it (see arduino_preprocess.py), found before
  # the original because its folder is listed first.
  get_filename_component(sketch_name ${TEST_SKETCH_DIR} NAME)
  file(GLOB sketch_inos CONFIGURE_DEPENDS "${TEST_SKETCH_DIR}/*.ino")
  set(prepared_dir ${CMAKE_CURRENT_BINARY_DIR}/${name}_sketch)
  set(prepared_ino ${prepared_dir}/${sketch_name}.ino)
  add_custom_command(
    OUTPUT ${prepared_ino}
    COMMAND Python3::Interpreter
            ${CMAKE_CURRENT_SOURCE_DIR}/arduino_preprocess.py
            ${TEST_SKETCH_DIR} ${prepared_ino}
    DEPENDS ${sketch_inos} ${CMAKE_CURRENT_SOURCE_DIR}/arduino_preprocess.py
    COMMENT "Preparing ${sketch_name}.ino like the Arduino builder")
  add_custom_target(${name}_sketch DEPENDS ${prepared_ino})
  add_dependencies(${name} ${name}_sketch)
  target_include_directories(${name} PRIVATE ${prepared_dir}
                                             ${TEST_SKETCH_DIR})
  target_link_libraries(${name} PRIVATE fake_hardware GTest::gtest_main)
  if(NOT TEST_SKETCH_DEFINES_CAN)
    target_link_libraries(${name} PRIVATE fake_can_object)
  endif()
  if(MSVC)
    target_compile_options(${name} PRIVATE /W4)
  else()
    target_compile_options(${name} PRIVATE -Wall -Wextra)
  endif()
  # ctest runs each test in its own process, so sketch globals (including
  # function-local statics) start fresh.
  gtest_discover_tests(${name})
endfunction()

add_sketch_test(control_board_test
  SKETCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ControlBoard
  SOURCES ControlBoard/control_board_test.cpp)

add_sketch_test(fsb_test
  SKETCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../FSB_CODE
  SOURCES FSB_CODE/fsb_test.cpp)


add_sketch_test(telemetry_sender_test
  SKETCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Telemetry/Telemetry_Sender
  SOURCES Telemetry_Sender/telemetry_sender_test.cpp)
