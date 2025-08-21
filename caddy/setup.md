## Instructions to run Caddy as a Reverse Proxy Rootlessly with Podman

#### Root Tasks
- Add `net.ipv4.ip_unprivileged_port_start = 80` to the bottom of `/etc/sysctl.conf` (needs sudo)
- Reboot

#### User Tasks
- Socket goes under `~/.config/systemd/user/`
- Container quadlet goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now caddy.socket`
- Manually start container `systemctl --user start caddy`