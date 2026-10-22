# Symbolic link for debug version:
# mklink /D "C:\Program Files\Common Files\VST3\GuitarGuy.vst3" "C:\Users\savas\Documents\JUCE Projects\GuitarGuy\cmake_build\GuitarGuy_artefacts\Debug\VST3\GuitarGuy.vst3"

# visual studio project preferences:

# command: C:\Program Files (x86)\Image-Line\FL Studio 12\FL64.exe


cmake_minimum_required(VERSION 3.15)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for macOS")
endif()

project(GuitarGuy VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# JUCE path
if(APPLE)
    set(JUCE_PATH "/Users/admin/Documents/repos/JUCE")
elseif(NOT APPLE)
    set(JUCE_PATH "C:/Program Files/JUCE")
endif()

# Include JUCE
add_subdirectory(${JUCE_PATH} JUCE)

# Conditional formats
if(APPLE)
    set(PLUGIN_FORMATS AU VST3)
elseif(NOT APPLE)
    set(PLUGIN_FORMATS VST3)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    add_compile_options(
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
    )
endif()

set(PLUGIN_CODE "GuiG")
set(PLUGIN_PRODUCT_NAME "GuitarGuy")
set(PLUGIN_BUNDLE_ID "com.MixedSignalsAudio.GuitarGuy")

# Plugin setup arguments (common)
set(PLUGIN_ARGS
    PLUGIN_MANUFACTURER_CODE "Manu"
    PLUGIN_CODE "${PLUGIN_CODE}"
    FORMATS ${PLUGIN_FORMATS}
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

# Plugin setup
juce_add_plugin(GuitarGuy ${PLUGIN_ARGS})

target_compile_definitions(GuitarGuy
    PRIVATE
        JUCE_DISABLE_VST2_BUILDING=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# Source files
target_sources(GuitarGuy PRIVATE
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/ADSRData.cpp
    Source/ADSRData.h
    Source/SynthVoice.cpp
    Source/SynthVoice.h
    Source/SynthSound.h
    Source/dr_wav.h
)

# Binary data
file(GLOB_RECURSE ALL_WAV_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Audio/*.wav")
file(GLOB_RECURSE ALL_PNG_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Photo/*.png")

juce_add_binary_data(BinaryResources SOURCES
    ${ALL_WAV_FILES}
    ${ALL_PNG_FILES}
)

# Additional Libraries

# Link everything
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
