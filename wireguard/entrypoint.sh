#!/bin/sh
set -e
# Podman/Conmon hands in exactly 2 sockets: FD 3 (UDP dual‑stack) & FD 4 (TCP dual‑stack)
socat -u FD:3 UDP:[::1]:51820,reuseaddr &
socat FD:4 TCP-LISTEN:51821,reuseaddr,fork &
exec "$@"