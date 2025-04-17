## Instructions to run wireguard as a systemd container with rootless podman, pasta, and wg-easy

- Based loosely on [Using WireGuard Easy with rootless Podman](https://github.com/wg-easy/wg-easy/wiki/Using-WireGuard-Easy-with-rootless-Podman-%28incl.-Kubernetes-yaml-file-generation%29)

- Create a file `/etc/modules-load.d/wg-easy.conf` with the following content:
```
wireguard
nft_masq
```
- Containerfile goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Copy wireguard-nat.service to `/etc/systemd/system`
- Reload root daemon `sudo systemctl daemon-reload`
- Enable NAT service `sudo systemctl enable --now host-wireguard-nat.service` 
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually start container `systemctl --user start wireguard`