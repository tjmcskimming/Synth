cmake_minimum_required(VERSION 3.26)
project(Synth)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/Cellar/")

# Find required libraries
find_package(spdlog REQUIRED)
find_library(PORTAUDIO_LIBRARY NAMES portaudio)
find_library(FLTK_LIBRARY NAMES fltk)

# Specify the target and sources
add_executable(Synth main.cpp
        AudioEngine.cpp
        AudioEngine.h
)

# Specify include directories and link libraries for the target
target_include_directories(Synth PRIVATE /opt/homebrew/include)
target_link_libraries(Synth PRIVATE ${PORTAUDIO_LIBRARY} ${FLTK_LIBRARY} spdlog::spdlog)
