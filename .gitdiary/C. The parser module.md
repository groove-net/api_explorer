Let’s build the `parser` module.

Since you are strictly using C++17, we have access to the `<filesystem>` library. This makes recursive directory traversal completely painless and standard across all operating systems.

Coupled with `<regex>`, we can quickly scan for your specific FastEndpoints file naming conventions (`GET.cs`, `POST.cs`) and extract the route strings right out of the `Configure()` method.

Here is the implementation.

### 1. The Public Interface

We need a clear data structure to hold the parsed endpoint information so the `ui` module can render it later.

**Location: `include/parser.hpp`**

```cpp
#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

namespace parser {

    // Represents a single discovered FastEndpoint
    struct EndpointInfo {
        std::string method;    // "GET", "POST", etc.
        std::string route;     // "/auth/2fa/confirm" or "/api/{MyString}"
        std::string file_path; // Absolute or relative path to the .cs file
    };

    // Recursively scans the directory for HTTP verb .cs files and extracts routes
    std::vector<EndpointInfo> parse_endpoints(const std::string& endpoints_dir);

}

#endif // PARSER_HPP
```

### 2. The Implementation

This handles the file I/O and regex matching. It targets your specific C# structure, looking only for files named after HTTP methods and then scanning their contents for the route definitions.

**Location: `src/parser/parser.cpp`**

```cpp
#include "parser.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>
#include <cctype>

namespace fs = std::filesystem;

namespace parser {

    std::vector<EndpointInfo> parse_endpoints(const std::string& endpoints_dir) {
        std::vector<EndpointInfo> endpoints;

        if (!fs::exists(endpoints_dir) || !fs::is_directory(endpoints_dir)) {
            std::cerr << "[Parser Error] Invalid Endpoints directory: " << endpoints_dir << "\n";
            return endpoints;
        }

        // Regex 1: Match files named strictly as HTTP verbs (case-insensitive just in case)
        std::regex file_name_regex("^(GET|POST|PUT|DELETE|PATCH)\\.cs$", std::regex_constants::icase);

        // Regex 2: Extract the route string from the method call. 
        // Looks for: Post("/my/route") or Get("/api/{param}")
        // Group 1 captures the method, Group 2 captures the route string inside the quotes
			  std::regex route_regex(
			      R"REGEX((Get|Post|Put|Delete|Patch)\s*\(\s*"([^"]+)"\s*\))REGEX",
			      std::regex_constants::icase);

        for (const auto& entry : fs::recursive_directory_iterator(endpoints_dir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::smatch file_match;

                // Check if the file is an HTTP verb file
                if (std::regex_match(filename, file_match, file_name_regex)) {
                    std::string method = file_match[1].str();
                    
                    // Standardize the method name to uppercase
                    for (auto & c : method) c = toupper(c);

                    std::ifstream file(entry.path());
                    if (!file.is_open()) continue;

                    // Read the entire file
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    std::string content = buffer.str();

                    std::smatch route_match;
                    // Search for the route definition inside the Configure() method space
                    if (std::regex_search(content, route_match, route_regex) && route_match.size() > 2) {
                        EndpointInfo info;
                        info.method = method;
                        info.route = route_match[2].str();
                        info.file_path = entry.path().string();
                        
                        endpoints.push_back(info);
                    }
                }
            }
        }

        return endpoints;
    }

}
```

### 3. The Local CMakeLists

**Location: `src/parser/CMakeLists.txt`**

```makefile
add_library(parser STATIC
    parser.cpp
)

target_include_directories(parser PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
```

### 4. Wiring It All Together in `main.cpp`

Let's test both the `config` and `parser` modules. If you point this at the `Endpoints` folder of your C# authentication service, it will map out the routing tree instantly.

**Location: `src/main.cpp`**

```cpp
#include "config.hpp"
#include "parser.hpp"
#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "Starting REST TUI Tester...\n" << std::endl;

    // TODO: Update these paths for your local environment
    std::string project_root = "."; 
    std::string endpoints_dir = project_root + "/Endpoints";

    // 1. Get Base URL
    auto base_url = config::get_base_url(project_root);
    if (base_url.has_value()) {
        std::cout << "🌐 Base URL: " << base_url.value() << "\n";
    } else {
        std::cout << "⚠️ Could not find base URL. Check launchSettings.json.\n";
    }

    std::cout << "--------------------------------------------------\n";

    // 2. Parse Endpoints
    std::cout << "🔍 Scanning for FastEndpoints in: " << endpoints_dir << "\n";
    auto endpoints = parser::parse_endpoints(endpoints_dir);

    if (endpoints.empty()) {
        std::cout << "⚠️  No endpoints found.\n";
    } else {
        std::cout << "✅ Found " << endpoints.size() << " endpoints:\n\n";
        for (const auto& ep : endpoints) {
            std::cout << "[" << ep.method << "] " << ep.route << "\n";
            std::cout << "    -> " << ep.file_path << "\n\n";
        }
    }

    return EXIT_SUCCESS;
}
```

Make sure to add `add_subdirectory(src/parser)` and link `parser` in your root `CMakeLists.txt`.

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

# -----------------------------
# Modules (Add subdirectories here)
# -----------------------------
add_subdirectory(src/config)
add_subdirectory(src/parser)

# -----------------------------
# Main Executable
# -----------------------------
add_executable(api_explorer src/main.cpp)

# -----------------------------
# Link Libraries
# -----------------------------
target_link_libraries(api_explorer PRIVATE config)
target_link_libraries(api_explorer PRIVATE parser)

# -----------------------------
# Include Directories
# -----------------------------
target_include_directories(api_explorer PRIVATE include)

```

With this foundation, the application successfully reads your project configuration and maps out the domain routes.
