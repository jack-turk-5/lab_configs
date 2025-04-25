#!/usr/bin/env sh
set -e

# ----- TERM Trap for graceful shutdown -----
trap 'echo "[WGDashboard] Stopping..."; bash "$WGDASH/src/wgd.sh" stop; exit 0' SIGTERM

# ----- Paths & Config -----
WGDASH=/opt/wireguarddashboard         # upstream application root
CONFIG=/data/wg-dashboard.ini

echo "------------------------- START ----------------------------"
echo "Starting WireGuard Dashboard container..."

# 1) replicate ensure_installation()
echo "→ Installing/updating WGDashboard…"
chmod +x "$WGDASH/src/wgd.sh"
cd "$WGDASH/src"
sed -i '/clear/d' ./wgd.sh                                  # remove clears for logging :contentReference[oaicite:3]{index=3}

mkdir -p /data/db
ln -sf /data/db ./db
touch "$CONFIG"
ln -sf "$CONFIG" ./wg-dashboard.ini

python3 -m venv venv                                        # create venv
mv /usr/lib/python3.12/site-packages/psutil* venv/lib/python3.12/site-packages/
mv /usr/lib/python3.12/site-packages/bcrypt* venv/lib/python3.12/site-packages/

./wgd.sh install                                           # install Python deps, WireGuard tools, etc. :contentReference[oaicite:4]{index=4}

# 2) replicate set_envvars()
echo "→ Setting environment variables…"
if [ ! -s "$CONFIG" ]; then
  cat > "$CONFIG" <<EOF
[Peers]
peer_global_dns = ${global_dns:-1.1.1.1}
remote_endpoint = ${public_ip:-$(curl -fs ifconfig.me)}
peer_endpoint_allowed_ip = 0.0.0.0/0

[Server]
app_port = ${wgd_port:-10086}
EOF
else
  sed -i "s|^peer_global_dns = .*|peer_global_dns = ${global_dns:-1.1.1.1}|" "$CONFIG"
  sed -i "s|^remote_endpoint = .*|remote_endpoint = ${public_ip}|"      "$CONFIG"
  sed -i "s|^app_port = .*|app_port = ${wgd_port:-10086}|"               "$CONFIG"
fi

# 3) Bring up wg0 via Boringtun
echo "→ Starting Boringtun userspace VPN…"
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

boringtun-cli --foreground wg0 &                              # spawn Boringtun :contentReference[oaicite:5]{index=5}
sed -i 's/^daemon = True/daemon = False/' gunicorn.conf.py

# Override bind address in config (must match FD 3) :contentReference[oaicite:6]{index=6}
# If your gunicorn.conf.py has a bind setting, replace it; otherwise this is a no-op
sed -i 's|^bind *=.*|bind = "fd://3"|' gunicorn.conf.py || true

echo "→ Launching WGDashboard (Gunicorn → FD 3)…"
bash ./wgd.sh start