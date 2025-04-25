#!/usr/bin/env sh
set -e
socat FD:3 TCP:[::]:10086 & 
exec bash entrypoint.sh