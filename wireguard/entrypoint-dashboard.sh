#!/usr/bin/env sh
set -e
socat FD:3 TCP-LISTEN:10086,reuseaddr,fork &
exec bash ./wgd.sh start