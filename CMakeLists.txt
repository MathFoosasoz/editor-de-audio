cmake_minimum_required(VERSION 3.15)
project(AudioEditor CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCES
    main.cpp
    src/core/AudioFile.cpp
    src/core/MPEGFile.cpp
    src/core/MP3File.cpp
    src/frame/MPEGFrame.cpp
    src/frame/Layer3Frame.cpp
    src/metadata/ID3v2Tag.cpp
    src/cli/CommandDispatcher.cpp
)

add_executable(audio_editor ${SOURCES})