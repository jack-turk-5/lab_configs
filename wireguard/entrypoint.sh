#!/usr/bin/env sh
set -e

# 1) Kick off the WG‑Easy UI entrypoint in the background.
#    This will generate /etc/wireguard/wg0.conf from ENV vars if needed.
#    We exec the original entrypoint script shipped with wg‑easy.
exec /usr/local/bin/docker-entrypoint.sh &

# 2) Wait up to 10 seconds for /etc/wireguard/wg0.conf to be created
timeout=20
while [ ! -f /etc/wireguard/wg0.conf ] && [ $timeout -gt 0 ]; do
  sleep 0.5
  timeout=$((timeout - 1))
done

if [ ! -f /etc/wireguard/wg0.conf ]; then
  echo "ERROR: /etc/wireguard/wg0.conf never appeared" >&2
  exit 1
fi

# 3) Now that the base config exists, start the shim
wg-shim &

# 4) Wait for wg0 to show up in ip link
timeout=25
while ! ip link show wg0 >/dev/null 2>&1 && [ $timeout -gt 0 ]; do
  sleep 0.2
  timeout=$((timeout - 1))
done
if ! ip link show wg0 >/dev/null 2>&1; then
  echo "ERROR: wg0 never appeared" >&2
  exit 1
fi

# 5) Bring wg0 up at L2/L3 (Address= is already in wg0.conf) 
ip link set up dev wg0

# 6) Launch the UI proxy (socket‑activated FD 4 → localhost:51821)
socat -d -d ACCEPT:4,fork TCP:127.0.0.1:51821 &

# 7) Finally, wait on the UI server process
wait