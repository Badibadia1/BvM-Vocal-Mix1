cmake_minimum_required(VERSION 3.22)

# Universal macOS build (Intel + Apple Silicon); override with -DCMAKE_OSX_ARCHITECTURES=... if needed.
if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
endif()
if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

project(BvM VERSION 1.4.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
option(BVM_BUILD_PLUGIN "Build the JUCE plug-in targets" ON)
option(BVM_BUILD_TESTS "Build standalone DSP regression tests" ON)
set(BVM_JUCE_SOURCE_DIR "" CACHE PATH "Optional path to an existing JUCE checkout")

if(BVM_BUILD_TESTS)
  add_executable(BvM_DSP_Tests tests/dsp_smoke.cpp)
  target_include_directories(BvM_DSP_Tests PRIVATE Source)
  if(MSVC)
    target_compile_options(BvM_DSP_Tests PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(BvM_DSP_Tests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME BvM_DSP_Tests COMMAND BvM_DSP_Tests)
endif()

if(BVM_BUILD_PLUGIN)
  if(BVM_JUCE_SOURCE_DIR)
    add_subdirectory("${BVM_JUCE_SOURCE_DIR}" JUCE)
  else()
    include(FetchContent)
    FetchContent_Declare(JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.4
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
  endif()

  if(APPLE)
    set(BVM_FORMATS VST3 AU Standalone)
  else()
    set(BVM_FORMATS VST3 Standalone)
  endif()
  set(BVM_COPY TRUE)
  if(DEFINED ENV{CI})
    set(BVM_COPY FALSE)
  endif()

  juce_add_plugin(BvM
    COMPANY_NAME "BadiBadia"
    PLUGIN_MANUFACTURER_CODE Bdba
    PLUGIN_CODE Bvm1
    FORMATS ${BVM_FORMATS}
    PRODUCT_NAME "BvM"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${BVM_COPY})

  target_sources(BvM PRIVATE Source/PluginProcessor.cpp)
  target_compile_definitions(BvM PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=0)
  if(MSVC)
    target_compile_options(BvM PRIVATE /utf-8 /W4 /permissive-)
  else()
    target_compile_options(BvM PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  target_link_libraries(BvM
    PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
endif()

