add_library(SlopeOverload-impl STATIC passthrough_clap.cpp)
target_link_libraries(SlopeOverload-impl PUBLIC clap)

make_clapfirst_plugins(
        TARGET_NAME SlopeOverload
        IMPL_TARGET SlopeOverload-impl

        OUTPUT_NAME "Slope Overload"

        ENTRY_SOURCE "passthrough_clap_entry.cpp"

        BUNDLE_IDENTIFIER "com.icebreakeraudio.slopeoverload"
        BUNDLE_VERSION ${PROJECT_VERSION}

        COPY_AFTER_BUILD FALSE

        PLUGIN_FORMATS CLAP VST3 AUV2

        ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/SlopeOverload_assets

        AUV2_MANUFACTURER_NAME "Icebreaker Audio"
        AUV2_MANUFACTURER_CODE "IceB"
        AUV2_SUBTYPE_CODE "IASO"
        AUV2_INSTRUMENT_TYPE "aufx"

        STANDALONE_CONFIGURATIONS
        standalone "Slope Overload" com.icebreakeraudio.slopeoverload
)
