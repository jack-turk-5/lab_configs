#!/usr/bin/env sh
set -e
wg-quick up wg0
socat -u UDP6-LISTEN:51820,bind=[::],reuseaddr,fork,ipv6only=0 UDP:127.0.0.1:51820 &
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
exec node server.js