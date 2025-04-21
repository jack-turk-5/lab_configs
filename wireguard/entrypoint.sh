#!/usr/bin/env sh
set -e
wg-quick up wg0
socat -u UDP6-LISTEN:51820,bind=[::],reuseaddr,fork,ipv6only=0 FD:3 &
socat FD:4,fork,reuseaddr TCP6:[::]:51821 &
exec node server.js