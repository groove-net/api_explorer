add_library(http STATIC
    executor.cpp
)

target_include_directories(http PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
