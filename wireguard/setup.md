## Instructions for running WireGuard as a Systemd container with Rootless Podman, Slirp4netns, and WG-Easy


#### Root Tasks
- Create a file `/etc/modules-load.d/wg-easy.conf` with the following content:
```bash
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