#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <filesystem>

struct StreamInfo {
    std::string name;
    bool live = false;
    std::chrono::system_clock::time_point started_at;
    int viewer_estimate = 0;
    std::chrono::system_clock::time_point last_viewer_ping;
};

class StreamManager {
public:
    explicit StreamManager(const std::string& hls_path);

    // Called by nginx on_publish / on_publish_done callbacks
    void on_publish(const std::string& stream_name);
    void on_publish_done(const std::string& stream_name);

    // Query stream status
    bool is_live(const std::string& stream_name) const;
    std::vector<StreamInfo> get_all_streams() const;
    StreamInfo get_stream(const std::string& stream_name) const;

    // Track viewer activity (called on HLS requests)
    void record_viewer_activity(const std::string& stream_name);

    // Scan HLS directory for active streams (fallback detection)
    void scan_hls_directory();

private:
    std::string hls_path_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, StreamInfo> streams_;

    bool hls_files_exist(const std::string& stream_name) const;
};
