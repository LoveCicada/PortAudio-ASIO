cmake_minimum_required(VERSION 3.16)

if(MSVC)
    set(CMAKE_MFC_FLAG 2)
endif()

project(LtcAsioPlayer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "LtcAsioPlayer is Windows-only (ASIO + MFC).")
endif()

set(PORTAUDIO_ROOT "E:/opensource/portaudio/portaudio-19.7.0"
    CACHE PATH "PortAudio source root")
set(ASIOSDK_ROOT_DIR "E:/opensource/portaudio/ASIO-SDK_2.3.4_2025-10-15/ASIOSDK"
    CACHE PATH "ASIO SDK root (contains common/asio.h)")

if(NOT EXISTS "${PORTAUDIO_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "PORTAUDIO_ROOT not found: ${PORTAUDIO_ROOT}")
endif()
if(NOT EXISTS "${ASIOSDK_ROOT_DIR}/common/asio.h")
    message(FATAL_ERROR "ASIOSDK_ROOT_DIR not found: ${ASIOSDK_ROOT_DIR}")
endif()

list(APPEND CMAKE_MODULE_PATH "${PORTAUDIO_ROOT}/cmake_support")

set(PA_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PA_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(PA_USE_ASIO ON CACHE BOOL "" FORCE)
set(PA_USE_DS OFF CACHE BOOL "" FORCE)
set(PA_USE_WMME OFF CACHE BOOL "" FORCE)
set(PA_USE_WASAPI OFF CACHE BOOL "" FORCE)
set(PA_USE_WDMKS OFF CACHE BOOL "" FORCE)

add_subdirectory("${PORTAUDIO_ROOT}" portaudio_build)

set(LTC_ASIO_PLAYER_SOURCES
    src/app/LtcAsioPlayerApp.cpp
    src/ui/MainDlg.cpp
    src/audio/WavFileReader.cpp
    src/audio/DeviceEnumerator.cpp
    src/audio/AudioEngine.cpp
    res/LtcAsioPlayer.rc
)

add_executable(LtcAsioPlayer WIN32 ${LTC_ASIO_PLAYER_SOURCES})

target_include_directories(LtcAsioPlayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/res
    ${PORTAUDIO_ROOT}/include
)

target_compile_definitions(LtcAsioPlayer PRIVATE
    _AFXDLL
    PA_USE_ASIO=1
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0601
)

target_compile_features(LtcAsioPlayer PRIVATE cxx_std_11)

target_link_libraries(LtcAsioPlayer PRIVATE portaudio_static comctl32)

if(MSVC)
    target_compile_options(LtcAsioPlayer PRIVATE /W3)
endif()

set_target_properties(LtcAsioPlayer PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(MSVC)
    set_target_properties(LtcAsioPlayer PROPERTIES
        VS_GLOBAL_UseOfMfc "Dynamic"
    )
endif()
