#!/usr/bin/env sh
set -e

### Trap SIGTERM for graceful shutdown ###
trap 'echo "[WGDashboard] Stopping..."; kill $gunicorn_pid 2>/dev/null; exit 0' SIGTERM

### 1) Ensure installation (from upstream entrypoint) ###
echo "------------------------- START ----------------------------"
echo "Quick-installing WGDashboard dependencies and venv…"

WGDASH=/opt/wireguarddashboard    # upstream APP root
config_file=/data/wg-dashboard.ini

# Make sure the wgd.sh script is executable, remove 'clear' for logging
chmod +x "$WGDASH/src/wgd.sh"
cd "$WGDASH/src"
sed -i '/clear/d' ./wgd.sh

# Link or create data directories & config
mkdir -p /data/db
ln -sf /data/db ./db
touch "$config_file"
ln -sf "$config_file" ./wg-dashboard.ini

# Create Python venv & relocate psutil/bcrypt into it
python3 -m venv venv
mv /usr/lib/python3.12/site-packages/psutil* venv/lib/python3.12/site-packages/
mv /usr/lib/python3.12/site-packages/bcrypt* venv/lib/python3.12/site-packages/

# Run upstream install (installs Python deps & WireGuard tools)
./wgd.sh install

### 2) Initialize wg-dashboard.ini defaults (from upstream set_envvars) ###
echo "------------- SETTING ENVIRONMENT VARIABLES ----------------"
if [ ! -s "$config_file" ]; then
  cat > "$config_file" <<EOF
[Peers]
peer_global_dns   = ${global_dns:-1.1.1.1}
remote_endpoint   = ${public_ip:-$(curl -s ifconfig.me)}
peer_endpoint_allowed_ip = 0.0.0.0/0

[Server]
app_port = ${wgd_port:-10086}
EOF
else
  # Update DNS if changed
  sed -i "s/^peer_global_dns = .*/peer_global_dns = ${global_dns}/" "$config_file"
  # Update remote_endpoint if set
  sed -i "s/^remote_endpoint = .*/remote_endpoint = ${public_ip}/" "$config_file"
  # Update port if changed
  sed -i "s/^app_port = .*/app_port = ${wgd_port}/" "$config_file"
fi

### 3) Bring up wg0 via Boringtun (userspace WireGuard) ###
echo "Starting Boringtun userspace VPN…"
cd /etc/wireguard
if [ ! -f privatekey ]; then
  umask 077
  wg genkey | tee privatekey | wg pubkey > publickey
  cat > wg0.conf <<EOF
[Interface]
PrivateKey = $(cat privatekey)
ListenPort  = 51820
EOF
fi

boringtun-cli --foreground wg0 &
sleep 0.1

ip addr add ${WG_IP:-10.8.0.1}/24 dev wg0
ip link set wg0 up
wg setconf wg0 wg0.conf

### 4) Activate Python venv and start Gunicorn on FD 3 ###
echo "Launching WGDashboard via Gunicorn on systemd socket (FD 3)…"
. venv/bin/activate

exec gunicorn \
  --config ./gunicorn.conf.py \
  --bind fd://3 \
  --workers ${GUNICORN_WORKERS:-4} \
  --timeout ${GUNICORN_TIMEOUT:-120} \
  wgd_app:app