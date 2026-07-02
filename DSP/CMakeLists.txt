add_library(SlopeOverload-dsp STATIC IRLoader.cpp DeltaModulation.cpp Speaker.cpp)
target_link_libraries(SlopeOverload-dsp PUBLIC IADSP FFTConvolver)
target_include_directories(SlopeOverload-dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
