#!/usr/bin/env sh
set -e
wg-quick up wg0
socat -u UDP6-LISTEN:51820,bind=[::],reuseaddr,fork,ipv6only=0 FD:3 &
socat TCP6-LISTEN:51821,bind=[::],reuseaddr,fork,ipv6only=0 FD:4 &
exec node server.js