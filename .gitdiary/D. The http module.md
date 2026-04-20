Let’s build the `http` module.

Since your specification relies on `httpie` under the hood, this module's primary job is to act as a bridge between your C++ application and the system shell. We need it to safely construct the `http` command, execute it, and crucially—capture the standard output (stdout) so we can display it in the TUI and save it to our session cache later.

To capture the output of a system command in C++, we use `popen` (Pipe Open) instead of `std::system`. It opens a process by creating a pipe, forking, and invoking the shell, allowing us to read the output stream directly into a `std::string`.

Here is the implementation.

### 1. The Public Interface

We need a clean way to pass the route parameters and query strings, as well as a function to execute the final assembled command. `std::map` is perfect for handling the key-value pairs of our parameters.

**Location: `include/http.hpp`**

```cpp
#ifndef HTTP_HPP
#define HTTP_HPP

#include <string>
#include <map>

namespace http {

    // Helper to construct the final URL. 
    // Replaces {routeParams} and appends ?queryString=values
    std::string build_url(
        const std::string& base_url,
        const std::string& route_template,
        const std::map<std::string, std::string>& route_params = {},
        const std::map<std::string, std::string>& query_params = {}
    );

    // Executes the httpie command via system shell and captures the output.
    // Returns the raw console output (headers + body) from httpie.
    std::string execute_request(
        const std::string& method,
        const std::string& full_url,
        const std::string& body_file_path = ""
    );

}

#endif // HTTP_HPP
```

### 2. The Implementation

This file handles the string replacements for the URL and manages the pipe to the `httpie` process. I've included a platform check (`#ifdef _WIN32`) because Windows uses `_popen` while Unix-based systems (Linux/macOS) use standard `popen`.

**Location: `src/http/executor.cpp`**

```cpp
#include "http.hpp"
#include <cstdio>
#include <memory>
#include <array>
// #include <stdexcept>
// #include <iostream>

namespace http {

    std::string build_url(
        const std::string& base_url,
        const std::string& route_template,
        const std::map<std::string, std::string>& route_params,
        const std::map<std::string, std::string>& query_params
    ) {
        std::string final_url = base_url + route_template;

        // 1. Replace route parameters (e.g., {userId} -> 12345)
        for (const auto& [key, value] : route_params) {
            std::string token = "{" + key + "}";
            size_t pos = 0;
            while ((pos = final_url.find(token, pos)) != std::string::npos) {
                final_url.replace(pos, token.length(), value);
                pos += value.length(); // Move past the replaced segment
            }
        }

        // 2. Append query strings (e.g., ?sort=asc&limit=10)
        if (!query_params.empty()) {
            final_url += "?";
            bool first = true;
            for (const auto& [key, value] : query_params) {
                if (!first) final_url += "&";
                final_url += key + "=" + value;
                first = false;
            }
        }

        return final_url;
    }

    std::string execute_request(
        const std::string& method,
        const std::string& full_url,
        const std::string& body_file_path
    ) {
        // Construct the base command
        std::string command = "http --print=hb --pretty=format " + method + " \"" + full_url + "\"";

        // Append the JSON file payload if provided
        if (!body_file_path.empty()) {
            command += " < \"" + body_file_path + "\"";
        }

        // Redirect stderr to stdout so we capture error messages too
        command += " 2>&1";

        std::array<char, 256> buffer;
        std::string result;

        // Open the pipe
        #ifdef _WIN32
            std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
        #else
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        #endif

        if (!pipe) {
            return "[Error] Failed to open process pipe for httpie.";
        }

        // Read the output stream until the process finishes
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        return result;
    }

}
```

### 3. The Local CMakeLists

**Location: `src/http/CMakeLists.txt`** 

```makefile
add_library(http STATIC
    executor.cpp
)

target_include_directories(http PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
```

### 4. Wiring It Up in `main.cpp`

Let's test the entire pipeline end-to-end. We will mock a user selecting an endpoint, providing parameters, and firing off the request.

*(Make sure your local C# API is actually running before you test this, otherwise `httpie` will return a connection refused error!)*

**Location: `src/main.cpp`**

```cpp
#include "config.hpp"
#include "parser.hpp"
#include "http.hpp"
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

    // --- Mocking the UI flow ---
    
    // Imagine the user selected this route from the parser list:
    std::string selected_method = "POST";
    std::string selected_route = "/api/users/{userId}/update";
    
    // Imagine the TUI prompted the user for these variables:
    std::map<std::string, std::string> route_params = {
        {"userId", "9876"}
    };
    std::map<std::string, std::string> query_params = {
        {"force", "true"}
    };
    
    // Path to a mock JSON file (create a dummy 'test.json' in your build folder)
    std::string json_payload = ""; // Set to "test.json" if you want to test the body upload

    // 2. Build the final URL
    std::string full_url = http::build_url(base_url.value(), selected_route, route_params, query_params);
    
    std::cout << "\n🚀 Executing: " << selected_method << " " << full_url << "\n";
    std::cout << "--------------------------------------------------\n";

    // 3. Execute the HTTP call via HTTPie
    std::string response = http::execute_request(selected_method, full_url, json_payload);

    // 4. Print the result
    std::cout << response << "\n";
    std::cout << "--------------------------------------------------\n";

    return EXIT_SUCCESS;
}
```

Add `add_subdirectory(src/http)` and `target_link_libraries(my_app PRIVATE config parser http)` to your root `CMakeLists.txt`.

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
add_subdirectory(src/http)

# -----------------------------
# Main Executable
# -----------------------------
add_executable(api_explorer src/main.cpp)

# -----------------------------
# Link Libraries
# -----------------------------
target_link_libraries(api_explorer PRIVATE config)
target_link_libraries(api_explorer PRIVATE parser)
target_link_libraries(api_explorer PRIVATE http)

# -----------------------------
# Include Directories
# -----------------------------
target_include_directories(api_explorer PRIVATE include)

```

Compile it and give it a run. If your web server is running, you should see the beautifully formatted `httpie` output directly in your terminal.
