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
