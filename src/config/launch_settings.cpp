#include "config.hpp"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace config {

std::optional<std::string> get_base_url(const std::string &project_root_path) {
  // Construct the expected path to the launch settings
  std::string file_path = project_root_path + "/Properties/launchSettings.json";

  std::ifstream file(file_path);
  if (!file.is_open()) {
    std::cerr << "[Config Error] Could not open: " << file_path << "\n";
    return std::nullopt;
  }

  // Read the entire file into a string
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  // Regex to find the "http" profile and extract its "applicationUrl"
  // It looks for: "http": { ... "applicationUrl": "MATCH_THIS" ... }
  // R"(...)" is a C++11 raw string literal, which saves us from escaping quotes
  // wildly.
  std::regex url_regex(
      R"REGEX("http"\s*:\s*\{[^}]*"applicationUrl"\s*:\s*"([^"]+)")REGEX");
  std::smatch match;

  if (std::regex_search(content, match, url_regex) && match.size() > 1) {
    // match[0] is the whole matched string, match[1] is the first capture group
    // (our URL)
    return match.str(1);
  }

  std::cerr << "[Config Error] Could not find 'http' profile applicationUrl in "
            << file_path << "\n";
  return std::nullopt;
}

} // namespace config
