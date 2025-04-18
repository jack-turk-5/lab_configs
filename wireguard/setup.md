## Instructions for running WireGuard as a Systemd container with Rootless Podman, Slirp4netns, and WG-Easy

- Based loosely on [Using WireGuard Easy with Rootless Podman](https://github.com/wg-easy/wg-easy/wiki/Using-WireGuard-Easy-with-rootless-Podman-%28incl.-Kubernetes-yaml-file-generation%29)

#### Root Tasks
- Create a file `/etc/modules-load.d/wg-easy.conf` with the following content:
```
wireguard
nft_masq
ip_tables
iptable_filter
iptable_nat
xt_MASQUERADE
```
- Reload root daemon `sudo systemctl daemon-reload`

#### User Tasks
- Containerfile goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually start container `systemctl --user start wireguard`