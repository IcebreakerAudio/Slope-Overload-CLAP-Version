# Embeds a Win32 VERSIONINFO resource (Company/Product/Copyright/FileVersion,
# visible in Explorer's Properties > Details tab) into a module/executable target.
# No-op on non-Windows platforms; clap-wrapper itself doesn't provide this for
# CLAP/VST3 module targets, so each packaged format needs to opt in explicitly.
function(add_win32_version_info)
    set(oneValueArgs TARGET FILE_DESCRIPTION ORIGINAL_FILENAME INTERNAL_NAME FILE_TYPE)
    cmake_parse_arguments(WVI "" "${oneValueArgs}" "" ${ARGN})

    if (NOT WIN32)
        return()
    endif()

    if (NOT DEFINED WVI_TARGET)
        message(FATAL_ERROR "add_win32_version_info requires TARGET")
    endif()

    set(SLOPEOVERLOAD_RC_FILEVERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    set(SLOPEOVERLOAD_RC_FILETYPE "${WVI_FILE_TYPE}")
    set(SLOPEOVERLOAD_RC_FILE_DESCRIPTION "${WVI_FILE_DESCRIPTION}")
    set(SLOPEOVERLOAD_RC_ORIGINAL_FILENAME "${WVI_ORIGINAL_FILENAME}")
    set(SLOPEOVERLOAD_RC_INTERNAL_NAME "${WVI_INTERNAL_NAME}")

    set(generated_rc "${CMAKE_BINARY_DIR}/generated_rc/${WVI_TARGET}_version.rc")
    configure_file("${CMAKE_SOURCE_DIR}/cmake/VersionInfo.rc.in" "${generated_rc}" @ONLY)
    target_sources(${WVI_TARGET} PRIVATE "${generated_rc}")
endfunction()
