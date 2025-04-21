#!/usr/bin/env sh
set -e
wg-quick --socketactivation up wg0
socat ACCEPT:4,fork TCP:127.0.0.1:51821 &
exec node server.js