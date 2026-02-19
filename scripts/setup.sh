#!/bin/bash
# First-time server setup. Run once on the DigitalOcean droplet.
set -e

echo "=== Streaming Service — First-Time Setup ==="

# 1. Install nginx RTMP module
echo "[1/5] Installing nginx RTMP module..."
sudo apt update
sudo apt install -y libnginx-mod-rtmp cmake g++ libssl-dev

# 2. Create directories
echo "[2/5] Creating directories..."
sudo mkdir -p /var/www/hls
sudo chown www-data:www-data /var/www/hls
sudo mkdir -p /var/www/streaming-service/bin

# 3. Append RTMP block to nginx.conf
echo "[3/5] Configuring nginx RTMP..."
if ! grep -q "rtmp {" /etc/nginx/nginx.conf; then
    SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
    echo "" | sudo tee -a /etc/nginx/nginx.conf
    cat "$SCRIPT_DIR/nginx/rtmp.conf" | sudo tee -a /etc/nginx/nginx.conf
    echo "RTMP block added"
else
    echo "RTMP block already present, skipping"
fi

# 4. Install site config
echo "[4/5] Installing nginx site config..."
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
sudo cp "$SCRIPT_DIR/nginx/site.conf" /etc/nginx/sites-available/streamingservice
sudo ln -sf /etc/nginx/sites-available/streamingservice /etc/nginx/sites-enabled/
sudo nginx -t && sudo systemctl reload nginx

# 5. Install systemd service
echo "[5/5] Creating systemd service..."
sudo tee /etc/systemd/system/streaming-service.service > /dev/null << 'EOF'
[Unit]
Description=Streaming Service Backend
After=network.target nginx.service

[Service]
Type=simple
User=www-data
Group=www-data
WorkingDirectory=/var/www/streaming-service
ExecStart=/var/www/streaming-service/bin/streaming-service -c /var/www/streaming-service/config.json
Environment=PORT=8085
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable streaming-service

# Firewall
echo ""
echo "=== Setup complete ==="
echo ""
echo "Open RTMP port:  sudo ufw allow 1935/tcp"
echo "Deploy the binary with:  ./scripts/deploy.sh server"
echo "Then start:  sudo systemctl start streaming-service"
