cmake_minimum_required(VERSION 3.25)

project(SlopeOverload VERSION 0.1.0 LANGUAGES C CXX)

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

add_subdirectory(third_party/clap)
add_subdirectory(third_party/clap-helpers)
add_subdirectory(third_party/clap-wrapper)
add_subdirectory(third_party/IADSP)
add_subdirectory(third_party/visage)

# third_party/FFTConvolver has no CMakeLists.txt and isn't used until Phase 2's
# convolution stage - its build wiring is deferred there (see TASKS.md).

add_subdirectory(DSP)
add_subdirectory(Plugin)
