#ifndef UI_HPP
#define UI_HPP

#include "parser.hpp"
#include "session.hpp"
#include <string>
#include <vector>

namespace ui {

// Launches the interactive terminal user interface
void render(const std::vector<parser::EndpointInfo> &endpoints,
            const std::string &base_url, session::Cache &app_session);

} // namespace ui

#endif // UI_HPP
