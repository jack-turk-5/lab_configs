#!/usr/bin/env sh
set -e
wg-quick up wg0
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
socat -d -d OPEN:/proc/self/fd/3,rdonly TUN:10.0.10.1/24,tun-name=wg0,tun-type=tun,iff-no-pi,iff-up &
socat -d -d TUN:10.0.10.1/24,tun-name=wg0,tun-type=tun,iff-no-pi,iff-up OPEN:/proc/self/fd/3,wronly &
exec node /app/server.js