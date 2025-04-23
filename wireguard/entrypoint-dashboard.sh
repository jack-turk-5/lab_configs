#!/usr/bin/env sh
set -e
socat FD:3 TCP:127.0.0.1:10086 & 
exec bash entrypoint.sh