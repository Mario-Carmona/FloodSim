
include(${CMAKE_SOURCE_DIR}/cmake/python.cmake)

create_python_env(${PYTHON_SUBDIR_NAME} ${ISOLATE_ENVIRONMENT})

add_subdirectory(${PYTHON_SUBDIR_NAME})
