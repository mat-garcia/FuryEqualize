cmake_minimum_required(VERSION 3.20)
project(FuryEqualizeCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(FuryEqualizeCore SHARED
    src/audio_engine.cpp
    src/dsp/biquad.cpp
    src/dsp/compressor.cpp
    src/dsp/sniper_preset.cpp
    src/wasapi/wasapi_engine.cpp
)

target_include_directories(FuryEqualizeCore PUBLIC include)

# WASAPI / COM libs
target_link_libraries(FuryEqualizeCore PRIVATE
    ole32
    avrt
    mmdevapi
    ksuser
    mfplat
)

target_compile_definitions(FuryEqualizeCore PRIVATE UNICODE _UNICODE NOMINMAX)

if(MSVC)
    target_compile_options(FuryEqualizeCore PRIVATE /W4 /permissive-)
    set_target_properties(FuryEqualizeCore PROPERTIES
        OUTPUT_NAME "FuryEqualizeCore"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
    )
endif()
