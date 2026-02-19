#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
};

struct HlsConfig {
    std::string path = "/var/www/hls";
};

struct WebConfig {
    std::string path = "./web";
};

struct AuthConfig {
    bool enabled = true;
    std::vector<std::string> stream_keys = {"stream"};
};

struct RtmpConfig {
    int port = 1935;
    std::string application = "live";
};

struct AppConfig {
    ServerConfig server;
    HlsConfig hls;
    WebConfig web;
    AuthConfig auth;
    RtmpConfig rtmp;

    static AppConfig load(const std::string& path);
    void save(const std::string& path) const;
};
