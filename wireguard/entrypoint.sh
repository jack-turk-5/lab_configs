#!/usr/bin/env sh
set -e

# 1) Forward systemd socket FD 3 into the dashboard’s HTTP port
#    Use 0.0.0.0 so host.containers.internal can reach it
socat -d -d \
  ACCEPT-FD:3,fork \
  TCP:127.0.0.1:${wgd_port:-10086},retry,interval=0.5,forever &

# 3) Bring up wg0 via Boringtun
cd /etc/wireguard

# Generate keys & a basic wg0.conf if missing
if [ ! -f privatekey ]; then
  umask 077
  wg genkey | tee privatekey | wg pubkey > publickey
  cat > wg0.conf <<EOF
[Interface]
PrivateKey = $(cat privatekey)
ListenPort  = 51820
EOF
fi

# Launch Boringtun (userspace TUN device)
boringtun-cli --foreground wg0 &
sleep 0.1

# Configure interface
ip addr add ${WG_IP:-10.8.0.1}/24 dev wg0
ip link set wg0 up
wg setconf wg0 wg0.conf

# 4) Exec the original WGDashboard entrypoint (installs & starts HTTP service)
exec /entrypoint.sh