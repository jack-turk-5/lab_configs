#!/usr/bin/env sh
set -e

# WireGuard interface up
wg-quick up wg0

# Inherited FD 3 (host’s port 51820) → local WireGuard interface
socat -u FD:3 UDP4-SENDTO:127.0.0.1:51820 &

# New listener on port 51820, using both REUSEADDR and REUSEPORT
socat -u UDP4-LISTEN:51820,reuseaddr,reuseport,fork FD:3 &

# FD 4: TCP stream socket (UI)
# Forward incoming connections to the local HTTP server on 127.0.0.1:51821
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &

# Finally, start the UI (which now listens only on loopback)
exec node /app/server.js