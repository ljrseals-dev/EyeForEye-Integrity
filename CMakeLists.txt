cmake_minimum_required(VERSION 3.20)

project(EyeForEye-Integrity
    VERSION 0.1.0
    DESCRIPTION "C++17 voice synthesis and audio integrity foundation"
    LANGUAGES CXX
)

option(EYEFOREYE_BUILD_TESTS "Build the project tests" ON)

add_library(eye_for_eye
    src/version.cpp
)
add_library(EyeForEye::Core ALIAS eye_for_eye)

target_compile_features(eye_for_eye PUBLIC cxx_std_17)
target_include_directories(eye_for_eye
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(eye_for_eye PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(eye_for_eye PRIVATE /W4 /permissive-)
else()
    target_compile_options(eye_for_eye PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EYEFOREYE_BUILD_TESTS)
    enable_testing()
    add_executable(eye_for_eye_tests tests/test_version.cpp)
    target_link_libraries(eye_for_eye_tests PRIVATE EyeForEye::Core)
    target_compile_features(eye_for_eye_tests PRIVATE cxx_std_17)
    add_test(NAME eye_for_eye_tests COMMAND eye_for_eye_tests)
endif()
