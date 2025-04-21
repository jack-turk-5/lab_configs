#!/usr/bin/env sh
set -e
wg-quick up wg0
socat -u FD:3 GENERIC:PF_PACKET:SOCK_DGRAM:protocol=htons,interface=wg0 &
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
exec node /app/server.js