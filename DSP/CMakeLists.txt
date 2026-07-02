add_library(SlopeOverload-dsp STATIC IRLoader.cpp)
target_link_libraries(SlopeOverload-dsp PUBLIC IADSP)
target_include_directories(SlopeOverload-dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
