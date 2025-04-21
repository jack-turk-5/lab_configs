#!/usr/bin/env sh
set -e
# 1) Bring up wg0 (creates the TUN device)
wg-quick up wg0
# 2) Proxy UDP from FD 3 into existing wg0
# socat -u FD:3 TUN:0.0.0.0/0,tun-name=wg0,tun-type=tun,iff-up &
# 3) Proxy TCP from FD 4 to the UI
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
# 4) Launch Node UI
exec node /app/server.js