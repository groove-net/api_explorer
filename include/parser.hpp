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
std::vector<EndpointInfo> parse_endpoints(const std::string &endpoints_dir);

} // namespace parser

#endif // PARSER_HPP
