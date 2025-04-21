#!/usr/bin/env sh
set -e

# 1) Bring wg0 up (creates the TUN and configures its IP, e.g. 10.8.0.1/24)
wg-quick up wg0

# 2) UI plane: socket‑activated TCP (FD 4) → local server
socat -d -d \
    ACCEPT:4,fork \
    TCP:127.0.0.1:51821 &

# 3) VPN data plane: UDP socket (FD 3) ↔ existing wg0 (TUN)
#    TUN:<your-IP/CIDR>,tun-name=wg0,iff-no-pi
#    Replace 10.8.0.1/24 with the address from your wg0 config
socat -d -d \
    FD:3 \
    TUN:10.8.0.1/24,tun-name=wg0,iff-no-pi &

socat -d -d \
    TUN:10.8.0.1/24,tun-name=wg0,iff-no-pi \
    FD:3 &

# 4) Start the UI
exec node /app/server.js