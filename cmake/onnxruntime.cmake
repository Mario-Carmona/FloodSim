# ============================================================
# 🧠 ONNX Runtime C++ API integration using FetchContent
# ============================================================

include(FetchContent)

# ------------------------------------------------------------
# Check that ONNX_RUNTIME_VERSION is defined
# ------------------------------------------------------------
set(ONNX_RUNTIME_VERSION "1.23.2" CACHE STRING "ONNX Runtime C++ API version")

message(STATUS "Configuring ONNX Runtime C++ API (version ${ONNX_RUNTIME_VERSION})")

# ------------------------------------------------------------
# Select ONNX Runtime binary according to target platform
# ------------------------------------------------------------
if (WIN32)
    set(ONNX_RUNTIME_PLATFORM "win-x64")
    set(ONNX_RUNTIME_ARCHIVE_EXT "zip")
elseif (APPLE)
    set(ONNX_RUNTIME_PLATFORM "osx-x86_64")
    set(ONNX_RUNTIME_ARCHIVE_EXT "tgz")
else()
    set(ONNX_RUNTIME_PLATFORM "linux-x64")
    set(ONNX_RUNTIME_ARCHIVE_EXT "tgz")
endif()

set(ONNX_RUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_RUNTIME_VERSION}/onnxruntime-${ONNX_RUNTIME_PLATFORM}-${ONNX_RUNTIME_VERSION}.${ONNX_RUNTIME_ARCHIVE_EXT}")

message(STATUS "ONNX Runtime download URL resolved: ${ONNX_RUNTIME_URL}")

# Construct the custom installation path: <Project>/build/<OS>/_deps/onnxruntime
set(ONNX_RUNTIME_CUSTOM_INSTALL_DIR "$ENV{HOST_BUILD_DIR}/_deps/onnxruntime")

# ------------------------------------------------------------
# Fetch ONNX Runtime (download + extract into the build tree)
# ------------------------------------------------------------
FetchContent_Declare(
    onnxruntime
    URL ${ONNX_RUNTIME_URL}
    # Enforce exact extraction into the specified custom directory structure
    SOURCE_DIR "${ONNX_RUNTIME_CUSTOM_INSTALL_DIR}"
    SUBBUILD_DIR "${ONNX_RUNTIME_CUSTOM_INSTALL_DIR}-subbuild" 
)

message(STATUS "Fetching ONNX Runtime C++ API to: ${ONNX_RUNTIME_CUSTOM_INSTALL_DIR}...")

FetchContent_MakeAvailable(onnxruntime)

# ------------------------------------------------------------
# Expose ONNX Runtime include and library directories
# ------------------------------------------------------------
set(ONNX_RUNTIME_INCLUDE_DIR ${ONNX_RUNTIME_CUSTOM_INSTALL_DIR}/include)
set(ONNX_RUNTIME_LIB_DIR     ${ONNX_RUNTIME_CUSTOM_INSTALL_DIR}/lib)

message(STATUS "ONNX Runtime include directory: ${ONNX_RUNTIME_INCLUDE_DIR}")
message(STATUS "ONNX Runtime library directory: ${ONNX_RUNTIME_LIB_DIR}")

# ------------------------------------------------------------
# Define imported ONNX Runtime C++ API target
# ------------------------------------------------------------
add_library(onnxruntime SHARED IMPORTED)

if (WIN32)
    set(ONNX_RUNTIME_DLL ${ONNX_RUNTIME_LIB_DIR}/onnxruntime.dll CACHE INTERNAL "Global path to ONNX Runtime DLL")
    set(ONNX_RUNTIME_IMPLIB ${ONNX_RUNTIME_LIB_DIR}/onnxruntime.lib CACHE INTERNAL "Global path to ONNX Runtime import library")

    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION "${ONNX_RUNTIME_DLL}"
        IMPORTED_IMPLIB  "${ONNX_RUNTIME_IMPLIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${ONNX_RUNTIME_INCLUDE_DIR}"
    )

    set(ONNX_LIBS "${ONNX_RUNTIME_DLL}")

    # ============================================================
    # AUTOMATIC DLL DEPLOYMENT (vcpkg-style integration)
    # ============================================================
    # Overriding target_link_libraries to ensure that any EXECUTABLE 
    # linked with onnxruntime receives a copy of the DLL in its output directory.
    function(target_link_libraries _target)
        _target_link_libraries(${_target} ${ARGN})

        if(TARGET ${_target})
            get_target_property(_type ${_target} TYPE)
            if(_type STREQUAL "EXECUTABLE")
                add_custom_command(TARGET ${_target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${ONNX_RUNTIME_DLL}"
                        "$<TARGET_FILE_DIR:${_target}>"
                )
            endif()
        endif()
    endfunction()

elseif (APPLE)
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION ${ONNX_RUNTIME_LIB_DIR}/libonnxruntime.dylib
        INTERFACE_INCLUDE_DIRECTORIES ${ONNX_RUNTIME_INCLUDE_DIR}
    )

    set(ONNX_LIBS "${ONNX_RUNTIME_LIB_DIR}/libonnxruntime.dylib")

else() # Linux
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION ${ONNX_RUNTIME_LIB_DIR}/libonnxruntime.so
        INTERFACE_INCLUDE_DIRECTORIES ${ONNX_RUNTIME_INCLUDE_DIR}
    )

    # Glob all versioned shared object links (.so, .so.1, .so.1.23.2, etc.) to maintain symlink integrity
    file(GLOB ONNX_LIBS "${ONNX_RUNTIME_LIB_DIR}/libonnxruntime.so*")
endif()


include(GNUInstallDirs)

if(WIN32)
    # On Windows, .dll files are treated as execution binaries (RUNTIME).
    # They must be deployed to the 'bin' directory for the executable to resolve them dynamically at runtime.
    install(FILES ${ONNX_LIBS}
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT Unspecified)
else()
    # On Linux and macOS, .so / .dylib files are treated as shared libraries (LIBRARY).
    # Their proper, standard FHS-compliant deployment location is the 'lib' directory.
    install(FILES ${ONNX_LIBS}
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT Unspecified)
endif()


set(ONNX_LICENSE_FILE "${ONNX_RUNTIME_CUSTOM_INSTALL_DIR}/LICENSE" CACHE INTERNAL "Path to ONNX License")


message(STATUS "ONNX Runtime C++ API successfully configured")
