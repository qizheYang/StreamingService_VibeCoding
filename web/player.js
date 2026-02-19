const HLS_SRC = '/hls/stream.m3u8';
const STATUS_API = '/api/status';

const video = document.getElementById('video');
const statusEl = document.getElementById('status');
const viewersEl = document.getElementById('viewers');

let retryTimer = null;
let hls = null;

function startPlayer() {
    if (Hls.isSupported()) {
        hls = new Hls({
            enableWorker: true,
            lowLatencyMode: false,
            backBufferLength: 300,
            liveSyncDurationCount: 5,
            liveMaxLatencyDurationCount: 10,
        });
        hls.loadSource(HLS_SRC);
        hls.attachMedia(video);

        hls.on(Hls.Events.MANIFEST_PARSED, () => {
            setLive();
            video.play().catch(() => {});
            clearRetry();
        });

        hls.on(Hls.Events.ERROR, (event, data) => {
            if (data.fatal) {
                hls.destroy();
                hls = null;
                setOffline();
                scheduleRetry();
            }
        });
    } else if (video.canPlayType('application/vnd.apple.mpegurl')) {
        // Safari native HLS
        video.src = HLS_SRC;
        video.addEventListener('loadedmetadata', () => {
            setLive();
            video.play().catch(() => {});
        });
        video.addEventListener('error', () => {
            setOffline();
            scheduleRetry();
        });
    }
}

function setLive() {
    statusEl.textContent = 'LIVE';
    statusEl.className = 'status live';
}

function setOffline() {
    statusEl.textContent = 'Offline — waiting for stream...';
    statusEl.className = 'status offline';
    viewersEl.textContent = '';
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

async function pollStatus() {
    try {
        const res = await fetch(STATUS_API);
        const data = await res.json();
        if (data.live && (!hls || hls.media === null)) {
            clearRetry();
            startPlayer();
        }
    } catch {}
    setTimeout(pollStatus, 10000);
}

setOffline();
startPlayer();
pollStatus();
