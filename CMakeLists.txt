cmake_minimum_required(VERSION 3.20)
project(THE_MIDS_ROOM VERSION 1.0.0)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.5 # Using the same version as the previous project for consistency
)
FetchContent_MakeAvailable(juce)

# Create the plugin target
juce_add_plugin(THE_MIDS_ROOM
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE MidR
    FORMATS VST3 AU LV2 Standalone
    PRODUCT_NAME "THE MIDS ROOM"
)

juce_generate_juce_header(THE_MIDS_ROOM)

target_sources(THE_MIDS_ROOM
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Release>:-O3 -march=native -ffast-math>
)

target_compile_definitions(THE_MIDS_ROOM
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(THE_MIDS_ROOM
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
