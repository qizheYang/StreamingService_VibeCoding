#include "server.h"
#include "api/stream_api.h"
#include "api/auth_api.h"
#include "utils/logger.h"
#include <filesystem>
#include <csignal>

namespace fs = std::filesystem;

// Global pointer for signal handling
static Server* g_server = nullptr;

static void signal_handler(int sig) {
    Logger::info("Received signal " + std::to_string(sig) + ", shutting down...");
    if (g_server) g_server->stop();
}

Server::Server(const AppConfig& config)
    : config_(config)
    , stream_mgr_(config.hls.path)
    , auth_mgr_(config.auth.stream_keys, config.auth.enabled) {
}

Server::~Server() {
    stop();
}

void Server::run() {
    g_server = this;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    setup_routes();
    setup_hls_serving();
    setup_web_serving();
    start_stream_scanner();

    running_ = true;

    Logger::info("Streaming service backend starting on "
                 + config_.server.host + ":" + std::to_string(config_.server.port));
    Logger::info("HLS path: " + config_.hls.path);
    Logger::info("Web path: " + config_.web.path);
    Logger::info("Auth enabled: " + std::string(config_.auth.enabled ? "yes" : "no"));

    if (!svr_.listen(config_.server.host, config_.server.port)) {
        Logger::error("Failed to start server on " + config_.server.host
                      + ":" + std::to_string(config_.server.port));
    }
}

void Server::stop() {
    running_ = false;
    svr_.stop();
    if (scanner_thread_.joinable()) {
        scanner_thread_.join();
    }
    Logger::info("Server stopped");
}

void Server::setup_routes() {
    // Health check
    svr_.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // CORS preflight handler
    svr_.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.status = 204;
    });

    // Register API routes
    StreamAPI::register_routes(svr_, stream_mgr_);
    AuthAPI::register_routes(svr_, auth_mgr_);

    Logger::info("API routes registered");
}

void Server::setup_hls_serving() {
    // Serve HLS files (.m3u8, .ts) from the HLS directory
    svr_.Get(R"(/hls/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string file = req.matches[1];

        // Prevent path traversal
        if (file.find("..") != std::string::npos) {
            res.status = 403;
            return;
        }

        fs::path full_path = fs::path(config_.hls.path) / file;

        if (!fs::exists(full_path)) {
            res.status = 404;
            return;
        }

        // Set MIME types
        std::string ext = full_path.extension().string();
        std::string content_type = "application/octet-stream";
        if (ext == ".m3u8") content_type = "application/vnd.apple.mpegurl";
        else if (ext == ".ts") content_type = "video/mp2t";

        // Read file
        std::ifstream ifs(full_path, std::ios::binary);
        std::string body((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Cache-Control", "no-cache");
        res.set_content(body, content_type);

        // Track viewer activity for .m3u8 requests
        if (ext == ".m3u8") {
            std::string stream_name = full_path.stem().string();
            stream_mgr_.record_viewer_activity(stream_name);
        }
    });

    Logger::info("HLS serving configured at /hls/");
}

void Server::setup_web_serving() {
    // Serve the web player
    std::string web_path = config_.web.path;

    // Serve index.html at /streamingservice and /
    auto serve_index = [web_path](const httplib::Request&, httplib::Response& res) {
        fs::path index_path = fs::path(web_path) / "index.html";
        if (!fs::exists(index_path)) {
            res.status = 404;
            res.set_content("Player page not found", "text/plain");
            return;
        }

        std::ifstream ifs(index_path);
        std::string body((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
        res.set_content(body, "text/html");
    };

    svr_.Get("/", serve_index);
    svr_.Get("/streamingservice", serve_index);
    svr_.Get("/streamingservice/", serve_index);

    // Serve static files from web directory
    svr_.set_mount_point("/static", web_path);

    Logger::info("Web player serving configured");
}

void Server::start_stream_scanner() {
    // Background thread to periodically scan HLS directory
    scanner_thread_ = std::thread([this]() {
        while (running_) {
            stream_mgr_.scan_hls_directory();
            // Sleep 5 seconds between scans
            for (int i = 0; i < 50 && running_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
}
