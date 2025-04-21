#!/usr/bin/env
set -e
socat -u FD:3 TUN:0.0.0.0/0,tun-type=tun,tun-name=wg0,iff-up &
until ip link show wg0; do sleep 0.1; done
wg-quick up wg0
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
exec node /app/server.js