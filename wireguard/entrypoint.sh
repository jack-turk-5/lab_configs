#!/usr/bin/env sh
set -e

# —————————————————————————————————————————————————————————————
# WireGuard UDP (FD 3 ↔ internal UDP:51820)
# —————————————————————————————————————————————————————————————

# Host → container: read from FD 3, send to internal WireGuard port
socat -u FD:3 UDP4:127.0.0.1:51820,reuseaddr,fork &

# Container → host: listen internally, write back to FD 3
socat -u UDP4-LISTEN:51820,reuseaddr,fork FD:3 &

# —————————————————————————————————————————————————————————————
# UI HTTP (FD 4 ↔ internal TCP:51821)
# —————————————————————————————————————————————————————————————

# Host → container: read from FD 4, send to internal UI port
socat -u FD:4 TCP4:127.0.0.1:51821,reuseaddr,fork &

# Container → host: listen internally, write back to FD 4
socat -u TCP4-LISTEN:51821,reuseaddr,fork FD:4 &

# —————————————————————————————————————————————————————————————
# Finally, launch the wg‑easy UI
# —————————————————————————————————————————————————————————————
exec /usr/bin/dumb-init node server.js