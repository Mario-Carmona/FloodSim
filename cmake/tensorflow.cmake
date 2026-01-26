# ============================================================
# TensorFlow C API integration using FetchContent
# ============================================================

include(FetchContent)

# ------------------------------------------------------------
# Check that TF_VERSION is defined
# ------------------------------------------------------------
set(TF_VERSION "2.10.0" CACHE STRING "TensorFlow C API version")

message(STATUS "Configuring TensorFlow C API (version ${TF_VERSION})")

# ------------------------------------------------------------
# Select TensorFlow binary according to platform
# ------------------------------------------------------------
if (WIN32)
    set(TF_PLATFORM "windows-x86_64")
    set(TF_ARCHIVE_EXT "zip")
    set(TF_OS_DIR "Windows")
elseif (APPLE)
    set(TF_PLATFORM "darwin-x86_64")
    set(TF_ARCHIVE_EXT "tar.gz")
    set(TF_OS_DIR "Darwin")
else()
    set(TF_PLATFORM "linux-x86_64")
    set(TF_ARCHIVE_EXT "tar.gz")
    set(TF_OS_DIR "Linux")
endif()

set(TF_URL "https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-cpu-${TF_PLATFORM}-${TF_VERSION}.${TF_ARCHIVE_EXT}")

message(STATUS "TensorFlow download URL: ${TF_URL}")


# Construimos la ruta: <Proyecto>/build/<SO>/_deps/tensorflow
# CMAKE_SOURCE_DIR es la raíz de tu proyecto (donde está el CMakeLists.txt principal)
set(TF_CUSTOM_INSTALL_DIR "${CMAKE_SOURCE_DIR}/build/${TF_OS_DIR}/_deps/tensorflow")


# ------------------------------------------------------------
# Fetch TensorFlow (download + extract into the build tree)
# ------------------------------------------------------------
FetchContent_Declare(
    tensorflow
    URL ${TF_URL}
    # Esto fuerza a que se descomprima EXACTAMENTE en esa ruta
    SOURCE_DIR "${TF_CUSTOM_INSTALL_DIR}"
    SUBBUILD_DIR "${TF_CUSTOM_INSTALL_DIR}-subbuild" 
)

message(STATUS "Fetching TensorFlow C API to: ${TF_CUSTOM_INSTALL_DIR}...")

FetchContent_MakeAvailable(tensorflow)

# ------------------------------------------------------------
# Expose TensorFlow include and library directories
# ------------------------------------------------------------
set(TF_INCLUDE_DIR ${TF_CUSTOM_INSTALL_DIR}/include)
set(TF_LIB_DIR     ${TF_CUSTOM_INSTALL_DIR}/lib)

message(STATUS "TensorFlow include directory: ${TF_INCLUDE_DIR}")
message(STATUS "TensorFlow library directory: ${TF_LIB_DIR}")

# ------------------------------------------------------------
# Define imported TensorFlow C API target
# ------------------------------------------------------------
add_library(tensorflow_c SHARED IMPORTED)

if (WIN32)
    set(TF_DLL ${TF_LIB_DIR}/tensorflow.dll)
    set(TF_IMPLIB ${TF_LIB_DIR}/tensorflow.lib)

    set_target_properties(tensorflow_c PROPERTIES
        IMPORTED_LOCATION "${TF_DLL}"
        IMPORTED_IMPLIB  "${TF_IMPLIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${TF_INCLUDE_DIR}"
    )

    # ============================================================
    # AUTOMATIC DLL DEPLOYMENT (vcpkg-style)
    # ============================================================
    # Overriding target_link_libraries to ensure that any EXECUTABLE 
    # linked with tensorflow_c receives a copy of the DLL in its output directory.
    function(target_link_libraries _target)
        _target_link_libraries(${_target} ${ARGN})

        if(TARGET ${_target})
            get_target_property(_type ${_target} TYPE)
            if(_type STREQUAL "EXECUTABLE")
                add_custom_command(TARGET ${_target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${TF_DLL}"
                        "$<TARGET_FILE_DIR:${_target}>"
                )
            endif()
        endif()
    endfunction()

elseif (APPLE)
    set_target_properties(tensorflow_c PROPERTIES
        IMPORTED_LOCATION ${TF_LIB_DIR}/libtensorflow.dylib
        INTERFACE_INCLUDE_DIRECTORIES ${TF_INCLUDE_DIR}
    )

else() # Linux
    set_target_properties(tensorflow_c PROPERTIES
        IMPORTED_LOCATION ${TF_LIB_DIR}/libtensorflow.so
        INTERFACE_INCLUDE_DIRECTORIES ${TF_INCLUDE_DIR}
    )
endif()

message(STATUS "TensorFlow C API successfully configured")
