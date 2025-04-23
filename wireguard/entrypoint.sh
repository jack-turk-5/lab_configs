#!/usr/bin/env sh
set -e
cd /etc/wireguard
if [ ! -f privatekey ]; then
  umask 077                       
  wg genkey | tee privatekey | wg pubkey > publickey
  echo "[Interface]" > wg0.conf
  echo "PrivateKey = $(cat privatekey)"  >> wg0.conf
  echo "ListenPort = 51820"             >> wg0.conf
  echo "Address = 10.8.0.1/24"          >> wg0.conf
fi

exec boringtun -f --interface wg0 --config wg0.conf