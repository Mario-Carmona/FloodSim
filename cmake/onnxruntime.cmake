# ============================================================
# ONNX Runtime C++ API integration using FetchContent
# ============================================================

include(FetchContent)

# ------------------------------------------------------------
# Check that ONNX_RUNTIME_VERSION is defined
# ------------------------------------------------------------
set(ONNX_RUNTIME_VERSION "1.23.2" CACHE STRING "ONNX Runtime C++ API version")

message(STATUS "Configuring ONNX Runtime C++ API (version ${ONNX_RUNTIME_VERSION})")

# ------------------------------------------------------------
# Select ONNX Runtime binary according to platform
# ------------------------------------------------------------
if (WIN32)
    set(ONNX_RUNTIME_PLATFORM "win-x64")
    set(ONNX_RUNTIME_ARCHIVE_EXT "zip")
    set(ONNX_RUNTIME_OS_DIR "Windows")
elseif (APPLE)
    set(ONNX_RUNTIME_PLATFORM "osx-x86_64")
    set(ONNX_RUNTIME_ARCHIVE_EXT "tgz")
    set(ONNX_RUNTIME_OS_DIR "Darwin")
else()
    set(ONNX_RUNTIME_PLATFORM "linux-x64")
    set(ONNX_RUNTIME_ARCHIVE_EXT "tgz")
    set(ONNX_RUNTIME_OS_DIR "Linux")
endif()

set(ONNX_RUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_RUNTIME_VERSION}/onnxruntime-${ONNX_RUNTIME_PLATFORM}-${ONNX_RUNTIME_VERSION}.${ONNX_RUNTIME_ARCHIVE_EXT}")

message(STATUS "ONNX Runtime download URL: ${ONNX_RUNTIME_URL}")


# Construimos la ruta: <Proyecto>/build/<SO>/_deps/onnxruntime
# CMAKE_SOURCE_DIR es la raíz de tu proyecto (donde está el CMakeLists.txt principal)
set(ONNX_RUNTIME_CUSTOM_INSTALL_DIR "${CMAKE_SOURCE_DIR}/build/${ONNX_RUNTIME_OS_DIR}/_deps/onnxruntime")


# ------------------------------------------------------------
# Fetch ONNX Runtime (download + extract into the build tree)
# ------------------------------------------------------------
FetchContent_Declare(
    onnxruntime
    URL ${ONNX_RUNTIME_URL}
    # Esto fuerza a que se descomprima EXACTAMENTE en esa ruta
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
    set(ONNX_RUNTIME_DLL ${ONNX_RUNTIME_LIB_DIR}/onnxruntime.dll)
    set(ONNX_RUNTIME_IMPLIB ${ONNX_RUNTIME_LIB_DIR}/onnxruntime.lib)

    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION "${ONNX_RUNTIME_DLL}"
        IMPORTED_IMPLIB  "${ONNX_RUNTIME_IMPLIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${ONNX_RUNTIME_INCLUDE_DIR}"
    )

    # ============================================================
    # AUTOMATIC DLL DEPLOYMENT (vcpkg-style)
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

else() # Linux
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION ${ONNX_RUNTIME_LIB_DIR}/libonnxruntime.so
        INTERFACE_INCLUDE_DIRECTORIES ${ONNX_RUNTIME_INCLUDE_DIR}
    )
endif()

message(STATUS "ONNX Runtime C++ API successfully configured")
