#!/usr/bin/env sh
set -e
cd /etc/wireguard
if [ ! -f privatekey ]; then
  umask 077
  wg genkey | tee privatekey \
    | wg pubkey > publickey
  cat > wg0.conf <<EOF
[Interface]
PrivateKey = $(cat privatekey)
ListenPort  = 51820
Address     = 10.8.0.1/24
EOF
fi
boringtun -f wg0 &
wg setconf wg0 /etc/wireguard/wg0.conf
wait