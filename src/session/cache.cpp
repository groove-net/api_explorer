#include "session.hpp"

namespace session {

void Cache::add_entry(const std::string &method, const std::string &url,
                      const std::string &payload_file,
                      const std::string &response) {
  history_.push_back({method, url, payload_file, response});
}

const std::vector<LogEntry> &Cache::get_history() const { return history_; }

const LogEntry *Cache::get_entry(size_t index) const {
  if (index < history_.size()) {
    return &history_[index];
  }
  return nullptr;
}

void Cache::clear() { history_.clear(); }

} // namespace session
