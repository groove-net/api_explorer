#include "http.hpp"
#include <array>
#include <cstdio>
#include <memory>

namespace http {

std::string build_url(const std::string &base_url,
                      const std::string &route_template,
                      const std::map<std::string, std::string> &route_params,
                      const std::map<std::string, std::string> &query_params) {
  std::string final_url = base_url + route_template;

  // 1. Replace route parameters (e.g., {userId} -> 12345)
  for (const auto &[key, value] : route_params) {
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
    for (const auto &[key, value] : query_params) {
      if (!first)
        final_url += "&";
      final_url += key + "=" + value;
      first = false;
    }
  }

  return final_url;
}

std::string execute_request(const std::string &method,
                            const std::string &full_url,
                            const std::string &body_file_path) {
  // Construct the base command
  std::string command =
      "http --print=hb --pretty=format " + method + " \"" + full_url + "\"";

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
  std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"),
                                                 _pclose);
#else
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"),
                                                pclose);
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

} // namespace http
