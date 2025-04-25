#!/usr/bin/env sh
set -e

ensure_blocking() {
  sleep 1s
  echo -e "\nEnsuring container continuation."
  local logdir="${WGDASH}/src/log"
  latestErrLog=$(find "$logdir" -name "error_*.log" -type f -print | sort -r | head -n 1)
  if [ -n "$latestErrLog" ]; then
    tail -f "$latestErrLog" & wait $!
  else
    echo "No log files found to tail. Something went wrong, exiting..."
  fi
}

# ----- TERM Trap for graceful shutdown -----
trap 'echo "[WGDashboard] Stopping..."; bash "$WGDASH/src/wgd.sh" stop; exit 0' SIGTERM

# ----- Paths & Config -----
WGDASH=/opt/wireguarddashboard         # upstream application root
CONFIG=/data/wg-dashboard.ini

echo "------------------------- START ----------------------------"
echo "Starting WireGuard Dashboard container..."
export WG_QUICK_USERSPACE_IMPLEMENTATION=boringtun-cli
export WG_SUDO=1

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
cd "$WGDASH/src"
sed -i 's/^daemon = True/daemon = False/' gunicorn.conf.py

echo "→ Launching WGDashboard (Gunicorn → FD 3)…"
export GUNICORN_CMD_ARGS="--bind=fd://3 --daemon False --workers=${GUNICORN_WORKERS:-4}"
sed -i 's|sudo "\$venv_gunicorn|"\$venv_gunicorn|g' "$WGDASH/src/wgd.sh"
bash ./wgd.sh start
ensure_blocking