#!/usr/bin/env sh
set -e

# 1) Bring up wg0 (creates /dev/net/tun, assigns IP, brings interface up)
wg-quick up wg0

# 2) UI plane (TCP 51821) → local Node server
socat -d -d \
    ACCEPT:4,fork \
    TCP:127.0.0.1:51821 &

# 3) VPN data plane (UDP 51820/socket‑activated fd 3) ↔ wg0 (TUN)
# Inbound: read from UDP socket FD:3 into the existing wg0
socat -d -d \
    FD:3 \
    TUN:tun-name=wg0,tun-type=tun,iff-no-pi &

# Outbound: read from wg0 and send back out FD:3
socat -d -d \
    TUN:tun-name=wg0,tun-type=tun,iff-no-pi \
    FD:3 &

# 4) Finally launch the UI
exec node /app/server.js