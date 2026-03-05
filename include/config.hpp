#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <optional>
#include <string>

namespace config {
// Parses launchSettings.json to find the base HTTP applicationUrl.
// Returns std::nullopt if the file isn't found or the URL cannot be parsed.
std::optional<std::string> get_base_url(const std::string &project_root_path);
} // namespace config

#endif // CONFIG_HPP
