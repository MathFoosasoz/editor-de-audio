cmake_minimum_required(VERSION 3.15)
project(AudioEditor CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE LAME_SOURCES "src/3rdparty/lame/*.c")
add_library(lame STATIC ${LAME_SOURCES})
target_include_directories(lame PUBLIC 
    src/3rdparty/lame/
    src/3rdparty/lame/vector/
)

target_compile_options(lame PRIVATE 
    "SHELL:-include stdint.h" 
    "SHELL:-include string.h" 
    "SHELL:-include stdlib.h"
    -Dieee754_float32_t=float
)

set(SOURCES
    main.cpp
    src/core/MPEGFile.cpp
    src/core/MP3File.cpp
    src/frame/MPEGFrame.cpp
    src/core/PCMFile.cpp
    src/frame/Layer3Frame.cpp
    src/frame/AudioFrame.cpp
    src/metadata/ID3v2Tag.cpp
    src/cli/CommandDispatcher.cpp
)

add_executable(audio_editor ${SOURCES})

target_link_libraries(audio_editor PRIVATE lame)

target_include_directories(audio_editor PRIVATE 
    include/
)