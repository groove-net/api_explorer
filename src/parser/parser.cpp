#include "parser.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace parser {

std::vector<EndpointInfo> parse_endpoints(const std::string &endpoints_dir) {
  std::vector<EndpointInfo> endpoints;

  if (!fs::exists(endpoints_dir) || !fs::is_directory(endpoints_dir)) {
    std::cerr << "[Parser Error] Invalid Endpoints directory: " << endpoints_dir
              << "\n";
    return endpoints;
  }

  // Regex 1: Match files named strictly as HTTP verbs (case-insensitive just in
  // case)
  std::regex file_name_regex("^(GET|POST|PUT|DELETE|PATCH)\\.cs$",
                             std::regex_constants::icase);

  // Regex 2: Extract the route string from the method call.
  // Looks for: Post("/my/route") or Get("/api/{param}")
  // Group 1 captures the method, Group 2 captures the route string inside the
  // quotes
  std::regex route_regex(
      R"REGEX((Get|Post|Put|Delete|Patch)\s*\(\s*"([^"]+)"\s*\))REGEX",
      std::regex_constants::icase);

  for (const auto &entry : fs::recursive_directory_iterator(endpoints_dir)) {
    if (entry.is_regular_file()) {
      std::string filename = entry.path().filename().string();
      std::smatch file_match;

      // Check if the file is an HTTP verb file
      if (std::regex_match(filename, file_match, file_name_regex)) {
        std::string method = file_match[1].str();

        // Standardize the method name to uppercase
        for (auto &c : method)
          c = toupper(c);

        std::ifstream file(entry.path());
        if (!file.is_open())
          continue;

        // Read the entire file
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        std::smatch route_match;
        // Search for the route definition inside the Configure() method
        // space
        if (std::regex_search(content, route_match, route_regex) &&
            route_match.size() > 2) {
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

} // namespace parser
