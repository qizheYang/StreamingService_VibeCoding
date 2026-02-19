#pragma once

#include <httplib.h>

class StreamManager;

namespace StreamAPI {
    // GET /api/streams          — list all streams
    // GET /api/streams/:name    — get stream info
    // GET /api/status           — quick status check (is any stream live?)
    void register_routes(httplib::Server& svr, StreamManager& mgr);
}
