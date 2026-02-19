#include "api/auth_api.h"
#include "core/auth_manager.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void AuthAPI::register_routes(httplib::Server& svr, AuthManager& mgr) {

    // POST /api/auth — nginx on_publish callback
    // nginx sends: name=<stream_name>&key=<stream_key> as form data
    // Return 200 to allow, 403 to reject
    svr.Post("/api/auth", [&mgr](const httplib::Request& req, httplib::Response& res) {
        // nginx-rtmp sends stream info as form-encoded POST body
        std::string key;

        // Try form param "key"
        if (req.has_param("key")) {
            key = req.get_param_value("key");
        }
        // Also check "name" param (the stream name itself can be the key)
        if (key.empty() && req.has_param("name")) {
            key = req.get_param_value("name");
        }

        if (mgr.validate(key)) {
            Logger::info("Auth OK for key: " + key);
            res.status = 200;
            res.set_content("OK", "text/plain");
        } else {
            Logger::warn("Auth REJECTED for key: " + key);
            res.status = 403;
            res.set_content("Forbidden", "text/plain");
        }
    });

    // POST /api/auth/keys — generate new key
    svr.Post("/api/auth/keys", [&mgr](const httplib::Request&, httplib::Response& res) {
        std::string key = mgr.generate_key();

        json j;
        j["key"] = key;

        res.set_content(j.dump(), "application/json");
    });

    // GET /api/auth/keys — list all keys
    svr.Get("/api/auth/keys", [&mgr](const httplib::Request&, httplib::Response& res) {
        auto keys = mgr.list_keys();

        json j;
        j["keys"] = keys;
        j["enabled"] = mgr.is_enabled();

        res.set_content(j.dump(), "application/json");
    });

    // DELETE /api/auth/keys/:key
    svr.Delete(R"(/api/auth/keys/(\w+))", [&mgr](const httplib::Request& req, httplib::Response& res) {
        std::string key = req.matches[1];
        bool removed = mgr.remove_key(key);

        json j;
        j["removed"] = removed;
        j["key"] = key;

        res.set_content(j.dump(), "application/json");
    });
}
