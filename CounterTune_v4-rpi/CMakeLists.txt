cmake_minimum_required(VERSION 3.15)

project(CounterTune_v4 VERSION 1.4.1)

set(CMAKE_CXX_STANDARD 17)

option(DEMO_BUILD "Build demo version" OFF)

# JUCE lives at ~/JUCE (same as GuitarGuy)
set(JUCE_PATH "$ENV{HOME}/JUCE" CACHE PATH "Path to JUCE")
add_subdirectory(${JUCE_PATH} JUCE)

if(DEMO_BUILD)
    set(PLUGIN_CODE "CtnD")
    set(PLUGIN_PRODUCT_NAME "CounterTune TRIAL VERSION")
    set(PLUGIN_BUNDLE_ID "com.MixedSignalsAudio.CounterTuneD4")
else()
    set(PLUGIN_CODE "CtnM")
    set(PLUGIN_PRODUCT_NAME "CounterTune")
    set(PLUGIN_BUNDLE_ID "com.MixedSignalsAudio.CounterTuneV4")
endif()

juce_add_plugin(CounterTune_v4
    PLUGIN_MANUFACTURER_CODE "Manu"
    PLUGIN_CODE "${PLUGIN_CODE}"
    FORMATS Standalone VST3
    PRODUCT_NAME "${PLUGIN_PRODUCT_NAME}"
    COMPANY_NAME "Mixed Signals Audio"
    BUNDLE_ID "${PLUGIN_BUNDLE_ID}"
    PLUGIN_IS_A_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    IS_MIDI_EFFECT FALSE
    IS_SYNTH FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    MIDI_INPUT_NAME "MIDI Input"
    MIDI_OUTPUT_NAME "MIDI Output"
)

target_compile_definitions(CounterTune_v4
    PRIVATE
        JUCE_DISABLE_VST2_BUILDING=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_AU=0
        $<$<BOOL:${DEMO_BUILD}>:DEMO_BUILD>
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# Headless: no PluginEditor, no UI BinaryData
target_sources(CounterTune_v4 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PedalboardControls.cpp
    Source/PedalboardControls.h
    Source/CtLog.h
    Source/dywapitchtrack.h
    Source/dywapitchtrack.c
    Source/Pdct/cComplex.cpp
    Source/Pdct/cComplex.h
    Source/Pdct/cComplexResonator.cpp
    Source/Pdct/cComplexResonator.h
    Source/Pdct/cPdct.cpp
    Source/Pdct/cPdct.h
    Source/Pdct/cPdctOutput.cpp
    Source/Pdct/cPdctOutput.h
    Source/Pdct/common_type.h
    Source/Pdct/log_config.h
    Source/Pdct/log_info.h
)

target_include_directories(CounterTune_v4 PRIVATE Source/Pdct)
target_compile_definitions(CounterTune_v4 PRIVATE _USE_MATH_DEFINES)

target_link_libraries(CounterTune_v4 PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

juce_generate_juce_header(CounterTune_v4)
