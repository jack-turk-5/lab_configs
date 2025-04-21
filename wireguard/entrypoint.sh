#!/usr/bin/env sh
set -e

# 1) Clone a new TUN device named wg0 and hook FD 3 into it
socat \
  -u FD:3 \
  TUN:0.0.0.0/0,tun-name=wg0,tun-type=tun,iff-up &

# 2) Load your WG config onto that tunnel
wg setconf wg0 /etc/wireguard/wg0.conf

# 3) Proxy TCP UI as before on FD 4:
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &

# 4) Start the Node UI
exec node /app/server.js