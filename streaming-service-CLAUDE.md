# 自建直播服务部署指南 — CLAUDE.md

## 目标

在 `rehydratedwater.com/streamingservice` 上部署自建直播服务。
OBS Studio 通过 RTMP 推流到服务器，服务器转码为 HLS，观众通过浏览器访问网页播放器观看直播。

## 架构概览

```
OBS Studio (导播电脑)
    ↓ RTMP 推流 (端口 1935)
    ↓ rtmp://rehydratedwater.com/live/<stream_key>
Nginx + RTMP Module (服务器)
    ↓ 实时转码为 HLS
    ↓ 生成 .m3u8 播放列表 + .ts 分片
Nginx HTTP (端口 443)
    ↓ 通过 HTTPS 提供 HLS 文件
    ↓ https://rehydratedwater.com/streamingservice/
观众浏览器
    ↓ hls.js 播放器加载 .m3u8
    ↓ 实时观看直播
```

## 技术栈

- **Nginx** + `libnginx-mod-rtmp`（RTMP 接收 + HLS 转码）
- **Let's Encrypt / Certbot**（HTTPS 证书，假设已有或将配置）
- **hls.js**（浏览器端 HLS 播放器，CDN 引入）
- **HTML/CSS/JS**（播放器前端页面）

## 服务器要求

- Ubuntu 22.04+ 或 Debian 12+
- 至少 2 核 CPU，2GB RAM（单路 1080p 推流足够）
- 开放端口：1935（RTMP）、80/443（HTTP/HTTPS）
- 域名 `rehydratedwater.com` 已解析到此服务器

## 部署步骤

### 1. 安装 Nginx + RTMP 模块

```bash
sudo apt update
sudo apt install -y nginx libnginx-mod-rtmp
```

验证模块已加载：

```bash
nginx -V 2>&1 | grep rtmp
# 应该看到 --add-dynamic-module=...nginx-rtmp-module
```

### 2. 配置 Nginx

编辑 `/etc/nginx/nginx.conf`，在文件**末尾**（`http {}` 块之外）添加 RTMP 配置块：

```nginx
rtmp {
    server {
        listen 1935;
        chunk_size 4096;

        application live {
            live on;
            record off;

            # 推流密钥验证（简单方案：on_publish 回调）
            # on_publish http://127.0.0.1/auth;

            # HLS 输出
            hls on;
            hls_path /var/www/hls;
            hls_fragment 3s;
            hls_playlist_length 60s;
            hls_cleanup on;

            # 禁止通过 RTMP 直接拉流（只允许 HLS 播放）
            deny play all;
        }
    }
}
```

在 `http {}` 块内的 `server {}` 中（或新建一个 server 块），添加 HLS 和播放器页面的 location：

```nginx
server {
    listen 443 ssl;
    server_name rehydratedwater.com;

    # SSL 证书路径（根据实际情况修改）
    ssl_certificate /etc/letsencrypt/live/rehydratedwater.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/rehydratedwater.com/privkey.pem;

    # 已有的网站配置...
    # root /var/www/html;
    # ...

    # ======= 直播服务 =======

    # HLS 分片文件
    location /hls {
        types {
            application/vnd.apple.mpegurl m3u8;
            video/mp2t ts;
        }
        root /var/www;
        add_header Cache-Control no-cache;
        add_header Access-Control-Allow-Origin *;
    }

    # 直播播放器页面
    location /streamingservice {
        alias /var/www/streamingservice;
        index index.html;
        try_files $uri $uri/ /streamingservice/index.html;
    }

    # RTMP 统计信息（可选，调试用）
    location /stat {
        rtmp_stat all;
        rtmp_stat_stylesheet stat.xsl;
        # 限制访问
        allow 127.0.0.1;
        deny all;
    }
}

# HTTP 重定向到 HTTPS
server {
    listen 80;
    server_name rehydratedwater.com;
    return 301 https://$host$request_uri;
}
```

### 3. 创建 HLS 目录

```bash
sudo mkdir -p /var/www/hls
sudo chown www-data:www-data /var/www/hls
```

### 4. 创建播放器页面

创建目录：

```bash
sudo mkdir -p /var/www/streamingservice
```

创建 `/var/www/streamingservice/index.html`：

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>直播 — rehydratedwater.com</title>
    <script src="https://cdn.jsdelivr.net/npm/hls.js@latest"></script>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: #0d0d14;
            color: #e0e0e0;
            font-family: 'Noto Sans SC', -apple-system, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
            padding: 2rem;
        }
        h1 {
            font-size: 1.5rem;
            margin-bottom: 1rem;
            color: #f0c040;
        }
        .player-container {
            width: 100%;
            max-width: 1280px;
            background: #000;
            border-radius: 8px;
            overflow: hidden;
            box-shadow: 0 4px 24px rgba(0,0,0,0.5);
        }
        video {
            width: 100%;
            display: block;
        }
        .status {
            margin-top: 1rem;
            font-size: 0.9rem;
            color: #888;
        }
        .status.live {
            color: #e74c3c;
            font-weight: bold;
        }
        .status.offline {
            color: #666;
        }
        .info {
            margin-top: 2rem;
            max-width: 1280px;
            width: 100%;
            color: #aaa;
            font-size: 0.85rem;
            line-height: 1.6;
        }
    </style>
</head>
<body>
    <h1>🀄 麻雀配信</h1>
    <div class="player-container">
        <video id="video" controls autoplay muted></video>
    </div>
    <div id="status" class="status offline">检查直播状态…</div>
    <div class="info" id="info"></div>

    <script>
        const VIDEO_SRC = '/hls/stream.m3u8';
        const video = document.getElementById('video');
        const statusEl = document.getElementById('status');
        let retryTimer = null;

        function startPlayer() {
            if (Hls.isSupported()) {
                const hls = new Hls({
                    enableWorker: true,
                    lowLatencyMode: true,
                    backBufferLength: 90,
                });
                hls.loadSource(VIDEO_SRC);
                hls.attachMedia(video);

                hls.on(Hls.Events.MANIFEST_PARSED, () => {
                    statusEl.textContent = '🔴 直播中';
                    statusEl.className = 'status live';
                    video.play().catch(() => {});
                    clearRetry();
                });

                hls.on(Hls.Events.ERROR, (event, data) => {
                    if (data.fatal) {
                        hls.destroy();
                        setOffline();
                        scheduleRetry();
                    }
                });
            } else if (video.canPlayType('application/vnd.apple.mpegurl')) {
                // Safari 原生 HLS 支持
                video.src = VIDEO_SRC;
                video.addEventListener('loadedmetadata', () => {
                    statusEl.textContent = '🔴 直播中';
                    statusEl.className = 'status live';
                    video.play().catch(() => {});
                });
                video.addEventListener('error', () => {
                    setOffline();
                    scheduleRetry();
                });
            }
        }

        function setOffline() {
            statusEl.textContent = '⏸ 当前未在直播，等待开播…';
            statusEl.className = 'status offline';
        }

        function scheduleRetry() {
            if (!retryTimer) {
                retryTimer = setTimeout(() => {
                    retryTimer = null;
                    startPlayer();
                }, 5000);
            }
        }

        function clearRetry() {
            if (retryTimer) {
                clearTimeout(retryTimer);
                retryTimer = null;
            }
        }

        // 启动
        setOffline();
        startPlayer();
    </script>
</body>
</html>
```

### 5. 测试配置并重启 Nginx

```bash
sudo nginx -t
sudo systemctl restart nginx
```

### 6. 防火墙放行

```bash
# UFW
sudo ufw allow 1935/tcp comment 'RTMP'
sudo ufw allow 'Nginx Full'

# 或 iptables
sudo iptables -A INPUT -p tcp --dport 1935 -j ACCEPT
```

### 7. OBS 推流设置

在 OBS 中：

- **设置** → **推流**
- 服务：**自定义**
- 服务器：`rtmp://rehydratedwater.com/live`
- 推流密钥：`stream`（对应 HLS 文件名 stream.m3u8）

点击「开始直播」后，观众访问 `https://rehydratedwater.com/streamingservice` 即可观看。

## 目录结构

```
/var/www/
├── hls/                          # HLS 分片（Nginx RTMP 自动写入，自动清理）
│   ├── stream.m3u8               # 播放列表（推流时自动生成）
│   ├── stream-0.ts               # 视频分片
│   ├── stream-1.ts
│   └── ...
├── streamingservice/
│   └── index.html                # 播放器网页
└── html/                         # 原有网站文件（不动）
```

## 配置文件位置

| 文件 | 路径 | 说明 |
|------|------|------|
| Nginx 主配置 | `/etc/nginx/nginx.conf` | RTMP 块加在末尾 |
| 站点配置 | `/etc/nginx/sites-available/default` 或 `rehydratedwater.com` | HTTP/HTTPS server 块 |
| 播放器页面 | `/var/www/streamingservice/index.html` | 前端播放器 |
| HLS 输出目录 | `/var/www/hls/` | 权限 www-data |
| SSL 证书 | `/etc/letsencrypt/live/rehydratedwater.com/` | Let's Encrypt |

## 推流密钥与安全

### 简单方案：固定密钥

OBS 推流密钥设为任意字符串（如 `stream`、`mahjong2024`），对应的 HLS 播放地址就是 `/hls/<密钥>.m3u8`。

### 进阶方案：on_publish 验证

在 RTMP application 块中启用：

```nginx
application live {
    live on;
    on_publish http://127.0.0.1/auth;
    # ...
}
```

然后写一个简单的验证端点：

```nginx
location = /auth {
    if ($arg_key != 'YOUR_SECRET_KEY') {
        return 403;
    }
    return 200;
}
```

OBS 推流密钥格式变为：`stream?key=YOUR_SECRET_KEY`

这样只有知道密钥的人才能推流，但观众仍可通过网页观看。

## 可选功能

### 多码率自适应（ABR）

用 `exec` 调用 ffmpeg 转码多个分辨率：

```nginx
application live {
    live on;
    
    exec ffmpeg -i rtmp://localhost/live/$name
        -c:a aac -b:a 128k -c:v libx264 -b:v 2500k -s 1280x720 -f flv rtmp://localhost/hls/$name_720p
        -c:a aac -b:a 96k  -c:v libx264 -b:v 1000k -s 854x480  -f flv rtmp://localhost/hls/$name_480p;
}

application hls {
    live on;
    hls on;
    hls_path /var/www/hls;
    hls_fragment 3s;
    hls_playlist_length 60s;
    hls_variant _720p BANDWIDTH=2628000,RESOLUTION=1280x720;
    hls_variant _480p BANDWIDTH=1096000,RESOLUTION=854x480;
}
```

### 直播录像

```nginx
application live {
    live on;
    record all;
    record_path /var/www/recordings;
    record_unique on;
    record_suffix _%Y%m%d_%H%M%S.flv;
    # ...
}
```

录像保存在 `/var/www/recordings/`，后续可用 ffmpeg 转为 mp4。

### 统计监控页面

下载 stat.xsl：

```bash
sudo wget https://raw.githubusercontent.com/arut/nginx-rtmp-module/master/stat.xsl \
    -O /var/www/html/stat.xsl
```

访问 `https://rehydratedwater.com/stat` 查看在线流、观众数、码率等实时数据。

### 聊天功能

播放器页面可集成 Discord 嵌入式频道、自建 WebSocket 聊天、或第三方聊天服务（如 Minnit）。不在本文档范围内，但 index.html 可以随时扩展。

## 运维命令

```bash
# 查看 Nginx 状态
sudo systemctl status nginx

# 重新加载配置（不中断现有连接）
sudo nginx -t && sudo systemctl reload nginx

# 查看 RTMP 连接日志
sudo tail -f /var/log/nginx/error.log

# 手动清理 HLS 分片（正常情况 hls_cleanup 会自动处理）
sudo rm -rf /var/www/hls/*

# 查看当前推流连接
curl http://127.0.0.1/stat 2>/dev/null | grep '<name>'
```

## 故障排查

| 问题 | 排查方向 |
|------|---------|
| OBS 推流失败 | 检查防火墙 1935 端口、`nginx -t` 验证配置、`tail -f /var/log/nginx/error.log` |
| 播放器显示「未在直播」 | 确认 OBS 已开始推流、检查 `/var/www/hls/` 下是否有 .m3u8 和 .ts 文件 |
| 播放卡顿/缓冲 | 检查服务器上行带宽（1080p 需要 6Mbps+）、降低 OBS 码率、增大 hls_fragment |
| HTTPS 证书问题 | `sudo certbot renew --dry-run` 测试续期、检查证书路径 |
| HLS 分片堆积 | 确认 `hls_cleanup on;` 已启用、检查 `/var/www/hls/` 目录权限 |

## 延迟说明

| 配置 | 预期延迟 |
|------|---------|
| 默认 HLS (3s fragment) | 6-15 秒 |
| 低延迟 HLS (1s fragment) | 3-8 秒 |
| LL-HLS | 2-4 秒（需额外配置） |

麻将直播场景下 6-15 秒延迟完全可以接受。如需更低延迟，将 `hls_fragment` 改为 `1s`，`hls_playlist_length` 改为 `10s`。

## 注意事项

- RTMP 块必须放在 `nginx.conf` 的顶层，与 `http {}` 块同级，不能嵌套在 `http {}` 内
- HLS 目录权限必须是 `www-data`（Nginx 工作进程用户）
- 推流密钥就是 HLS 文件名，不要用容易猜到的值（如果需要安全性）
- 如果服务器已有其他 Nginx 站点配置，注意不要覆盖，只添加 RTMP 块和新的 location
- 先在本地用 `rtmp://服务器IP/live/test` 测试，确认通了再绑定域名
