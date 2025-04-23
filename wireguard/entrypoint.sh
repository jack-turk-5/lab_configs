#!/usr/bin/env sh
set -e
cd /etc/wireguard

# 1) Generate key-pair & base config if missing
if [ ! -f privatekey ]; then
  umask 077
  wg genkey | tee privatekey | wg pubkey > publickey
  cat > wg0.conf <<EOF
[Interface]
PrivateKey = $(cat privatekey)
ListenPort  = 51820
EOF
fi

# 2) Launch BoringTun, letting it create the TUN device
#    disable-drop-privileges so getlogin() doesn’t fail
WG_SUDO=1 boringtun --foreground wg0 &

# 3) Wait a moment for wg0 to appear
sleep 0.1

# 4) Assign IP & bring up wg0
ip addr add 10.8.0.1/24 dev wg0
ip link set wg0 up

# 5) Push the WireGuard config into wg0
wg setconf wg0 wg0.conf

# 6) Keep the process alive
wait