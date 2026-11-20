cmake_minimum_required(VERSION 3.15)
project(Nicturne VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Загружаем JUCE из официального репозитория
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

# Создаём VST3 плагин
juce_add_plugin(Nicturne
    COMPANY_NAME "Nicturne"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Nctr
    PLUGIN_CODE Nctr
    FORMATS VST3
    PRODUCT_NAME "Nicturne"
)

# Исходные файлы
target_sources(Nicturne PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# Если есть отдельные DSP-файлы, добавьте их:
# target_sources(Nicturne PRIVATE Source/DSP.h Source/SynthEngine.h)

# Дефайны JUCE
target_compile_definitions(Nicturne PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Линкуем библиотеки JUCE
target_link_libraries(Nicturne PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_generate_juce_header(Nicturne)
