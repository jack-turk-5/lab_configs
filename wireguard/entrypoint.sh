#!/bin/sh
set -e
# FD 3 = UDP for WireGuard
# FD 4 = TCP for the UI
systemd-socket-activate --fdname=3 wg-quick up wg0
systemd-socket-activate --fdname=4 node server.js