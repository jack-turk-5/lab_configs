#!/usr/bin/env sh
set -e
wg-quick up wg0
socat -u FD:3 FD:3 &
socat ACCEPT:4,fork TCP6:[::1]:51821 &
exec node server.js