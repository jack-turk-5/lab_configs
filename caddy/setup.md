## Instructions to run Caddy as a Reverse Proxy Rootlessly with Podman

#### Root Tasks
- Add `net.ipv4.ip_unprivileged_port_start = 80` to the bottom of `/etc/sysctl.conf` (needs sudo)
- Reload sysctl with the the command
```
sudo sysctl --system
``` :contentReference[oaicite:1]{index=1} 
```

#### User Tasks
- Containerfile goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually start container `systemctl --user start caddy`