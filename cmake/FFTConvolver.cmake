# third_party/FFTConvolver ships without a CMakeLists.txt of its own - this module declares
# its static library target from its 4 loose sources.

set(FFTCONVOLVER_DIR "${CMAKE_SOURCE_DIR}/third_party/FFTConvolver")

add_library(FFTConvolver STATIC
    "${FFTCONVOLVER_DIR}/AudioFFT.cpp"
    "${FFTCONVOLVER_DIR}/FFTConvolver.cpp"
    "${FFTCONVOLVER_DIR}/TwoStageFFTConvolver.cpp"
    "${FFTCONVOLVER_DIR}/Utilities.cpp"
)

target_include_directories(FFTConvolver PUBLIC
    $<BUILD_INTERFACE:${FFTCONVOLVER_DIR}>
    $<INSTALL_INTERFACE:include>
)

# FFTConvolver's own SSE auto-detection (__SSE__ / _M_IX86_FP) never fires under MSVC - that
# compiler doesn't define either macro even on x64, where SSE2 is guaranteed baseline. Without
# this, the MSVC build silently falls back to FFTConvolver's scalar complex-multiply path.
# GCC/Clang already self-detect correctly and are left alone.
if (MSVC)
    target_compile_definitions(FFTConvolver PRIVATE FFTCONVOLVER_USE_SSE)
endif()
