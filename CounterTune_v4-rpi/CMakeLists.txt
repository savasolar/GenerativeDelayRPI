# Symbolic link for debug version:
# mklink /D "C:\Program Files\Common Files\VST3\CounterTune_v4.vst3" "C:\Users\savas\Documents\JUCE Projects\CounterTune_v4\cmake_build\CounterTune_v4_artefacts\Debug\VST3\CounterTune.vst3"

# visual studio project preferences:

# command: C:\Program Files (x86)\Image-Line\FL Studio 12\FL64.exe


cmake_minimum_required(VERSION 3.15)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for macOS")
endif()

project(CounterTune_v4 VERSION 1.4.1)

set(CMAKE_CXX_STANDARD 17)

# Build configuration options
option(DEMO_BUILD "Build demo version" OFF)

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

# Conditional naming
if(DEMO_BUILD)
    set(PLUGIN_CODE "CtnD")
    set(PLUGIN_PRODUCT_NAME "CounterTune TRIAL VERSION")
    set(PLUGIN_BUNDLE_ID "com.MixedSignalsAudio.CounterTuneD4")
else()
    set(PLUGIN_CODE "CtnM")
    set(PLUGIN_PRODUCT_NAME "CounterTune")
    set(PLUGIN_BUNDLE_ID "com.MixedSignalsAudio.CounterTuneV4")
endif()

# Plugin setup arguments (common)
set(PLUGIN_ARGS
    PLUGIN_MANUFACTURER_CODE "Manu"
    PLUGIN_CODE "${PLUGIN_CODE}"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "${PLUGIN_PRODUCT_NAME}"
    COMPANY_NAME "Mixed Signals Audio"
    BUNDLE_ID "${PLUGIN_BUNDLE_ID}"
    PLUGIN_IS_A_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    IS_MIDI_EFFECT FALSE
    IS_SYNTH FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MIDI_INPUT_NAME "MIDI Input"
    MIDI_OUTPUT_NAME "MIDI Output"
)

# Plugin setup
juce_add_plugin(CounterTune_v4 ${PLUGIN_ARGS})

target_compile_definitions(CounterTune_v4
    PRIVATE
        JUCE_DISABLE_VST2_BUILDING=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=1
        JUCE_PLUGINHOST_AU=1
	$<$<BOOL:${DEMO_BUILD}>:DEMO_BUILD>
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# Source files
target_sources(CounterTune_v4 PRIVATE
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
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

set_source_files_properties(Dependencies/dywapitchtrack/src/dywapitchtrack.c PROPERTIES LANGUAGE C)

target_include_directories(CounterTune_v4 PRIVATE Source/Pdct Dependencies/vstsdk2.4)
target_compile_definitions(CounterTune_v4 PRIVATE _USE_MATH_DEFINES)

# Binary data
juce_add_binary_data(BinaryResources SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/template.png
	#
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/knob-facade.png
	#
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/load-button.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/load-button-active.png
	#
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/open-button.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/open-button-active.png
	#
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/reset-button.png
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/reset-button-active.png
	#
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/AzeretMono-Regular.ttf
	${CMAKE_CURRENT_SOURCE_DIR}/Resources/AzeretMono-Bold.ttf
)

# Link everything
target_link_libraries(CounterTune_v4 PRIVATE
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
)

juce_generate_juce_header(CounterTune_v4)
