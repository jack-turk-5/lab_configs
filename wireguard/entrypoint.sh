#!/usr/bin/env sh
set -e
wg-quick up wg0
socat FD:3 udp:127.0.0.1:51820 &
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
exec node server.js