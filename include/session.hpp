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
  void add_entry(const std::string &method, const std::string &url,
                 const std::string &payload_file, const std::string &response);

  // Retrieves the entire history of the current session
  const std::vector<LogEntry> &get_history() const;

  // Retrieves a specific entry by index (useful for the UI later)
  const LogEntry *get_entry(size_t index) const;

  // Clears the current session cache manually
  void clear();

private:
  std::vector<LogEntry> history_;
};

} // namespace session

#endif // SESSION_HPP
