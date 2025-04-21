#!/usr/bin/env sh
set -e

# 1) Bring wg0 up (creates /dev/net/tun, sets IP, brings interface up)
wg-quick up wg0

# 2) UI plane: TCP socket (FD 4) → local server
socat -d -d \
    ACCEPT:4,fork \
    TCP:127.0.0.1:51821 &

# 3) VPN data plane: UDP socket (FD 3) ↔ wg0 (TUN)
# Inbound: deliver UDP packets from FD 3 into wg0
socat -d -d \
    FD:3 \
    TUN:tun-name=wg0,tun-type tun,iff-no-pi &

# Outbound: send packets from wg0 back out FD 3
socat -d -d \
    TUN:tun-name=wg0,tun-type tun,iff-no-pi \
    FD:3 &

# 4) Finally, start the UI
exec node /app/server.js