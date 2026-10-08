cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Version minimale de macOS")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")   # pas de DLL Visual C++ a installer

project(NeyOriental VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NEY_BUILD_TESTS "Compile le test d'integration (hote de test en ligne de commande)" OFF)
set(NEY_JUCE_PATH "" CACHE PATH "Dossier JUCE local (sinon JUCE est telecharge automatiquement)")

if(NEY_JUCE_PATH)
    add_subdirectory("${NEY_JUCE_PATH}" juce EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Les codes ci-dessous identifient le plugin dans FL Studio : ne jamais les changer,
# sinon les projets existants ne retrouvent plus le plugin.
juce_add_plugin(NeyOriental
    PRODUCT_NAME "Ney Oriental"
    COMPANY_NAME "MJ7"
    BUNDLE_ID com.mj7.neyoriental
    PLUGIN_MANUFACTURER_CODE Mjse
    PLUGIN_CODE Mj7n
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(NeyOriental PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_include_directories(NeyOriental PRIVATE Source)
target_compile_definitions(NeyOriental PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)
target_link_libraries(NeyOriental
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(NeyOriental PUBLIC /utf-8)
endif()

if(NEY_BUILD_TESTS)
    juce_add_console_app(NeyHostTest PRODUCT_NAME "NeyHostTest")
    target_sources(NeyHostTest PRIVATE tests/host_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(NeyHostTest PRIVATE Source)
    target_compile_definitions(NeyHostTest PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JucePlugin_Name="Ney Oriental" JucePlugin_Build_Standalone=1)
    target_link_libraries(NeyHostTest
        PRIVATE juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    if(MSVC)
        target_compile_options(NeyHostTest PUBLIC /utf-8)
    endif()
endif()
