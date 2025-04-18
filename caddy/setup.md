## Instructions to run Caddy as a Reverse Proxy Rootlessly with Podman

#### Root Tasks
- Add `net.ipv4.ip_unprivileged_port_start = 80` to the bottom of `/etc/sysctl.conf` (needs sudo)
- Reboot

#### User Tasks
- Containerfile goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually start container `systemctl --user start caddy`

#### Caddyfile Info
```
vpn.jackturk.dev {
    # With current slirp setup, local services will be available at host.containers.internal:<port>/endpoint
    reverse_proxy http://host.containers.internal:51821
    encode gzip
}
```