cmake_minimum_required(VERSION 3.28)
project(iLumidi)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# JUCE Directory
set(JUCE_DIR /Users/jamiebenchia/JUCE)
include_directories("${CMAKE_SOURCE_DIR}/JuceLibraryCode")
# Add JUCE subdirectory
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

# Use file(GLOB ...) to find source files
file(GLOB SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/Source/*.cpp"
        "${CMAKE_SOURCE_DIR}/Source/*.h"
)

# Main executable with GLOB-ed source files
add_executable(iLumidi ${SOURCE_FILES})

# Link to the required JUCE modules
target_link_libraries(iLumidi
        PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)