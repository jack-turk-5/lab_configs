#!/usr/bin/env sh
set -e

# Ensure LISTEN_FDS is inherited even after backgrounding
export LISTEN_FDS

# Start the shim (it grabs FD 3, configures wg0, and backgrounds)
wg-shim &
# UI proxy on FD 4
socat -d -d ACCEPT:4,fork TCP:127.0.0.1:51821 &
# Start your server
exec node /app/server.js