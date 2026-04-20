### 📁 Proposed Project Layout

Let's break the application down into distinct, focused modules.

```
your_tui_project/
├── build/
├── include/
│   ├── config.hpp      # Public API for configuration & settings
│   ├── parser.hpp      # Public API for parsing C# and JSON files
│   ├── session.hpp     # Public API for caching and state
│   ├── http.hpp        # Public API for the HTTPie wrapper
│   └── ui.hpp          # Public API for the TUI
├── lib/                # (External TUI library will go here eventually)
├── src/
│   ├── config/         # Reads launchSettings.json for the base URL
│   │   ├── CMakeLists.txt
│   │   └── launch_settings.cpp
│   ├── parser/         # Scans the Endpoints folder and extracts routes
│   │   ├── CMakeLists.txt
│   │   ├── internal_parser.hpp
│   │   ├── directory_scanner.cpp
│   │   └── route_extractor.cpp
│   ├── session/        # Manages the temporary cache of API calls
│   │   ├── CMakeLists.txt
│   │   └── cache.cpp
│   ├── http/           # Constructs and executes httpie system calls
│   │   ├── CMakeLists.txt
│   │   └── executor.cpp
│   ├── ui/             # Handles the terminal interface and user input
│   │   ├── CMakeLists.txt
│   │   ├── internal_ui.hpp
│   │   ├── views.cpp
│   │   └── input_handlers.cpp
│   └── main.cpp        # The orchestrator
├── .gitignore
├── CMakeLists.txt
└── README.md
```

### 🧩 Module Breakdown & Responsibilities

This architecture will keep the codebase incredibly clean as the tool scales. For example, when you point the `parser` module at the `AuthService` directory, it can independently extract the routing data and pass it back to `main.cpp`, completely agnostic of how the `ui` module decides to render it.

- **`config` Module:** * Responsible for finding and parsing `Properties/launchSettings.json`.
    - Extracts the `profiles -> http -> applicationUrl` to establish the base URL.
- **`parser` Module:** * Recursively traverses the target directory looking for `GET.cs`, `POST.cs`, etc.
    - Reads the file contents, locates the `Configure()` method, and extracts the exact string passed to the HTTP verb methods (e.g., `/auth/2fa/confirm` or `/api/{MyString}`).
- **`ui` Module:** * Renders the interactive list of parsed endpoints.
    - Prompts the user to fill in any discovered `{routeParameters}` and optional query strings.
    - Handles the file picker logic for selecting the JSON payload (e.g., `< scratchpad.json`).
- **`http` Module:** * Takes the base URL, the formatted endpoint, the parameters, and the payload file path.
    - Safely constructs the `httpie` shell command.
    - Executes the command via a system call (like `std::system` or `popen` to capture the output).
- **`session` Module:** * Logs the executed commands and their responses to a temporary, in-memory structure or a temp file.
    - Ensures automatic cleanup when the `main.cpp` hits `std::exit(EXIT_SUCCESS)`.

---

### The Root Orchestrator (`CMakeLists.txt`)

Following your guidelines, your root CMake file will elegantly tie these domains together:

```makefile
cmake_minimum_required(VERSION 3.15)
project(RestTuiTester VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add all sub-modules
add_subdirectory(src/config)
add_subdirectory(src/parser)
add_subdirectory(src/session)
add_subdirectory(src/http)
add_subdirectory(src/ui)

# Define main executable
add_executable(rest_tui src/main.cpp)

# Link modules to the executable
target_link_libraries(rest_tui PRIVATE config parser session http ui)

# Include public headers
target_include_directories(rest_tui PRIVATE include)
```

This setup completely aligns with your design philosophy: strict separation of concerns, high modularity, and rapid, isolated compilation.
