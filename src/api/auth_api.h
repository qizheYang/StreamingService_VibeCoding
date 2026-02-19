#pragma once

#include <httplib.h>

class AuthManager;

namespace AuthAPI {
    // POST /api/auth             — validate stream key (nginx on_publish callback)
    // POST /api/auth/keys        — generate a new stream key
    // DELETE /api/auth/keys/:key — remove a stream key
    // GET /api/auth/keys         — list stream keys (admin)
    void register_routes(httplib::Server& svr, AuthManager& mgr);
}
