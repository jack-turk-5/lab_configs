#!/usr/bin/env sh
set -e
wg-quick up wg0
socat -u FD:3 TUN:/dev/net/tun,tun-name=wg0,iff-up &
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
exec node /app/server.js