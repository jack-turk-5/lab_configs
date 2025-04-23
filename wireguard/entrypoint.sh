#!/usr/bin/env sh
set -e
cd /etc/wireguard

# 1) Generate keys & base config if missing
if [ ! -f privatekey ]; then
  umask 077
  wg genkey | tee privatekey | wg pubkey > publickey
  cat > wg0.conf <<EOF
[Interface]
PrivateKey = $(cat privatekey)
ListenPort  = 51820
EOF
fi

# 2) Start BoringTun in foreground on wg0
boringtun -f wg0 &

# 3) Assign IP & bring up the tunnel interface
ip addr add 10.8.0.1/24 dev wg0
ip link set wg0 up

# 4) Apply WireGuard config (peers, keys, ports)
wg setconf wg0 wg0.conf

# 5) Keep the container alive
wait