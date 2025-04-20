#!/bin/sh
set -e
socat -u FD:3 UDP:51820,reuseaddr &
socat FD:4 TCP-LISTEN:51821,reuseaddr,fork &
exec /usr/bin/dumb-init node server.js