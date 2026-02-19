#include "core/config.h"
#include "utils/logger.h"
#include <fstream>
#include <stdexcept>

AppConfig AppConfig::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::warn("Config file not found: " + path + ", using defaults");
        return {};
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Failed to parse config: " + std::string(e.what()));
    }

    AppConfig config;

    if (j.contains("server")) {
        auto& s = j["server"];
        if (s.contains("host")) config.server.host = s["host"].get<std::string>();
        if (s.contains("port")) config.server.port = s["port"].get<int>();
    }

    if (j.contains("hls")) {
        auto& h = j["hls"];
        if (h.contains("path")) config.hls.path = h["path"].get<std::string>();
    }

    if (j.contains("web")) {
        auto& w = j["web"];
        if (w.contains("path")) config.web.path = w["path"].get<std::string>();
    }

    if (j.contains("auth")) {
        auto& a = j["auth"];
        if (a.contains("enabled")) config.auth.enabled = a["enabled"].get<bool>();
        if (a.contains("stream_keys")) {
            config.auth.stream_keys = a["stream_keys"].get<std::vector<std::string>>();
        }
    }

    if (j.contains("rtmp")) {
        auto& r = j["rtmp"];
        if (r.contains("port")) config.rtmp.port = r["port"].get<int>();
        if (r.contains("application")) config.rtmp.application = r["application"].get<std::string>();
    }

    Logger::info("Config loaded from " + path);
    return config;
}

void AppConfig::save(const std::string& path) const {
    nlohmann::json j;
    j["server"]["host"] = server.host;
    j["server"]["port"] = server.port;
    j["hls"]["path"] = hls.path;
    j["web"]["path"] = web.path;
    j["auth"]["enabled"] = auth.enabled;
    j["auth"]["stream_keys"] = auth.stream_keys;
    j["rtmp"]["port"] = rtmp.port;
    j["rtmp"]["application"] = rtmp.application;

    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write config to: " + path);
    }
    file << j.dump(4) << std::endl;
}
