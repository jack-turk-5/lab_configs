# Podman environment variables for rootless systemd user services
set -gx XDG_RUNTIME_DIR /run/user/(id -u)
set -gx DBUS_SESSION_BUS_ADDRESS "unix:path=$XDG_RUNTIME_DIR/bus"
