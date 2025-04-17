## Instructions to run wireguard as a systemd container with rootless podman and wg-easy

- Based loosely on [Using WireGuard Easy with rootless Podman](https://github.com/wg-easy/wg-easy/wiki/Using-WireGuard-Easy-with-rootless-Podman-%28incl.-Kubernetes-yaml-file-generation%29)

- Containerfile goes under `~/.config/containers/systemd/`
- Need to create directories `~/.config/containers/volumes/wireguard/`, `~/.config/systemd/user/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually start container `systemctl --user start wireguard`
- Run `podman run --rm -it ghcr.io/wg-easy/wg-easy wgpw <password>` once to obtain passwork hash and put result in `~/.config/containers/systemd/env/wg.env`
- Copy `iptables.conf` to `/etc/modules-load.d/` so that the necessary modules autoload
- Copy `iptables-firewall.sh` to `/usr/local/bin/` and ensure it is executable
- Copy `iptables-firewall.service` to `/etc/systemd/system/` so that firewall rules are automatically applied
- Set the following value in `/etc/containers/containers.conf`
```
[engine]
network_backend = "netavark"
```
- Copy `podman-wireguard.service` to `/etc/systemd/system/`
- Restart system


# Not 100% rootless :(
slirp4netns is NAT‑only: By design, slirp4netns (the default rootless network mode) masquerades egress but does not support inbound DNAT for UDP, so published ports bind only on 127.0.0.1 and aren’t reachable externally

Pasta defaults to no NAT: Since Podman 5.0, Pasta replaced slirp4netns as the default rootless CNI but still offers only outbound masquerading when invoked with port‑mapping flags (-t), not a full routing table for inbound UDP

No UDP in systemd‑socket‑proxyd: Even systemd’s socket proxy can’t help directly—systemd‑socket‑proxyd supports only stream sockets (TCP) and lacks UDP datagram forwarding