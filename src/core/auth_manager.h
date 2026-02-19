#pragma once

#include <string>
#include <set>
#include <mutex>
#include <vector>

class AuthManager {
public:
    explicit AuthManager(const std::vector<std::string>& keys, bool enabled = true);

    // Validate a stream key
    bool validate(const std::string& key) const;

    // Key management
    std::string generate_key();
    void add_key(const std::string& key);
    bool remove_key(const std::string& key);
    std::vector<std::string> list_keys() const;

    bool is_enabled() const { return enabled_; }

private:
    bool enabled_;
    mutable std::mutex mutex_;
    std::set<std::string> keys_;
};
