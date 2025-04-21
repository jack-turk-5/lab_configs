#!/usr/bin/env sh
set -e

# WireGuard interface up
wg-quick up wg0

# FD 3: UDP datagram socket (WireGuard handshake)
# Forward incoming datagrams into wg0 and send outgoing via FD 3
socat -u FD:3 UDP4-SENDTO:127.0.0.1:51820 &
socat -u UDP4-LISTEN:51820,reuseaddr,fork FD:3 &

# FD 4: TCP stream socket (UI)
# Forward incoming connections to the local HTTP server on 127.0.0.1:51821
socat TCP4-LISTEN:51821,reuseaddr,fork FD:4 &

# Finally, start the UI (which now listens only on loopback)
exec node /app/server.js