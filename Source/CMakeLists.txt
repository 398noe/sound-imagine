juce_add_plugin("${PROJECT_NAME}"
    FORMATS VST3 Standalone
    PRODUCT_NAME "SoundImagine"
    COMPANY_NAME "TechProject"
    COMPANY_WEBSITE "https://github.com/398noe"

    PLUGIN_MANUFACTURER_CODE Nyao
    PLUGIN_CODE Imag

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    VST3_CATEGORIES "Analyzer"

    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(${PROJECT_NAME})

set(SHADER_FILES
)

target_sources(${PROJECT_NAME}
    PRIVATE
        lib/AudioUtilities.cpp
        lib/Manager.cpp
        lib/Processor.cpp
        component/Imager.cpp
        component/LRImager.cpp
        PluginProcessor.cpp
        PluginEditor.cpp
        include/lib/AudioUtilities.h
        include/lib/Constant.h
        include/lib/Manager.h
        include/lib/Processor.h
        include/PluginProcessor.h
        include/PluginEditor.h
        include/component/Imager.h
        include/component/LRImager.h
        # ${SHADER_FILES}
)

target_include_directories("${PROJECT_NAME}"
    PRIVATE
        include
)

target_compile_definitions("${PROJECT_NAME}"
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_definitions("${PROJECT_NAME}"
    PRIVATE JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${PROJECT_NAME},PRODUCT_NAME>"
    PRIVATE JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${PROJECT_NAME},VERSION>"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
