#!/bin/sh
set -e

# 1) Bring up your WireGuard interface
wg-quick up wg0

# 2) Start socket proxies in the background
socat UDP-LISTEN:51820,reuseaddr,fork FD:3 &
socat TCP-LISTEN:'[::]':51821,reuseaddr,fork FD:4 &

# 3) Finally exec the UI, inheriting FDs 3 & 4
exec node server.js