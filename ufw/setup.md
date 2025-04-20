## Setup UFW and IpTables for Firewall

#### Host Tasks
- `WARNING!` Before touching network settings over ssh run `sudo ufw allow 22/tcp comment 'Allow SSH access'` so you don't get locked out
- Uncomment `net/ipv4/ip_forward=1` in `/etc/ufw/sysctl.conf`
- Uncomment `net.ipv4.ip_forward=1` in `/etc/sysctl.conf`
- Add the following lines to the bottom of `/etc/sysctl.conf`
```conf
net.ipv4.ip_unprivileged_port_start = 80
net.ipv4.tcp_congestion_control = bbr
net.core.rmem_max = 33554432
net.core.wmem_max=33554432
net.ipv4.tcp_rmem="4096 87380 33554432"
net.ipv4.tcp_wmem="4096 65536 33554432"
```
- Set `DEFAULT_FORWARD_POLICY="ACCEPT"` in `/etc/default/ufw`
- Add
```conf
*nat
:POSTROUTING ACCEPT [0:0]
-A POSTROUTING -s 10.8.0.0/24 -o eth0 -j MASQUERADE
COMMIT
```
to the top of /etc/ufw/before.rules
- Copy `user.rules` and `user6.rules` to `/etc/ufw/`
- Disable and re-enable ufw with `sudo ufw enable` and `sudo ufw enable`
- Run `sudo ufw status verbose` to ensure everything worked correctly