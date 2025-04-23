## Instructions for running WireGuard as a Systemd container with Rootless Podman, Sockets, and WG-Easy

#### Root Tasks
- Copy `wireguard-iptables.service`to `/etc/systemd/system/wireguard-iptables.service`
- Reload root daemon `sudo systemctl daemon-reload`
- Activate the wireguard iptables root service with `sudo systemctl enable --now wireguard-iptables.service`

#### User Tasks
- Socket and network go under `~/.config/systemd/user/`
- `boringtun.container` and `wg-dahboard.container` go under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemctl user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now boringtun.socket wg-dashboard.socket` 
- Manually start containers `systemctl --user start boringtun wg-dashboard`

#### Iptables For Outside Internet VPN
```bash
WG_POST_UP=
# Accept return traffic for established connections
iptables -I FORWARD -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT;
# Allow new traffic from WireGuard -> host network on slirp4netns host‑egress tap
iptables -I FORWARD -i wg0 -o tap+ -j ACCEPT;
# Block VPN‑to‑container subnets (set up separate WG instance for remote access if eventually necessary)
iptables -I FORWARD -i wg0 -d 172.26.0.0/24,172.26.15.0/24 -j REJECT;
# Masquerade (SNAT) VPN egress
iptables -t nat -A POSTROUTING -o tap+ -j MASQUERADE;

WG_POST_DOWN=iptables -D FORWARD -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT;
iptables -D FORWARD -i wg0 -o tap+ -j ACCEPT;
iptables -D FORWARD -i wg0 -d 172.26.0.0/24,172.26.15.0/24 -j REJECT;
iptables -t nat -D POSTROUTING -o tap+ -j MASQUERADE;
```