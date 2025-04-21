#!/usr/bin/env sh
set -e
wg-quick up wg0
socat ACCEPT:3,fork TCP:127.0.0.1:51821 &
exec node server.js