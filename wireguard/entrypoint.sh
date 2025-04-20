#!/usr/bin/env sh
set -e

# 1) Bring up WireGuard
wg-quick up wg0

# 2) Only socket‑activate the UDP path into FD 3:
#    Listen on UDP 51820, fork, hand datagrams into FD 3
socat UDP-LISTEN:51820,reuseaddr,fork FD:3 &

# 3) Exec the UI (inherits FD 4 untouched)
exec node server.js