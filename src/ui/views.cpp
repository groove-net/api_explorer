#include "http.hpp"
#include "ui.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <thread>

using namespace ftxui;

namespace ui {

void render(const std::vector<parser::EndpointInfo> &endpoints,
            const std::string &base_url, session::Cache &app_session) {
  auto screen = ScreenInteractive::Fullscreen();

  // 1. Data for Endpoints Menu
  std::vector<std::string> menu_entries;
  for (const auto &ep : endpoints) {
    menu_entries.push_back("[" + ep.method + "] " + ep.route);
  }
  int selected_endpoint = 0;

  // 2. Data for Tabs
  int tab_index = 0;

  // 3. State variables for Explorer Tab
  std::string json_file_path = "";
  std::string response_text = "Select an endpoint and press Execute.";

  // 4. State variables for History Tab
  int history_selected = 0;
  std::vector<std::string> history_menu_entries;
  std::vector<int> history_endpoint_map;
  std::vector<int> history_session_map;

  // --- THE EXECUTION LOGIC ---
  auto do_request = [&] {
    if (endpoints.empty())
      return;

    const auto &ep = endpoints[selected_endpoint];
    std::map<std::string, std::string> empty_params;
    std::string full_url =
        http::build_url(base_url, ep.route, empty_params, empty_params);

    response_text = "Executing HTTPie call...\n";

    std::string method_copy = ep.method;
    std::string body_file_copy = json_file_path;
    int current_selected = selected_endpoint;

    std::thread([&, method_copy, full_url, body_file_copy, current_selected]() {
      std::string result =
          http::execute_request(method_copy, full_url, body_file_copy);

      screen.Post([&, method_copy, full_url, body_file_copy, result,
                   current_selected]() {
        response_text = result;
        app_session.add_entry(method_copy, full_url, body_file_copy, result);

        int session_idx = app_session.get_history().size() - 1;

        history_menu_entries.insert(history_menu_entries.begin(),
                                    "[" + method_copy + "] " + full_url);
        history_endpoint_map.insert(history_endpoint_map.begin(),
                                    current_selected);
        history_session_map.insert(history_session_map.begin(), session_idx);

        history_selected = 0;
      });
      screen.PostEvent(Event::Custom);
    }).detach();
  };

  // --- COLOR TRANSFORMATION HELPER ---
  // This lambda parses the "[METHOD] /route" string and applies FTXUI colors
  auto colorize_menu_entry = [](const EntryState &state) {
    std::string label = state.label;
    size_t end_bracket = label.find(']');
    Element e;

    if (end_bracket != std::string::npos && label[0] == '[') {
      std::string method = label.substr(1, end_bracket - 1);
      std::string rest = label.substr(end_bracket + 1);

      ftxui::Color c = Color::White;
      if (method == "GET")
        c = Color::Cyan;
      else if (method == "POST")
        c = Color::Green;
      else if (method == "PUT" || method == "PATCH")
        c = Color::Yellow;
      else if (method == "DELETE")
        c = Color::Red;

      e = hbox({text("[" + method + "]") | color(c) | bold, text(rest)});
    } else {
      e = text(label);
    }

    // Maintain standard selection styling
    if (state.focused)
      e = e | inverted;
    if (state.active)
      e = e | bold;
    return e;
  };

  // 5. UI Components: Explorer
  MenuOption main_menu_option;
  main_menu_option.entries_option.transform = colorize_menu_entry;
  auto menu = Menu(&menu_entries, &selected_endpoint, main_menu_option);

  InputOption file_input_option;
  file_input_option.placeholder = "e.g., payload.json (optional)";
  auto file_input = Input(&json_file_path, file_input_option);

  // Added a custom animated style to the button for extra pop
  ButtonOption btn_option = ButtonOption::Animated(Color::Green);
  auto execute_button = Button("Execute via HTTPie", do_request, btn_option);

  // 6. UI Components: History
  MenuOption history_option;
  history_option.entries_option.transform =
      colorize_menu_entry; // Reuse color logic!
  history_option.on_enter = [&] {
    if (history_menu_entries.empty())
      return;

    selected_endpoint = history_endpoint_map[history_selected];

    int session_idx = history_session_map[history_selected];
    const auto *entry = app_session.get_entry(session_idx);
    if (entry) {
      json_file_path = entry->payload_file;
    }

    tab_index = 0;
    do_request();
  };
  auto history_menu =
      Menu(&history_menu_entries, &history_selected, history_option);

  // 7. Component Composition
  auto explorer_container = Container::Vertical({file_input, execute_button});
  auto history_container = Container::Vertical({history_menu});

  auto tab_container =
      Container::Tab({explorer_container, history_container}, &tab_index);

  auto main_interactive = Container::Horizontal({menu, tab_container});
  auto root_container = main_interactive;

  // 8. Event Handler for global shortcuts
  auto event_handler = CatchEvent(root_container, [&](Event e) {
    if (e == Event::Character('1')) {
      tab_index = 0;
      return true;
    }
    if (e == Event::Character('2')) {
      tab_index = 1;
      return true;
    }
    if (e == Event::Character('q') || e == Event::Character('Q')) {
      screen.Exit();
      return true;
    }
    return false;
  });

  // 9. The Renderer
  auto renderer = Renderer(event_handler, [&] {
    Element explorer_view = vbox(
        {text(""),
         hbox({text("  URL: ") | color(Color::CyanLight),
               text(base_url + (endpoints.empty()
                                    ? ""
                                    : endpoints[selected_endpoint].route)) |
                   bold}),
         separator(), text(""),
         hbox({text("  JSON Body File: ") | color(Color::CyanLight),
               file_input->Render()}),
         text(""), separator(), text(""), execute_button->Render() | center,
         text(""), separator(),
         text("  Response:") | bold | color(Color::Yellow), text(""),
         paragraph(response_text) | flex});

    Element history_view;
    if (history_menu_entries.empty()) {
      history_view =
          text("No requests made in this session yet.") | dim | center;
    } else {
      int session_idx = history_session_map[history_selected];
      const auto *entry = app_session.get_entry(session_idx);

      history_view =
          vbox({text("  Past Requests (Press Enter to recall & execute):") |
                    bold | color(Color::CyanLight),
                text(""),
                history_menu->Render() | vscroll_indicator | frame |
                    size(HEIGHT, EQUAL, 5),
                separator(),
                text("  Historical Response:") | bold | color(Color::Yellow),
                text(""), paragraph(entry ? entry->response : "") | flex});
    }

    auto header =
        hbox({filler(),
              text(" Shortcuts: [1] Explorer  [2] History  [q] Quit ") | dim});

    // Custom visual tab indicator
    Element explorer_tab = text(" EXPLORER ");
    explorer_tab = (tab_index == 0) ? (explorer_tab | bold | inverted)
                                    : (explorer_tab | dim);

    Element history_tab = text(" HISTORY ");
    history_tab = (tab_index == 1) ? (history_tab | bold | inverted)
                                   : (history_tab | dim);

    Element tab_indicator = hbox({explorer_tab, history_tab});
    Element active_view = (tab_index == 0) ? explorer_view : history_view;

    // Added BlueLight coloring to the panel borders
    auto right_panel =
        window(tab_indicator, active_view) | flex | color(Color::BlueLight);

    auto left_panel = window(text(" Endpoints ") | bold | color(Color::Cyan),
                             menu->Render() | vscroll_indicator | frame) |
                      size(WIDTH, EQUAL, 40) | color(Color::BlueLight);

    return vbox({header, hbox({left_panel, right_panel}) | flex});
  });

  screen.Loop(renderer);
}

} // namespace ui
