cmake_minimum_required(VERSION 3.22)
project(TruchoRiser VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

if(DEFINED ENV{CI})
    set(TRUCHO_COPY FALSE)
else()
    set(TRUCHO_COPY TRUE)
endif()

juce_add_plugin(TruchoRiser
    COMPANY_NAME "Trucho"
    PLUGIN_MANUFACTURER_CODE Trch
    PLUGIN_CODE Trsr
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "TruchoRiser"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD ${TRUCHO_COPY})

target_sources(TruchoRiser PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(TruchoRiser PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(TruchoRiser
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lib_flags
        juce::juce_recommended_warning_flags)
