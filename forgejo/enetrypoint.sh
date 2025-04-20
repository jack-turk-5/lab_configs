#!/bin/sh
set -e
socat TCP-LISTEN:22,reuseaddr,fork FD:3 &
exec /usr/local/bin/entrypoint "$@"