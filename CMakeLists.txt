cmake_minimum_required(VERSION 3.20)

project(Eyesore
    VERSION 0.1.0
    DESCRIPTION "C++17 voice synthesis and audio integrity engine"
    LANGUAGES CXX
)

option(EYESORE_BUILD_TESTS "Build the Eyesore project tests" ON)

add_library(eye_for_eye src/version.cpp)
add_library(Eyesore::Core ALIAS eye_for_eye)
add_library(EyeForEye::Core ALIAS eye_for_eye)
target_compile_features(eye_for_eye PUBLIC cxx_std_17)
target_include_directories(eye_for_eye
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
set_target_properties(eye_for_eye PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(eye_for_eye PRIVATE /W4 /permissive-)
else()
    target_compile_options(eye_for_eye PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(eye_core STATIC
    src/core/PositionMap.cpp
    src/core/ScaleEngine.cpp
    src/audio/Envelope.cpp
    src/audio/WavReader.cpp
    src/audio/SampleBank.cpp
    src/audio/Renderer.cpp
    src/dsp/FFT.cpp
    src/dsp/PitchShifter.cpp
)
target_include_directories(eye_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(eye_core PUBLIC cxx_std_17)
set_target_properties(eye_core PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(eye_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(eye_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()

if(EYESORE_BUILD_TESTS)
    enable_testing()
    add_executable(eye_for_eye_tests tests/test_version.cpp)
    target_link_libraries(eye_for_eye_tests PRIVATE Eyesore::Core)
    target_compile_features(eye_for_eye_tests PRIVATE cxx_std_17)
    add_test(NAME eyesore_version COMMAND eye_for_eye_tests)

    add_executable(test_position_map tests/test_position_map.cpp)
    target_link_libraries(test_position_map PRIVATE eye_core)
    add_test(NAME position_map COMMAND test_position_map)

    add_executable(test_scale_engine tests/test_scale_engine.cpp)
    target_link_libraries(test_scale_engine PRIVATE eye_core)
    add_test(NAME scale_engine COMMAND test_scale_engine)

    add_executable(test_pitch_shifter tests/test_pitch_shifter.cpp)
    target_link_libraries(test_pitch_shifter PRIVATE eye_core)
    add_test(NAME pitch_shifter COMMAND test_pitch_shifter)

    add_executable(artifact_harness src/harness/ArtifactHarness.cpp)
    target_link_libraries(artifact_harness PRIVATE eye_core)
    add_test(NAME artifact_harness COMMAND artifact_harness)
endif()
