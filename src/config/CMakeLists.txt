# Define the config library
add_library(config STATIC
    launch_settings.cpp
)

# Expose the include directory so main.cpp (and others) can find config.hpp
target_include_directories(config PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
