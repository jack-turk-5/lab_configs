#!/usr/bin/env sh
set -e
socat -u UDP6-LISTEN:51820,bind=[::],reuseaddr,fork,ipv6only=0 UDP:127.0.0.1:51820 &
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
wg-quick up wg0
exec node server.js