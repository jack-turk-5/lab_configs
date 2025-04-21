#!/usr/bin/env sh
set -e
socat FD:3 udp:127.0.0.1:51820 &
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
wg-quick up wg0
exec node server.js