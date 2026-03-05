/*
 ============================================================================
 Name        : api_explorer
 Author      : Gabriel Adelemoni
 Version     : 1.0
 Description : A simple C++ application demonstrating project structure with
               src, include, CMake-based build system, and modular design.
 License     : MIT
 ============================================================================
*/

#include "config.hpp"
#include "parser.hpp"
#include "session.hpp"
#include "ui.hpp"
#include <cstdlib>
#include <iostream>

int main() {
  std::string project_root = ".";
  std::string endpoints_dir = project_root + "/Endpoints";

  auto base_url = config::get_base_url(project_root);
  if (!base_url.has_value()) {
    std::cerr << "⚠️ Could not find base URL in launchSettings.json.\n";
    return EXIT_FAILURE;
  }

  auto endpoints = parser::parse_endpoints(endpoints_dir);
  if (endpoints.empty()) {
    std::cerr << "⚠️ No endpoints found in " << endpoints_dir << "\n";
    return EXIT_FAILURE;
  }

  session::Cache app_session;

  // Hand control over to the FTXUI rendering loop
  ui::render(endpoints, base_url.value(), app_session);

  return EXIT_SUCCESS;
}
