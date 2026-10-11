cmake_minimum_required(VERSION 3.22)
project(17TESSVocalSauce VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recommended: build with Visual Studio 2022, x64. JUCE's VST3 wrapper uses Steinberg interfaces.
include(FetchContent)
FetchContent_Declare(JUCE
 GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
 GIT_TAG 8.0.7
 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VocalSauce
 COMPANY_NAME "17TESS AUDIO"
 BUNDLE_ID "com.17tess.audio.vocalsauce"
 PLUGIN_MANUFACTURER_CODE Tess
 PLUGIN_CODE VcS1
 FORMATS VST3
 PRODUCT_NAME "17TESS VOCAL SAUCE"
 DESCRIPTION "Minecraft-inspired multi FX vocal sauce"
 IS_SYNTH FALSE
 NEEDS_MIDI_INPUT FALSE
 NEEDS_MIDI_OUTPUT FALSE
 IS_MIDI_EFFECT FALSE
 EDITOR_WANTS_KEYBOARD_FOCUS FALSE
 VST3_CATEGORIES Fx
 COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(VocalSauce)
target_sources(VocalSauce PRIVATE source/PluginProcessor.cpp source/PluginEditor.cpp)
target_include_directories(VocalSauce PRIVATE source)
target_compile_definitions(VocalSauce PRIVATE
 JUCE_WEB_BROWSER=0
 JUCE_USE_CURL=0
 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(VocalSauce PRIVATE
 juce::juce_audio_utils
 juce::juce_audio_processors
 juce::juce_dsp
 juce::juce_gui_extra
 juce::juce_recommended_config_flags
 juce::juce_recommended_warning_flags)
