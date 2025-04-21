#!/usr/bin/env sh
set -e
wg-quick up wg0
socat -u FD:3 TUN:0.0.0.0/0,tun-name=wg0,tun-type=tun,iff-up &
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
exec node /app/server.js