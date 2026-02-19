cmake_minimum_required(VERSION 3.16)
project(streaming-service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies via FetchContent ---
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.3
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

FetchContent_MakeAvailable(httplib json)

# --- Main executable ---
add_executable(streaming-service
    src/main.cpp
    src/server.cpp
    src/core/config.cpp
    src/core/stream_manager.cpp
    src/core/auth_manager.cpp
    src/api/stream_api.cpp
    src/api/auth_api.cpp
    src/utils/logger.cpp
)

target_include_directories(streaming-service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(streaming-service PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
)

# Find OpenSSL for HTTPS support (optional)
find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
    target_compile_definitions(streaming-service PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(streaming-service PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "OpenSSL found — HTTPS support enabled")
else()
    message(STATUS "OpenSSL not found — HTTP only")
endif()

# Find threads
find_package(Threads REQUIRED)
target_link_libraries(streaming-service PRIVATE Threads::Threads)

# --- Install ---
install(TARGETS streaming-service DESTINATION bin)
install(DIRECTORY web/ DESTINATION share/streaming-service/web)
install(FILES config.json DESTINATION etc/streaming-service)
