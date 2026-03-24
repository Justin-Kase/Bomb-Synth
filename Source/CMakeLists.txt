juce_add_binary_data(BombSynthBinaryData
    SOURCES ../Resources/logo.png
)

juce_add_plugin(BombSynth
    COMPANY_NAME        "Illbomb"
    BUNDLE_ID           com.illbomb.bombsynth
    IS_SYNTH            TRUE
    NEEDS_MIDI_INPUT    TRUE
    NEEDS_MIDI_OUTPUT   FALSE
    IS_MIDI_EFFECT      FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    PLUGIN_MANUFACTURER_CODE    ILbm
    PLUGIN_CODE                 BSyn
    FORMATS             VST3 AU
    PRODUCT_NAME        "Bomb Synth"
)

file(GLOB_RECURSE PLUGIN_SOURCES
    "*.h" "*.cpp"
    "engine/*.h"       "engine/*.cpp"
    "engine/oscillators/*.h" "engine/oscillators/*.cpp"
    "engine/filters/*.h"     "engine/filters/*.cpp"
    "engine/envelopes/*.h"   "engine/envelopes/*.cpp"
    "modulation/*.h"   "modulation/*.cpp"
    "effects/*.h"      "effects/*.cpp"
    "sequencer/*.h"    "sequencer/*.cpp"
    "ui/*.h"           "ui/*.cpp"
    "ui/visualizers/*.h" "ui/visualizers/*.cpp"
)

target_sources(BombSynth PRIVATE ${PLUGIN_SOURCES})

target_compile_definitions(BombSynth
    PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(BombSynth
    PRIVATE
        BombSynthBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
