#!/bin/bash
# WGDashboard Entrypoint - with BoringTun userspace WireGuard

# Trap SIGTERM to ensure clean shutdown of WGDashboard
trap 'stop_service' SIGTERM
stop_service() {
    echo "[WGDashboard] Stopping WGDashboard..."
    # Stop the WGDashboard service (gunicorn + app)
    bash ./wgd.sh stop
    exit 0
}

echo "------------------------- START ----------------------------"
echo "Starting WireGuard Dashboard container (using BoringTun)..."

# Ensure /dev/net/tun exists for BoringTun (userspace WireGuard)
if [ ! -c /dev/net/tun ]; then
    echo "[Entrypoint] Creating /dev/net/tun device"
    mkdir -p /dev/net && mknod /dev/net/tun c 10 200 && chmod 600 /dev/net/tun
fi

# Enable IP forwarding for WireGuard (needed for MASQUERADE rules)
sysctl -w net.ipv4.ip_forward=1 >/dev/null 2>&1
sysctl -w net.ipv6.conf.all.forwarding=1 >/dev/null 2>&1

# Path to WGDashboard config file
config_file="/data/wg-dashboard.ini"

ensure_installation() {
    echo "Performing initial setup tasks..."
    chmod +x "${WGDASH}/src/wgd.sh"
    cd "${WGDASH}/src" || exit 1

    # Remove any 'clear' commands from wgd.sh for cleaner logs
    sed -i '/clear/d' ./wgd.sh

    # Ensure persistent data directories exist and are linked
    if [ ! -d "/data/db" ]; then
        echo "Creating persistent database directory /data/db"
        mkdir -p /data/db
    fi
    if [ ! -d "${WGDASH}/src/db" ]; then
        ln -s /data/db "${WGDASH}/src/db"
    fi

    # Ensure the WGDashboard config (.ini) exists in /data and link it
    if [ ! -f "${config_file}" ]; then
        echo "Creating initial wg-dashboard.ini in /data"
        touch "${config_file}"
    fi
    if [ ! -f "${WGDASH}/src/wg-dashboard.ini" ]; then
        ln -s "${config_file}" "${WGDASH}/src/wg-dashboard.ini"
    fi

    # Activate the pre-built Python virtual environment
    echo "Activating Python virtual environment..."
    . "${WGDASH}/src/venv/bin/activate"

    # (Python dependencies are already installed in the image)
    echo "Python venv activated. Skipping runtime pip installation."

    # Run installation steps (except dependency installation) from WGDashboard script
    # This will create default configs (wg0.conf, ssl-tls.ini) if not present.
    ./wgd.sh install -y
    echo "Core installation steps completed."
    
    # Ensure a WireGuard config file exists, else copy the template
    if [ ! -f "/etc/wireguard/wg0.conf" ]; then
        echo "No WireGuard config found. Copying default template to /etc/wireguard/wg0.conf"
        cp -f "${WGDASH}/src/wg0.conf.template" "/etc/wireguard/wg0.conf"
        echo "Generating new private key for wg0..."
        PRIVATE_KEY=$(wg genkey)  # use wg to generate a key
        sed -i "s|^PrivateKey *=.*$|PrivateKey = ${PRIVATE_KEY}|" /etc/wireguard/wg0.conf
        echo "WireGuard default config created with a new private key."
    else
        echo "Existing /etc/wireguard/wg0.conf found; using it as-is."
    fi
}

set_envvars() {
    echo "------------- SETTING ENVIRONMENT VARIABLES -------------"
    # If the config file is empty, initialize it with default sections
    if [ ! -s "${config_file}" ]; then
        echo "Initializing wg-dashboard.ini with default [Peers] and [Server] sections."
        {
          echo "[Peers]"
          echo "peer_global_dns = ${global_dns}"
          echo "remote_endpoint = ${public_ip}"
          echo ""
          echo "[Server]"
          echo "app_port = ${wgd_port}"
        } > "${config_file}"
    else
        echo "Using existing wg-dashboard.ini configuration."
    fi

    # Update the config file with current env vars if they differ
    # Global DNS
    current_dns=$(grep -E "^peer_global_dns" "${config_file}" | awk '{print $NF}')
    if [ -n "${global_dns}" ] && [ "${global_dns}" != "${current_dns}" ]; then
        echo "Updating peer_global_dns to ${global_dns}"
        sed -i "s/^peer_global_dns = .*/peer_global_dns = ${global_dns}/" "${config_file}"
    fi
    # Public endpoint (if none provided, attempt to detect)
    current_endpoint=$(grep -E "^remote_endpoint" "${config_file}" | awk '{print $NF}')
    if [ -z "${public_ip}" ]; then
        default_ip=$(curl -s ifconfig.me || echo "")
        if [ -n "${default_ip}" ]; then
            echo "No public_ip provided. Detected public IP: ${default_ip}"
            sed -i "s/^remote_endpoint = .*/remote_endpoint = ${default_ip}/" "${config_file}"
        fi
    elif [ "${public_ip}" != "${current_endpoint}" ]; then
        echo "Updating remote_endpoint to ${public_ip}"
        sed -i "s/^remote_endpoint = .*/remote_endpoint = ${public_ip}/" "${config_file}"
    fi
    # Dashboard port
    current_port=$(grep -E "^app_port" "${config_file}" | awk '{print $NF}')
    if [ "${wgd_port}" != "${current_port}" ]; then
        echo "Updating app_port to ${wgd_port}"
        sed -i "s/^app_port = .*/app_port = ${wgd_port}/" "${config_file}"
    fi
}

start_core() {
    echo "---------------------- STARTING CORE ----------------------"
    echo "Launching WGDashboard web service (Gunicorn)..."
    bash ./wgd.sh start   # start the dashboard (does NOT bring up wg0 interface)
}

ensure_blocking() {
    echo "Ensuring the container process keeps running (tailing logs)..."
    # Tail the latest error log if it exists, to prevent container exit
    LOGDIR="${WGDASH}/src/log"
    latest_err=$(find "$LOGDIR" -name "error_*.log" -type f -print | sort -r | head -n 1)
    if [ -n "$latest_err" ]; then
        tail -f "$latest_err" & wait ${!}
    else
        echo "No error log found to tail. Exiting..."
    fi
}

# Execute the steps in order
ensure_installation
set_envvars
start_core
ensure_blocking