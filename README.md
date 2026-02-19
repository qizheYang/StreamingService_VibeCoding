# Streaming Service

Self-hosted live streaming for `rehydratedwater.com/streamingservice`. C++ backend with REST API, nginx-rtmp for RTMP ingestion, hls.js web player.

## Watch the Stream

**https://rehydratedwater.com/streamingservice**

The player auto-detects when a stream goes live and starts playing.

## OBS Studio Setup

### 1. Stream Settings

Open OBS **Settings** -> **Stream**:

| Field | Value |
|-------|-------|
| Service | **Custom...** |
| Server | `rtmp://rehydratedwater.com/live` |
| Stream Key | `stream` |

The stream key must match one in `config.json` `auth.stream_keys`. Default is `stream`.

### 2. Output Settings

**Settings** -> **Output** -> **Streaming**:

| Setting | Value |
|---------|-------|
| Encoder | x264 or hardware (NVENC/QSV) |
| Rate Control | CBR |
| Bitrate | 2500-6000 kbps |
| Keyframe Interval | 2 seconds |

**Settings** -> **Video**:

| Setting | Value |
|---------|-------|
| Base Resolution | 1920x1080 |
| Output Resolution | 1920x1080 or 1280x720 |
| FPS | 30 or 60 |

### 3. Go Live

Click **Start Streaming** in OBS. Viewers at `https://rehydratedwater.com/streamingservice` will see the stream within ~10 seconds.

## Architecture

```
OBS Studio
    | RTMP (port 1935)
    v
Nginx + RTMP Module
    | on_publish -> C++ backend auth
    | RTMP -> HLS (.m3u8 + .ts)
    v
C++ Backend (port 8085)
    | HLS serving, web player, REST API
    v
Nginx reverse proxy (port 443)
    v
Viewer browser (hls.js)
```

## Deploy

Deployment follows the same pattern as Guandan/Mahjong:
- Web player files pushed to `qizheYang/rehydratedwater.com` repo (auto-deployed via webhook)
- Server binary compiled on the DigitalOcean droplet, managed by systemd

### Auto (GitHub Actions)

Push to `main` triggers `.github/workflows/deploy.yml`:
1. Copies `web/index.html` into the `rehydratedwater.com` repo under `streamingservice/`
2. SSHs into the server, builds the C++ binary, does atomic swap, restarts the service

### Manual

```bash
# Deploy everything
./scripts/deploy.sh all

# Just the web player
./scripts/deploy.sh web

# Just the server binary
./scripts/deploy.sh server
```

### First-time server setup

SSH into the server and install nginx-rtmp:

```bash
sudo apt install -y libnginx-mod-rtmp

# Append RTMP block to nginx.conf (see nginx/rtmp.conf)
# Install site config (see nginx/site.conf)
# Open RTMP port
sudo ufw allow 1935/tcp
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/streaming-service -c config.json
```

Requires CMake 3.16+, C++17 compiler, OpenSSL dev headers. Dependencies (cpp-httplib, nlohmann/json) fetched automatically by CMake.

## API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/health` | GET | Health check |
| `/api/status` | GET | Is any stream live? |
| `/api/streams` | GET | List all streams |
| `/api/streams/:name` | GET | Single stream info |
| `/api/auth` | POST | Validate stream key (nginx callback) |
| `/api/auth/keys` | GET | List stream keys |
| `/api/auth/keys` | POST | Generate new key |
| `/api/auth/keys/:key` | DELETE | Remove a key |

## Configuration

`config.json`:
```json
{
    "server": { "host": "0.0.0.0", "port": 8085 },
    "hls": { "path": "/var/www/hls" },
    "web": { "path": "./web" },
    "auth": { "enabled": true, "stream_keys": ["stream"] },
    "rtmp": { "port": 1935, "application": "live" }
}
```

CLI: `./streaming-service [-c config.json] [-p port] [-v] [-h]`

## Server Management

```bash
sudo systemctl status streaming-service
sudo systemctl restart streaming-service
journalctl -u streaming-service -f
```

## Troubleshooting

| Problem | Fix |
|---------|-----|
| OBS can't connect | `sudo ufw allow 1935/tcp` |
| Player says "Offline" | Check OBS is streaming; check `/var/www/hls/` for .m3u8 files |
| Auth rejected | Stream key must match `auth.stream_keys` in config.json |
| Port conflict | Mahjong uses 8080, this service uses 8085 |
| HLS files not created | `sudo chown www-data:www-data /var/www/hls` |
| High latency | Set `hls_fragment 1s;` in nginx rtmp.conf |
