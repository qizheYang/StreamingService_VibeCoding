#include "server.h"
#include "core/config.h"
#include "utils/logger.h"
#include <iostream>
#include <string>

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  -c, --config <path>   Config file path (default: config.json)\n"
              << "  -p, --port <port>     Override server port\n"
              << "  -v, --verbose         Enable debug logging\n"
              << "  -h, --help            Show this help\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string config_path = "config.json";
    int port_override = 0;
    bool verbose = false;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_path = argv[++i];
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port_override = std::stoi(argv[++i]);
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (verbose) {
        Logger::set_level(Logger::Level::DEBUG);
    }

    Logger::info("=== Streaming Service Backend ===");

    // Load config
    AppConfig config;
    try {
        config = AppConfig::load(config_path);
    } catch (const std::exception& e) {
        Logger::error("Failed to load config: " + std::string(e.what()));
        return 1;
    }

    // Apply overrides
    if (port_override > 0) {
        config.server.port = port_override;
    }

    // Run server
    try {
        Server server(config);
        server.run();
    } catch (const std::exception& e) {
        Logger::error("Fatal: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
