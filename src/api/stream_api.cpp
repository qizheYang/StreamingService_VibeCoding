#include "api/stream_api.h"
#include "core/stream_manager.h"
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

namespace {

std::string time_to_iso(std::chrono::system_clock::time_point tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    auto tm = std::gmtime(&time);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tm);
    return buf;
}

int64_t uptime_seconds(std::chrono::system_clock::time_point started_at) {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - started_at).count();
}

json stream_to_json(const StreamInfo& info) {
    json j;
    j["name"] = info.name;
    j["live"] = info.live;
    if (info.live) {
        j["started_at"] = time_to_iso(info.started_at);
        j["uptime_seconds"] = uptime_seconds(info.started_at);
    }
    j["viewers"] = info.viewer_estimate;
    return j;
}

} // anonymous namespace

void StreamAPI::register_routes(httplib::Server& svr, StreamManager& mgr) {

    // GET /api/status — quick check
    svr.Get("/api/status", [&mgr](const httplib::Request&, httplib::Response& res) {
        auto streams = mgr.get_all_streams();
        bool any_live = false;
        for (const auto& s : streams) {
            if (s.live) { any_live = true; break; }
        }

        json j;
        j["live"] = any_live;
        j["stream_count"] = streams.size();

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(j.dump(), "application/json");
    });

    // GET /api/streams — list all
    svr.Get("/api/streams", [&mgr](const httplib::Request&, httplib::Response& res) {
        auto streams = mgr.get_all_streams();
        json arr = json::array();
        for (const auto& s : streams) {
            arr.push_back(stream_to_json(s));
        }

        json j;
        j["streams"] = arr;

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(j.dump(), "application/json");
    });

    // GET /api/streams/:name
    svr.Get(R"(/api/streams/(\w+))", [&mgr](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        auto info = mgr.get_stream(name);

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(stream_to_json(info).dump(), "application/json");
    });

    // POST /api/streams/:name/publish — called by nginx on_publish
    svr.Post(R"(/api/streams/(\w+)/publish)", [&mgr](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        mgr.on_publish(name);

        json j;
        j["status"] = "ok";
        j["stream"] = name;

        res.set_content(j.dump(), "application/json");
    });

    // POST /api/streams/:name/publish_done — called by nginx on_publish_done
    svr.Post(R"(/api/streams/(\w+)/publish_done)", [&mgr](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1];
        mgr.on_publish_done(name);

        json j;
        j["status"] = "ok";
        j["stream"] = name;

        res.set_content(j.dump(), "application/json");
    });
}
