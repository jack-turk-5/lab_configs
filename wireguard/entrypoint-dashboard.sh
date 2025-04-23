#!/usr/bin/env sh
set -e
socat -d -d ACCEPT:4,fork TCP:127.0.0.1:10086 &
exec bash entrypoint.sh