#pragma once

#include "core/config.h"
#include "core/stream_manager.h"
#include "core/auth_manager.h"
#include <httplib.h>
#include <atomic>
#include <thread>

class Server {
public:
    explicit Server(const AppConfig& config);
    ~Server();

    void run();
    void stop();

private:
    void setup_routes();
    void setup_hls_serving();
    void setup_web_serving();
    void start_stream_scanner();

    AppConfig config_;
    httplib::Server svr_;
    StreamManager stream_mgr_;
    AuthManager auth_mgr_;
    std::atomic<bool> running_{false};
    std::thread scanner_thread_;
};
