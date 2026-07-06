set(SlopeOverloadAssetFiles
        "${CMAKE_SOURCE_DIR}/Assets/Background.svg"
        "${CMAKE_SOURCE_DIR}/Assets/DigitalNumbers-Regular.ttf"
        "${CMAKE_SOURCE_DIR}/Assets/VT323-Regular.ttf"
        "${CMAKE_SOURCE_DIR}/Assets/PowerButton_On.svg"
        "${CMAKE_SOURCE_DIR}/Assets/PowerButton_Off.svg")

add_embedded_resources(SlopeOverloadAssets "SlopeOverloadAssets.h" "assets" "${SlopeOverloadAssetFiles}")

add_library(SlopeOverload-ui STATIC
        SlopeOverloadEditor.cpp
        DigitalSlider.cpp
        GroupedToggle.cpp
        PowerToggleButton.cpp
        ShadowToggleButton.cpp
        ShadowLabel.cpp)
target_link_libraries(SlopeOverload-ui PUBLIC visage SlopeOverloadAssets)
target_include_directories(SlopeOverload-ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
