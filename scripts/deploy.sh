#!/bin/bash
set -e

MODE="${1:-all}"
SSH_HOST="digitalocean"

deploy_web() {
    echo "=== Deploying web player ==="

    # Clone/update target repo
    TEMP_DIR=$(mktemp -d)
    git clone https://github.com/qizheYang/rehydratedwater.com.git "$TEMP_DIR"

    mkdir -p "$TEMP_DIR/streamingservice"
    cp web/index.html "$TEMP_DIR/streamingservice/"

    cd "$TEMP_DIR"
    git add streamingservice/
    if ! git diff --staged --quiet; then
        git commit -m "Auto-deploy streaming service web"
        git push
        echo "Web player deployed to rehydratedwater.com/streamingservice/"
    else
        echo "No web changes to deploy"
    fi

    rm -rf "$TEMP_DIR"
}

deploy_server() {
    echo "=== Deploying server binary ==="

    ssh "$SSH_HOST" << 'REMOTE'
        set -e

        # Clone fresh
        rm -rf /tmp/streamingservice
        git clone https://github.com/qizheYang/StreamingService_VibeCoding.git /tmp/streamingservice
        cd /tmp/streamingservice

        # Build
        cmake -B build -DCMAKE_BUILD_TYPE=Release
        cmake --build build -j$(nproc)

        # Deploy binary (atomic swap)
        sudo mkdir -p /var/www/streaming-service/bin
        sudo cp build/streaming-service /var/www/streaming-service/bin/streaming-service_new
        sudo cp config.json /var/www/streaming-service/
        sudo cp -r web /var/www/streaming-service/
        sudo sed -i 's|"./web"|"/var/www/streaming-service/web"|' /var/www/streaming-service/config.json
        sudo mv /var/www/streaming-service/bin/streaming-service_new /var/www/streaming-service/bin/streaming-service

        # Ensure HLS dir
        sudo mkdir -p /var/www/hls
        sudo chown www-data:www-data /var/www/hls

        # Restart
        sudo systemctl restart streaming-service
        echo "Server binary deployed and restarted"

        # Clean up
        rm -rf /tmp/streamingservice
REMOTE
}

case "$MODE" in
    web)    deploy_web ;;
    server) deploy_server ;;
    all)    deploy_web; deploy_server ;;
    *)      echo "Usage: $0 {web|server|all}"; exit 1 ;;
esac

echo "=== Done ==="
