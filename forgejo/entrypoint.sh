#!/bin/sh
set -e
socat TCP-LISTEN:222,reuseaddr,fork FD:3 &
exec /usr/bin/dumb-init -- /usr/local/bin/docker-entrypoint.sh "$@"