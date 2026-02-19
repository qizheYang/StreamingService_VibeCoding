#include "core/auth_manager.h"
#include "utils/logger.h"
#include <random>
#include <sstream>
#include <iomanip>

AuthManager::AuthManager(const std::vector<std::string>& keys, bool enabled)
    : enabled_(enabled), keys_(keys.begin(), keys.end()) {
    Logger::info("Auth manager initialized with " + std::to_string(keys_.size()) + " keys, enabled=" + (enabled_ ? "true" : "false"));
}

bool AuthManager::validate(const std::string& key) const {
    if (!enabled_) return true;

    std::lock_guard<std::mutex> lock(mutex_);
    return keys_.count(key) > 0;
}

std::string AuthManager::generate_key() {
    // Generate a random 32-character hex key
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::ostringstream oss;
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(2) << dis(gen);
    }

    std::string key = oss.str();
    add_key(key);
    return key;
}

void AuthManager::add_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    keys_.insert(key);
    Logger::info("Stream key added");
}

bool AuthManager::remove_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return keys_.erase(key) > 0;
}

std::vector<std::string> AuthManager::list_keys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {keys_.begin(), keys_.end()};
}
