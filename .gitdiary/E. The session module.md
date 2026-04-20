Let’s build the `session` module.

Your specification states: *"IT will cache that api call in temp session data (which will be deleted once the app exits)."* The absolute cleanest way to fulfill this in C++ without dealing with orphan temporary files on the user's hard drive is to use an **in-memory cache**. Because the OS reclaims all memory allocated by a process the moment it terminates (even if it crashes), an in-memory approach guarantees 100% reliable cleanup with zero extra code.

Here is the implementation.

### 1. The Public Interface

We will define a `LogEntry` struct to hold the details of a single API call, and a `Cache` class to manage the history of the current session.

**Location: `include/session.hpp`**

```cpp
#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <vector>

namespace session {

    // Represents a single executed API call and its result
    struct LogEntry {
        std::string method;
        std::string url;
        std::string payload_file; // Empty if no body was sent
        std::string response;     // The raw output from HTTPie
    };

    // Manages the temporary session data
    class Cache {
    public:
        // Records a new API call into the session memory
        void add_entry(
            const std::string& method, 
            const std::string& url, 
            const std::string& payload_file, 
            const std::string& response
        );

        // Retrieves the entire history of the current session
        const std::vector<LogEntry>& get_history() const;

        // Retrieves a specific entry by index (useful for the UI later)
        const LogEntry* get_entry(size_t index) const;

        // Clears the current session cache manually
        void clear();

    private:
        std::vector<LogEntry> history_;
    };

}

#endif // SESSION_HPP
```

### 2. The Implementation

This is a straightforward data-management class.

**Location: `src/session/cache.cpp`** 

```cpp
#include "session.hpp"

namespace session {

    void Cache::add_entry(
        const std::string& method, 
        const std::string& url, 
        const std::string& payload_file, 
        const std::string& response
    ) {
        history_.push_back({
            method,
            url,
            payload_file,
            response
        });
    }

    const std::vector<LogEntry>& Cache::get_history() const {
        return history_;
    }

    const LogEntry* Cache::get_entry(size_t index) const {
        if (index < history_.size()) {
            return &history_[index];
        }
        return nullptr;
    }

    void Cache::clear() {
        history_.clear();
    }

}
```

### 3. The Local CMakeLists

**Location: `src/session/CMakeLists.txt`**

```makefile
add_library(session STATIC
    cache.cpp
)

target_include_directories(session PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
```

### 4. Wiring It Up in `main.cpp`

Let's integrate the `session` module into our test flow. We'll simulate making a request, caching it, and then reading back the session history just as the UI would when rendering a "History" tab.

**Location: `src/main.cpp`**

```cpp
#include "config.hpp"
#include "parser.hpp"
#include "http.hpp"
#include "session.hpp"
#include <iostream>
#include <cstdlib>

int main() {
	  std::cout << "Starting REST TUI Tester...\n" << std::endl;
																																																												  std::cout << "Starting REST TUI Tester...\n" << std::endl;
	  // Initialize our session cache
	  session::Cache app_session;
	    
	  // TODO: Replace this with the actual path to your C# WebAPI project root
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
	    for (const auto &ep : endpoints) {
	      std::cout << "[" << ep.method << "] " << ep.route << "\n";
	      std::cout << "    -> " << ep.file_path << "\n\n";
	    }
	  }
	
	  // --- Mocking the UI flow ---
	
	  // Imagine the user selected this route from the parser list:
	  std::string selected_method = "POST";
	  std::string selected_route = "/auth/login/";
	
	  // Imagine the TUI prompted the user for these variables:
	  std::map<std::string, std::string> route_params = {};
	  std::map<std::string, std::string> query_params = {};
	
	  // Path to a mock JSON file (create a dummy 'test.json' in your build folder)
	  std::string json_payload = "scratchpad.json"; // Set to "test.json" if you
	                                                // want to test the body upload
	
	  // 2. Build the final URL
	  std::string full_url = http::build_url(base_url.value(), selected_route,
	                                         route_params, query_params);
	
	  std::cout << "\n🚀 Executing: " << selected_method << " " << full_url << "\n";
	  std::cout << "--------------------------------------------------\n";
	
	  // 3. Execute the HTTP call via HTTPie
	  std::string response =
	      http::execute_request(selected_method, full_url, json_payload);
	
	  // 4. Print the result
	  std::cout << response << "\n";
	  std::cout << "--------------------------------------------------\n";
	
    // --- Cache the Request ---
   
    app_session.add_entry(selected_method, full_url, json_payload, response);
    std::cout << "✅ Request cached in session memory.\n";

    // Simulating a "History View" in the UI
    std::cout << "\n================ SESSION HISTORY ================\n";
    const auto& history = app_session.get_history();
    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << "[" << i + 1 << "] " << history[i].method << " " << history[i].url << "\n";
        // To avoid spamming the console, we could just print the first 50 chars of the response
        std::string short_resp = history[i].response.substr(0, 50);
        std::cout << "    Response: " << short_resp << "...\n";
    }
    std::cout << "=================================================\n";

    // Memory is automatically freed and session "deleted" when the program exits here.
    return EXIT_SUCCESS;
}
```

Don't forget to update your root `CMakeLists.txt`:

1. Add `add_subdirectory(src/session)`
2. Add `session` to your `target_link_libraries(rest_tui PRIVATE config parser session http ui)`

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
add_subdirectory(src/session)

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
target_link_libraries(api_explorer PRIVATE session)

# -----------------------------
# Include Directories
# -----------------------------
target_include_directories(api_explorer PRIVATE include)

```

With this, the entire backend engine of your application is completely functional. We are parsing your specific FastEndpoints setup, extracting URLs from configuration, executing via `httpie`, and managing state.
