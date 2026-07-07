cmake_minimum_required(VERSION 3.25)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" SLOPEOVERLOAD_VERSION LIMIT_COUNT 1)

project(SlopeOverload VERSION ${SLOPEOVERLOAD_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pin the MSVC runtime library here, at the root, before any add_subdirectory() call, so every
# submodule (clap-wrapper's own libs, the CPM-fetched VST3 SDK, etc.) links against the same
# runtime. Left to its own devices, clap-wrapper's CMakeLists.txt only sets this within its own
# subdirectory scope, which doesn't reach sibling targets like ours and causes an LNK2038
# static/dynamic runtime mismatch when linking the VST3 wrapper.
if (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Auto-fetch the VST3/AudioUnit SDKs via CPM instead of vendoring them (see TASKS.md Open Items #1)
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "Download VST3/AudioUnit SDKs via CPM" FORCE)

# Force static linking for third-party deps that default to BUILD_SHARED_LIBS (e.g. visage's
# vendored FreeType). Plugin bundles are loaded into a host's process directory, not this build
# tree, so a sibling DLL (e.g. freetyped.dll) would never be found on the host's DLL search path.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build all libraries as static by default" FORCE)

add_subdirectory(third_party/clap)
add_subdirectory(third_party/clap-helpers)
add_subdirectory(third_party/clap-wrapper)
add_subdirectory(third_party/IADSP)
add_subdirectory(third_party/visage)

# third_party/FFTConvolver has no CMakeLists.txt of its own - cmake/FFTConvolver.cmake
# declares its static library target from its 4 loose sources.
include(cmake/FFTConvolver.cmake)

include(cmake/WindowsVersionInfo.cmake)

add_subdirectory(DSP)
add_subdirectory(UI)
add_subdirectory(Plugin)
