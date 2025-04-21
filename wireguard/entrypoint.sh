#!/usr/bin/env sh
set -e
# 1) Bring up wg0 (creates the TUN device)
wg-quick up wg0
# 3) Proxy TCP from FD 4 to the UI
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
socat -d -d FD:3,rdonly TUN:0.0.0.0/0,tun-name=wg0,tun-type=tun,iff-no-pi,iff-up &
socat -d -d TUN:0.0.0.0/0,tun-name=wg0,tun-type=tun,iff-no-pi,iff-up FD:3,wronly &
# 4) Launch Node UI
exec node /app/server.js