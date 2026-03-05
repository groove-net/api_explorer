add_library(session STATIC
    cache.cpp
)

target_include_directories(session PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
