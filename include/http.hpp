#ifndef HTTP_HPP
#define HTTP_HPP

#include <map>
#include <string>

namespace http {

// Helper to construct the final URL.
// Replaces {routeParams} and appends ?queryString=values
std::string
build_url(const std::string &base_url, const std::string &route_template,
          const std::map<std::string, std::string> &route_params = {},
          const std::map<std::string, std::string> &query_params = {});

// Executes the httpie command via system shell and captures the output.
// Returns the raw console output (headers + body) from httpie.
std::string execute_request(const std::string &method,
                            const std::string &full_url,
                            const std::string &body_file_path = "");

} // namespace http

#endif // HTTP_HPP
