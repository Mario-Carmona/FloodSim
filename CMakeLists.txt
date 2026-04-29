
cmake_minimum_required(VERSION 4.1.1)


if(BUILD_APP)
    set(ENABLE_INPUT_ADAPTERS ON)
    set(ENABLE_OUTPUT_ADAPTERS ON)
    set(ENABLE_STATE_UPDATER_ADAPTERS ON)
    set(ENABLE_APP ON)
    set(ENABLE_CORE ON)
endif()

if(BUILD_ALL_TESTS)
    set(ENABLE_STATE_UPDATER_ADAPTERS ON)
    set(ENABLE_CORE ON)
endif()


set(VCPKG_MANIFEST_FEATURES "")

if(ENABLE_INPUT_ADAPTERS)
    list(APPEND VCPKG_MANIFEST_FEATURES "input-adapters")
endif()

if(ENABLE_OUTPUT_ADAPTERS)
    list(APPEND VCPKG_MANIFEST_FEATURES "output-adapters")
endif()

if(ENABLE_APP)
    list(APPEND VCPKG_MANIFEST_FEATURES "app")
endif()

if(ENABLE_CORE)
    list(APPEND VCPKG_MANIFEST_FEATURES "app-core")
endif()

if(BUILD_APP)
    if(ENABLE_GUI)
        list(APPEND VCPKG_MANIFEST_FEATURES "gui")
    endif()
endif()

if(BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "test")
endif()



project(DanaSim VERSION 1.0.0 LANGUAGES CXX)


if(BUILD_APP OR BUILD_TESTS)
    add_subdirectory(src)
endif()

if(BUILD_TESTS)
    enable_testing()

    add_subdirectory(tests)
endif()

if(BUILD_PYTHON_ENVS)
    add_subdirectory(python)
endif()
