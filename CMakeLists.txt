cmake_minimum_required(VERSION 3.15)

# -----------------------------
# Project Info
# -----------------------------
project(api_explorer VERSION 1.0 LANGUAGES CXX)

# -----------------------------
# C++ Standard
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Fetch External Dependencies ---
include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG main
)
FetchContent_MakeAvailable(ftxui)
# -----------------------------------

# -----------------------------
# Modules (Add subdirectories here)
# -----------------------------
add_subdirectory(src/config)
add_subdirectory(src/parser)
add_subdirectory(src/http)
add_subdirectory(src/session)
add_subdirectory(src/ui)

# -----------------------------
# Main Executable
# -----------------------------
add_executable(api_explorer src/main.cpp)

# -----------------------------
# Link Libraries
# -----------------------------
target_link_libraries(api_explorer PRIVATE config)
target_link_libraries(api_explorer PRIVATE parser)
target_link_libraries(api_explorer PRIVATE session)
target_link_libraries(api_explorer PRIVATE ui)

# -----------------------------
# Include Directories
# -----------------------------
target_include_directories(api_explorer PRIVATE include)
