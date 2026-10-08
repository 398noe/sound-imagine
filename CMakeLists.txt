cmake_minimum_required(VERSION 3.24)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MSVCIncludePrefix.cmake")
project(SoundImagine VERSION 2.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/External/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "Run git submodule update --init --recursive first.")
endif()
add_subdirectory(External/JUCE)
juce_add_plugin(SoundImagine
    COMPANY_NAME "398noe" COMPANY_WEBSITE "https://github.com/398noe"
    BUNDLE_ID "com.398noe.soundimagine"
    PLUGIN_MANUFACTURER_CODE Nyao PLUGIN_CODE Imag PRODUCT_NAME "SoundImagine"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer FORMATS VST3 Standalone)
target_sources(SoundImagine PRIVATE src/Analyzer.cpp src/PluginProcessor.cpp src/PluginEditor.cpp)
target_include_directories(SoundImagine PUBLIC src)
target_compile_definitions(SoundImagine PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SoundImagine PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(SoundImagine PRIVATE /utf-8)
endif()
include(CTest)
if(BUILD_TESTING)
    juce_add_console_app(sound_imagine_tests PRODUCT_NAME "SoundImagine tests")
    target_sources(sound_imagine_tests PRIVATE tests/Tests.cpp src/Analyzer.cpp src/PluginProcessor.cpp src/PluginEditor.cpp)
    target_include_directories(sound_imagine_tests PRIVATE src)
    target_compile_definitions(sound_imagine_tests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(sound_imagine_tests PRIVATE juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    if(MSVC)
        target_compile_options(sound_imagine_tests PRIVATE /utf-8)
    endif()
    add_dependencies(sound_imagine_tests SoundImagine_VST3)
    add_test(NAME analysis_and_plugin COMMAND sound_imagine_tests "${CMAKE_BINARY_DIR}/verification"
        "$<TARGET_FILE_DIR:SoundImagine_VST3>/../..")
    set_tests_properties(analysis_and_plugin PROPERTIES TIMEOUT 120)
endif()
