add_library(SlopeOverload-impl STATIC SlopeOverloadPlugin.cpp Parameter.cpp ParamAttachment.cpp)
target_link_libraries(SlopeOverload-impl PUBLIC clap clap-helpers SlopeOverload-dsp SlopeOverload-ui)
target_compile_definitions(SlopeOverload-impl PUBLIC SLOPEOVERLOAD_VERSION_STRING="${PROJECT_VERSION}")
target_include_directories(SlopeOverload-impl PUBLIC "${CMAKE_SOURCE_DIR}/third_party/clap-wrapper/include")

make_clapfirst_plugins(
        TARGET_NAME SlopeOverload
        IMPL_TARGET SlopeOverload-impl

        OUTPUT_NAME "${PRODUCT_NAME}"

        ENTRY_SOURCE "SlopeOverloadPlugin_entry.cpp"

        BUNDLE_IDENTIFIER "${BUNDLE_ID}"
        BUNDLE_VERSION ${PROJECT_VERSION}

        COPY_AFTER_BUILD FALSE

        PLUGIN_FORMATS CLAP VST3 AUV2

        ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/SlopeOverload_assets

        AUV2_MANUFACTURER_NAME "${COMPANY_NAME}"
        AUV2_MANUFACTURER_CODE "IceB"
        AUV2_SUBTYPE_CODE "IASO"
        AUV2_INSTRUMENT_TYPE "aufx"

        STANDALONE_CONFIGURATIONS
        standalone "${PRODUCT_NAME}" "${BUNDLE_ID}"
)

if (WIN32)
    add_win32_version_info(TARGET SlopeOverload_clap
            FILE_DESCRIPTION "Slope Overload CLAP Plugin"
            ORIGINAL_FILENAME "Slope Overload.clap"
            INTERNAL_NAME "SlopeOverload" FILE_TYPE "VFT_DLL")
    add_win32_version_info(TARGET SlopeOverload_vst3
            FILE_DESCRIPTION "Slope Overload VST3 Plugin"
            ORIGINAL_FILENAME "Slope Overload.vst3"
            INTERNAL_NAME "SlopeOverload" FILE_TYPE "VFT_DLL")
    add_win32_version_info(TARGET SlopeOverload_standalone
            FILE_DESCRIPTION "Slope Overload Standalone"
            ORIGINAL_FILENAME "Slope Overload.exe"
            INTERNAL_NAME "SlopeOverload" FILE_TYPE "VFT_APP")
endif()
