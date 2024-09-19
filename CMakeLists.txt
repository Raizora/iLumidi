cmake_minimum_required(VERSION 3.20)
project(iLumidi VERSION 1.0.0)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE Directory (Development Branch)
if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "/Users/jamiebenchia/JUCE")  # Update this path to your JUCE development branch
endif()

# Add JUCE
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

# Include Directories
include_directories(
        "${CMAKE_SOURCE_DIR}/JuceLibraryCode"
        "${JUCE_DIR}/modules"
)

# Collect Source Files
file(GLOB_RECURSE SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/Source/*.cpp"
        "${CMAKE_SOURCE_DIR}/Source/*.h"
)

# Main Executable
add_executable(iLumidi ${SOURCE_FILES})

# Link JUCE Libraries
target_link_libraries(iLumidi PRIVATE
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

# Set MacOSX Bundle (Optional)
set_target_properties(iLumidi PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.ilumidi"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

# Set Xcode Properties (Optional)
if (CMAKE_GENERATOR STREQUAL Xcode)
    set_target_properties(iLumidi PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
    )
endif()