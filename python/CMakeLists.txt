
# =========================================================
# 🐍 Python Integration & Environment Configuration
# =========================================================

# ---------------------------------------------------------
# 🛠️ Build Tooling & Macro Resolution
# ---------------------------------------------------------
# Inject custom CMake utilities and macros responsible for Python 
# lifecycle management and environment bootstrapping.
include(${CMAKE_SOURCE_DIR}/cmake/python.cmake)

# ---------------------------------------------------------
# 🌐 Virtual Environment Provisioning
# ---------------------------------------------------------
# Provision the local Python runtime environment. Conditionally enforces 
# strict dependency isolation (e.g., venv creation) based on global build flags.
create_python_env(${PYTHON_SUBDIR_NAME} ${ISOLATE_ENVIRONMENT})

# ---------------------------------------------------------
# 📂 Internal Module Hierarchy Traversals
# ---------------------------------------------------------
# Dynamically register the underlying Python source or bindings directory
add_subdirectory(${PYTHON_SUBDIR_NAME})
