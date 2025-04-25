#!/usr/bin/env sh
set -e
socat FD:3 TCP:0.0.0.0:10086 & 
exec bash entrypoint.sh