#!/usr/bin/env sh
set -e

# 1) Start the shim (UDP↔TUN & kernel WireGuard) in background
wg-shim &

# 2) Wait for wg0 to appear
timeout=25
while ! ip link show wg0 >/dev/null 2>&1 && [ $timeout -gt 0 ]; do
  sleep 0.2; timeout=$((timeout-1))
done
[ $timeout -gt 0 ] || { echo "ERROR: wg0 never appeared" >&2; exit 1; }

# 3) Assign IP/netmask and bring wg0 up
[ -n "$WG_ADDRESS" ] && ip address add dev wg0 "$WG_ADDRESS"
ip link set up dev wg0

# 4) Launch UI proxy: socket FD 4 → Node server
socat -d -d ACCEPT:4,fork TCP:127.0.0.1:51821 &

# Start your server
exec node /app/server.js