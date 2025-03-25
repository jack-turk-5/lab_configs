## Instructions to run wireguard as a systemd container with rootless podman and wg-easy

- Based loosely on [Using WireGuard Easy with rootless Podman](https://github.com/wg-easy/wg-easy/wiki/Using-WireGuard-Easy-with-rootless-Podman-%28incl.-Kubernetes-yaml-file-generation%29)

- Containerfile goes under `~/.config/containers/systemd/`
- Need to create directories `~/.config/containers/volumes/wireguard/` and `~/.config/systemd/user/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually start container `systemctl --user start wireguard`
- Run `podman run --rm -it ghcr.io/wg-easy/wg-easy wgpw <password>` once to obtain passwork hash and put result in `~/.config/containers/systemd/env/wg.env`