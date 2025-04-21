#!/usr/bin/env sh
set -e

# 1) Launch wg-shim (handles UDP↔TUN & kernel config)
wg-shim &

# 2) Wait for wg0 to appear
timeout=25
while ! ip link show wg0 >/dev/null 2>&1 && [ $timeout -gt 0 ]; do
  sleep 0.2; timeout=$((timeout-1))
done
[ $timeout -gt 0 ] || { echo "wg0 never appeared" >&2; exit 1; }

# 3) Assign IP from $WG_ADDRESS and up it
[ -n "$WG_ADDRESS" ] && ip address add dev wg0 "$WG_ADDRESS"
ip link set up dev wg0

# 4) Sock‑activated UI (FD 4) → local server
socat -d -d ACCEPT:4,fork TCP:127.0.0.1:51821 &
# 6) Finally, exec your Node (or whichever) server
exec node /app/server.js