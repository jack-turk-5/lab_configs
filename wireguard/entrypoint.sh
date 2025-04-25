#!/usr/bin/env sh
set -e

### Trap SIGTERM for graceful shutdown ###
trap 'echo "[WGDashboard] Stopping..."; kill $gunicorn_pid 2>/dev/null; exit 0' SIGTERM

WGDASH=/opt/wireguarddashboard    # upstream app root
CONFIG=/data/wg-dashboard.ini

echo "------------------------- START ----------------------------"
echo "Quick-installing WGDashboard dependencies and venv…"

# 1) Prepare upstream entrypoint logic
chmod +x "$WGDASH/src/wgd.sh"
cd "$WGDASH/src"
# remove any 'clear' calls so logs remain visible
sed -i '/clear/d' ./wgd.sh

# link data dirs & config
mkdir -p /data/db
ln -sf /data/db ./db
touch "$CONFIG"
ln -sf "$CONFIG" ./wg-dashboard.ini

# create a Python venv and relocate heavy deps into it
python3 -m venv venv
mv /usr/lib/python3.12/site-packages/psutil* venv/lib/python3.12/site-packages/
mv /usr/lib/python3.12/site-packages/bcrypt* venv/lib/python3.12/site-packages/

# run upstream install (installs Python deps, wireguard-tools, etc.)
./wgd.sh install

echo "------------- SETTING ENVIRONMENT VARIABLES ----------------"
# 2) Seed or update wg-dashboard.ini defaults
if [ ! -s "$CONFIG" ]; then
  cat > "$CONFIG" <<EOF
[Peers]
peer_global_dns   = ${global_dns:-1.1.1.1}
remote_endpoint   = ${public_ip:-$(curl -fs ifconfig.me)}
peer_endpoint_allowed_ip = 0.0.0.0/0

[Server]
app_port = ${wgd_port:-10086}
EOF
else
  sed -i "s|^peer_global_dns = .*|peer_global_dns = ${global_dns:-1.1.1.1}|" "$CONFIG"
  sed -i "s|^remote_endpoint = .*|remote_endpoint = ${public_ip}| " "$CONFIG"
  sed -i "s|^app_port = .*|app_port = ${wgd_port:-10086}|" "$CONFIG"
fi

echo "Starting Boringtun userspace VPN…"
# 3) Bring up wg0 via Boringtun
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

echo "Launching WGDashboard via Gunicorn on systemd socket (FD 3)…"
# 4) Activate venv and exec Gunicorn bound to FD 3
cd "$WGDASH/src"
. venv/bin/activate

exec gunicorn \
  --config ./gunicorn.conf.py \
  --bind fd://3 \
  --daemon False \
  --workers ${GUNICORN_WORKERS:-4} \
  --timeout ${GUNICORN_TIMEOUT:-120} \
  wsgi_app=dashboard:app