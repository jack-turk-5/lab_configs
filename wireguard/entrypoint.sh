#!/usr/bin/env sh
set -e

# 1) Launch the shim (it picks up FD 3, opens /dev/net/tun, configures wg0 via libwg)
#    Run in background so we can continue setup.
wg-shim &

# 2) Wait up to 5 seconds for wg0 to exist
timeout=25
while ! ip link show wg0 >/dev/null 2>&1 && [ $timeout -gt 0 ]; do
  sleep 0.2
  timeout=$((timeout - 1))
done
if ! ip link show wg0 >/dev/null 2>&1; then
  echo "ERROR: wg0 interface never appeared" >&2
  exit 1
fi

# 3) Configure wg0’s IP and bring it up
#    Replace 10.8.0.1/24 with whatever your [Interface] Address= line specifies
ip address add dev wg0 10.8.0.1/24 2>/dev/null || true
ip link set dev wg0 up

# 4) (Optional) Add default route via wg0 if your AllowedIPs include 0.0.0.0/0
ip route add default dev wg0

# 5) Start your UI proxy (socket‑activated FD 4 → localhost:51821)
socat -d -d ACCEPT:4,fork TCP:127.0.0.1:51821 &

# 6) Finally, exec your Node (or whichever) server
exec node /app/server.js