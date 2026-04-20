FTXUI is a phenomenal choice. It is modern, declarative, and heavily relies on C++17 features, fitting perfectly into your established architecture. Because it uses a component tree model (similar to React or Flutter), you can build complex layouts by nesting simple elements.

To integrate an external library like FTXUI cleanly without manually downloading binaries into your `lib/` folder, we will use CMake's `FetchContent` module. This instructs CMake to pull the library directly from GitHub during the build process and link it seamlessly.

Here is how we build the `ui` module.

### 1. Updating the Root Orchestrator

Before writing the UI code, we need to tell CMake to fetch FTXUI. Update your root `CMakeLists.txt` to include the `FetchContent` block right before you add your subdirectories.

**Location: `CMakeLists.txt` (Root)**

```makefile
cmake_minimum_required(VERSION 3.15)
project(RestTuiTester VERSION 1.0 LANGUAGES CXX)

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

# Add your sub-modules
add_subdirectory(src/config)
add_subdirectory(src/parser)
add_subdirectory(src/session)
add_subdirectory(src/http)
add_subdirectory(src/ui)

add_executable(rest_tui src/main.cpp)

# Link modules to the executable
target_link_libraries(api_explorer PRIVATE config)
target_link_libraries(api_explorer PRIVATE parser)
target_link_libraries(api_explorer PRIVATE http)
target_link_libraries(api_explorer PRIVATE session)
target_link_libraries(api_explorer PRIVATE ui)

target_include_directories(rest_tui PRIVATE include)
```

### 2. The Public Interface

We need a simple entry point to hand control over to the UI event loop. We will pass in the discovered endpoints, the base URL, and our session cache.

**Location: `include/ui.hpp`**

```cpp
#ifndef UI_HPP
#define UI_HPP

#include "parser.hpp"
#include "session.hpp"
#include <string>
#include <vector>

namespace ui {

    // Launches the interactive terminal user interface
    void render(
        const std::vector<parser::EndpointInfo>& endpoints, 
        const std::string& base_url, 
        session::Cache& app_session
    );

}

#endif // UI_HPP
```

### 3. The Implementation

FTXUI builds interfaces using `Components` (for interaction like inputs and menus) and `Elements` (for layout and drawing like borders and text).

This implementation creates a classic two-pane layout: a sidebar menu displaying your parsed C# endpoints (like `/auth/2fa/confirm`), and a main content area for parameter input and response display.

**Location: `src/ui/views.cpp`**

```cpp
#include "http.hpp"
#include "ui.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <thread>

using namespace ftxui;

namespace ui {

void render(const std::vector<parser::EndpointInfo> &endpoints,
            const std::string &base_url, session::Cache &app_session) {
  auto screen = ScreenInteractive::Fullscreen();

  // 1. Data for Endpoints Menu
  std::vector<std::string> menu_entries;
  for (const auto &ep : endpoints) {
    menu_entries.push_back("[" + ep.method + "] " + ep.route);
  }
  int selected_endpoint = 0;

  // 2. Data for Tabs
  int tab_index = 0;

  // 3. State variables for Explorer Tab
  bool use_json_payload = false;
  std::string response_text = "Select an endpoint and press Execute.";

  // 4. State variables for History Tab
  int history_selected = 0;
  std::vector<std::string> history_menu_entries;
  std::vector<int> history_endpoint_map;
  std::vector<int> history_session_map;

  // --- THE EXECUTION LOGIC ---
  auto do_request = [&] {
    if (endpoints.empty())
      return;

    const auto &ep = endpoints[selected_endpoint];
    std::map<std::string, std::string> empty_params;
    std::string full_url =
        http::build_url(base_url, ep.route, empty_params, empty_params);

    response_text = "Executing HTTPie call...\n";

    std::string method_copy = ep.method;
    std::string body_file_copy = use_json_payload ? "payload.json" : "";
    int current_selected = selected_endpoint;

    std::thread([&, method_copy, full_url, body_file_copy, current_selected]() {
      std::string result =
          http::execute_request(method_copy, full_url, body_file_copy);

      screen.Post([&, method_copy, full_url, body_file_copy, result,
                   current_selected]() {
        response_text = result;
        app_session.add_entry(method_copy, full_url, body_file_copy, result);

        int session_idx = app_session.get_history().size() - 1;

        history_menu_entries.insert(history_menu_entries.begin(),
                                    "[" + method_copy + "] " + full_url);
        history_endpoint_map.insert(history_endpoint_map.begin(),
                                    current_selected);
        history_session_map.insert(history_session_map.begin(), session_idx);

        history_selected = 0;
      });
      screen.PostEvent(Event::Custom);
    }).detach();
  };

  // --- COLOR TRANSFORMATION HELPER ---
  // This lambda parses the "[METHOD] /route" string and applies FTXUI colors
  auto colorize_menu_entry = [](const EntryState &state) {
    std::string label = state.label;
    size_t end_bracket = label.find(']');
    Element e;

    if (end_bracket != std::string::npos && label[0] == '[') {
      std::string method = label.substr(1, end_bracket - 1);
      std::string rest = label.substr(end_bracket + 1);

      ftxui::Color c = Color::White;
      if (method == "GET")
        c = Color::Cyan;
      else if (method == "POST")
        c = Color::Green;
      else if (method == "PUT" || method == "PATCH")
        c = Color::Yellow;
      else if (method == "DELETE")
        c = Color::Red;

      e = hbox({text("[" + method + "]") | color(c) | bold, text(rest)});
    } else {
      e = text(label);
    }

    // Maintain standard selection styling
    if (state.focused)
      e = e | inverted;
    if (state.active)
      e = e | bold;
    return e;
  };

  // 5. UI Components: Explorer
  MenuOption main_menu_option;
  main_menu_option.entries_option.transform = colorize_menu_entry;
  auto menu = Menu(&menu_entries, &selected_endpoint, main_menu_option);

  auto file_toggle = Checkbox("Use payload.json", &use_json_payload);

  // Added a custom animated style to the button for extra pop
  ButtonOption btn_option = ButtonOption::Animated(Color::Green);
  auto execute_button = Button("Execute via HTTPie", do_request, btn_option);

  auto explorer_container = Container::Vertical({file_toggle, execute_button});

  // 6. UI Components: History
  MenuOption history_option;
  history_option.entries_option.transform =
      colorize_menu_entry; // Reuse color logic!
  history_option.on_enter = [&] {
    if (history_menu_entries.empty())
      return;

    selected_endpoint = history_endpoint_map[history_selected];

    int session_idx = history_session_map[history_selected];
    const auto *entry = app_session.get_entry(session_idx);
    if (entry) {
      use_json_payload = (!entry->payload_file.empty());
    }

    tab_index = 0;
    do_request();
  };
  auto history_menu =
      Menu(&history_menu_entries, &history_selected, history_option);

  // 7. Component Composition
  auto history_container = Container::Vertical({history_menu});

  auto tab_container =
      Container::Tab({explorer_container, history_container}, &tab_index);

  auto main_interactive = Container::Horizontal({menu, tab_container});
  auto root_container = main_interactive;

  // 8. Event Handler for global shortcuts
  auto event_handler = CatchEvent(root_container, [&](Event e) {
    if (e == Event::Character('1')) {
      tab_index = 0;
      return true;
    }
    if (e == Event::Character('2')) {
      tab_index = 1;
      return true;
    }
    if (e == Event::Character('q') || e == Event::Character('Q')) {
      screen.Exit();
      return true;
    }
    return false;
  });

  // 9. The Renderer
  auto renderer = Renderer(event_handler, [&] {
    Element explorer_view = vbox(
        {text(""),
         hbox({text("  URL: ") | color(Color::CyanLight),
               text(base_url + (endpoints.empty()
                                    ? ""
                                    : endpoints[selected_endpoint].route)) |
                   bold}),
         separator(), text(""),
         hbox({text("  JSON Body: ") | color(Color::CyanLight),
               file_toggle->Render()}),
         text(""), separator(), text(""), execute_button->Render() | center,
         text(""), separator(),
         text("  Response:") | bold | color(Color::Yellow), text(""),
         paragraph(response_text) | flex});

    Element history_view;
    if (history_menu_entries.empty()) {
      history_view =
          text("No requests made in this session yet.") | dim | center;
    } else {
      int session_idx = history_session_map[history_selected];
      const auto *entry = app_session.get_entry(session_idx);

      history_view =
          vbox({text("  Past Requests (Press Enter to recall & execute):") |
                    bold | color(Color::CyanLight),
                text(""),
                history_menu->Render() | vscroll_indicator | frame |
                    size(HEIGHT, EQUAL, 5),
                separator(),
                text("  Historical Response:") | bold | color(Color::Yellow),
                text(""), paragraph(entry ? entry->response : "") | flex});
    }

    auto header =
        hbox({filler(),
              text(" Shortcuts: [1] Explorer  [2] History  [q] Quit ") | dim});

    // Custom visual tab indicator
    Element explorer_tab = text(" EXPLORER ");
    explorer_tab = (tab_index == 0) ? (explorer_tab | bold | inverted)
                                    : (explorer_tab | dim);

    Element history_tab = text(" HISTORY ");
    history_tab = (tab_index == 1) ? (history_tab | bold | inverted)
                                   : (history_tab | dim);

    Element tab_indicator = hbox({explorer_tab, history_tab});
    Element active_view = (tab_index == 0) ? explorer_view : history_view;

    // Added BlueLight coloring to the panel borders
    auto right_panel =
        window(tab_indicator, active_view) | flex | color(Color::BlueLight);

    auto left_panel = window(text(" Endpoints ") | bold | color(Color::Cyan),
                             menu->Render() | vscroll_indicator | frame) |
                      size(WIDTH, EQUAL, 40) | color(Color::BlueLight);

    return vbox({header, hbox({left_panel, right_panel}) | flex});
  });

  screen.Loop(renderer);
}

} // namespace ui
```

### 4. The Local CMakeLists

We must link the specific FTXUI libraries that handle the DOM (document object model), Components (interaction), and the Screen (rendering loop).

**Location: `src/ui/CMakeLists.txt`**

```makefile
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
```

### 5. Final Wiring in `main.cpp`

Now we replace our mocked flow with the actual TUI launch.

**Location: `src/main.cpp`** 

```cpp
#include "config.hpp"
#include "parser.hpp"
#include "session.hpp"
#include "ui.hpp"
#include <iostream>
#include <cstdlib>

int main() {
    std::string project_root = "."; 
    std::string endpoints_dir = project_root + "/Endpoints";

    auto base_url = config::get_base_url(project_root);
    if (!base_url.has_value()) {
        std::cerr << "⚠️ Could not find base URL in launchSettings.json.\n";
        return EXIT_FAILURE;
    }

    auto endpoints = parser::parse_endpoints(endpoints_dir);
    if (endpoints.empty()) {
        std::cerr << "⚠️ No endpoints found in " << endpoints_dir << "\n";
        return EXIT_FAILURE;
    }

    session::Cache app_session;

    // Hand control over to the FTXUI rendering loop
    ui::render(endpoints, base_url.value(), app_session);

    return EXIT_SUCCESS;
}
```

When you compile and run this, you will have a fully functional terminal interface. You can navigate the parsed endpoints with your arrow keys, type a filename for the JSON payload, and hit enter on the Execute button to fire off the `httpie` call.

**Remove http from main**

```makefile
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
```
