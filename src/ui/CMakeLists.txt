add_library(ui STATIC
    views.cpp
)

target_include_directories(ui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)

# Link FTXUI components to our UI module
target_link_libraries(ui PRIVATE 
    ftxui::component
    ftxui::dom
    ftxui::screen
    http
    session
    Threads::Threads
)
