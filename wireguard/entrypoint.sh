#!/usr/bin/env sh
set -e

# 1) Bring up wg0
wg-quick up wg0

# 2) Wait for wg0
timeout=25
while ! ip link show wg0 >/dev/null 2>&1 && [ $timeout -gt 0 ]; do
  sleep 0.2
  timeout=$((timeout - 1))
done
if ! ip link show wg0 >/dev/null 2>&1; then
  echo "ERROR: wg0 interface did not come up" >&2
  exit 1
fi

# 3) UI plane (TCP 51821 → FD 4)
socat -d -d ACCEPT:4,fork TCP:127.0.0.1:51821 &

# 4) VPN data plane (UDP FD 3 ↔ wg0)
socat -d -d FD:3 \
    TUN:10.8.0.1/24,tun-name=wg0,iff-no-pi &

socat -d -d \
    TUN:10.8.0.1/24,tun-name=wg0,iff-no-pi \
    FD:3 &

# 5) Start the UI server
exec node /app/server.js