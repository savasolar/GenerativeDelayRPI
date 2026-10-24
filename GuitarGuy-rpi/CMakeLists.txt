cmake_minimum_required(VERSION 3.15)

project(GuitarGuy VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# JUCE lives at ~/JUCE
set(JUCE_PATH "$ENV{HOME}/JUCE" CACHE PATH "Path to JUCE")
add_subdirectory(${JUCE_PATH} JUCE)

set(PLUGIN_CODE "GuiG")
set(PLUGIN_PRODUCT_NAME "GuitarGuy")
set(PLUGIN_BUNDLE_ID "com.MixedSignalsAudio.GuitarGuy")

juce_add_plugin(GuitarGuy
    PLUGIN_MANUFACTURER_CODE "Manu"
    PLUGIN_CODE "${PLUGIN_CODE}"
    FORMATS VST3
    PRODUCT_NAME "${PLUGIN_PRODUCT_NAME}"
    COMPANY_NAME "Mixed Signals Audio"
    BUNDLE_ID "${PLUGIN_BUNDLE_ID}"
    PLUGIN_IS_A_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    IS_MIDI_EFFECT FALSE
    IS_SYNTH TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    MIDI_INPUT_NAME "MIDI Input"
    MIDI_OUTPUT_NAME "MIDI Output"
)

target_compile_definitions(GuitarGuy
    PRIVATE
        JUCE_DISABLE_VST2_BUILDING=1
        JUCE_PLUGINHOST_VST3=0
        JUCE_PLUGINHOST_VST=0
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_sources(GuitarGuy PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/ADSRData.cpp
    Source/ADSRData.h
    Source/SynthVoice.cpp
    Source/SynthVoice.h
    Source/SynthSound.h
    Source/dr_wav.h
)

file(GLOB_RECURSE ALL_WAV_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Audio/*.wav")

juce_add_binary_data(BinaryResources SOURCES ${ALL_WAV_FILES})

target_link_libraries(GuitarGuy PRIVATE
    BinaryResources
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
    juce::juce_osc
)

juce_generate_juce_header(GuitarGuy)