cmake_minimum_required(VERSION 3.22)

# Must precede project(): these drive the compiler checks.
#
# Build a universal binary. Left unset, CMake targets its OWN architecture — and
# an x86_64 CMake (Intel Homebrew under Rosetta) then silently produces an
# Intel-only plug-in, which native arm64 hosts skip without any error. arm64
# macOS starts at 11.0, so the deployment target has to clear that.
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING
            "macOS build architectures" FORCE)
    endif()
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
            "Minimum macOS version" FORCE)
    endif()
endif()

project(CapicolaVST VERSION 0.4.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: either a sibling/absolute checkout (-DJUCE_DIR=...) or the macOS
# default install location.
set(JUCE_DIR "/Applications/JUCE" CACHE PATH "Path to a JUCE source checkout")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR} — pass -DJUCE_DIR=/path/to/JUCE")
endif()
add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

juce_add_plugin(Capicola
    COMPANY_NAME            "Sungam"
    BUNDLE_ID               "com.sungam.snida"
    PLUGIN_MANUFACTURER_CODE Sngm
    PLUGIN_CODE             Snda
    FORMATS                 VST3 AU Standalone
    PRODUCT_NAME            "Snida"
    DESCRIPTION             "Keyframe time-stretcher / transient auto-slicer"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES         Fx Delay "Pitch Shift"
    AU_MAIN_TYPE            kAudioUnitType_Effect)

target_sources(Capicola PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/CapicolaEngine.cpp
    Source/Panel.cpp)

target_compile_definitions(Capicola PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

# The DSP core, verbatim from the firmware tree. SYSTEM so the firmware's own
# warning profile doesn't bleed into JUCE's stricter one.
target_include_directories(Capicola SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib")
target_include_directories(Capicola PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_link_libraries(Capicola PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Dev tool: render the faceplate to PNGs without a host. -DCAPICOLA_PANEL_SHOT=ON
option(CAPICOLA_PANEL_SHOT "Build the offscreen faceplate renderer" OFF)
if(CAPICOLA_PANEL_SHOT)
    juce_add_console_app(panel_shot PRODUCT_NAME "panel_shot" NEEDS_CURL FALSE)
    target_sources(panel_shot PRIVATE
        tools/panel_shot.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
        Source/CapicolaEngine.cpp
        Source/Panel.cpp)
    target_compile_definitions(panel_shot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_include_directories(panel_shot SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../lib")
    target_include_directories(panel_shot PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_link_libraries(panel_shot PRIVATE juce::juce_audio_utils juce::juce_dsp)
endif()
