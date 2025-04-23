#!/usr/bin/env sh
set -e
cd /etc/wireguard

# 1) Generate keys & config if missing
if [ ! -f privatekey ]; then
  umask 077
  wg genkey | tee privatekey | wg pubkey > publickey
  cat > wg0.conf <<EOF
[Interface]
PrivateKey = $(cat privatekey)
ListenPort  = 51820
EOF
fi

# 2) Create and bring up the TUN device
ip tuntap add dev wg0 mode tun
ip link set dev wg0 up

# 3) Launch BoringTun without dropping privileges
#    (disable-drop-privileges or WG_SUDO=1 ensures no getlogin error)
WG_SUDO=1 boringtun -f wg0 &

# 4) Push the WireGuard config into the running tunnel
wg setconf wg0 /etc/wireguard/wg0.conf

# 5) Wait on boringtun so the container stays alive
wait