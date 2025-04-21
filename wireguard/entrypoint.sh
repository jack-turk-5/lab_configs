#!/usr/bin/env sh
set -e
wg-quick up wg0
socat - udp:FD:3,sp:58120,fork,reuseaddr &
socat - tcp-listen:51821,fork,reuseaddr &
socat - tcp-connect:[::]:51821
exec node server.js