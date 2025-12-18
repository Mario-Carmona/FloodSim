# ============================================================
# TensorFlow C API integration using FetchContent
# ============================================================

include(FetchContent)

# ------------------------------------------------------------
# Check that TF_VERSION is defined
# ------------------------------------------------------------
set(TF_VERSION "2.6.0" CACHE STRING "TensorFlow C API version")

message(STATUS "Configuring TensorFlow C API (version ${TF_VERSION})")

# ------------------------------------------------------------
# Select TensorFlow binary according to platform
# ------------------------------------------------------------
if (WIN32)
    set(TF_PLATFORM "windows-x86_64")
    set(TF_ARCHIVE_EXT "zip")
elseif (APPLE)
    set(TF_PLATFORM "darwin-x86_64")
    set(TF_ARCHIVE_EXT "tar.gz")
else()
    set(TF_PLATFORM "linux-x86_64")
    set(TF_ARCHIVE_EXT "tar.gz")
endif()

set(TF_URL "https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-cpu-${TF_PLATFORM}-${TF_VERSION}.${TF_ARCHIVE_EXT}")

message(STATUS "TensorFlow download URL: ${TF_URL}")

# ------------------------------------------------------------
# Fetch TensorFlow (download + extract into the build tree)
# ------------------------------------------------------------
FetchContent_Declare(
    tensorflow
    URL ${TF_URL}
)

message(STATUS "Fetching TensorFlow C API...")

FetchContent_MakeAvailable(tensorflow)

# ------------------------------------------------------------
# Expose TensorFlow include and library directories
# ------------------------------------------------------------
set(TF_INCLUDE_DIR ${tensorflow_SOURCE_DIR}/include)
set(TF_LIB_DIR     ${tensorflow_SOURCE_DIR}/lib)

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
        IMPORTED_LOCATION ${TF_DLL}
        IMPORTED_IMPLIB  ${TF_IMPLIB}
        INTERFACE_INCLUDE_DIRECTORIES ${TF_INCLUDE_DIR}
    )

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
