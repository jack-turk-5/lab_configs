#!/bin/bash
# Allow essential ICMP traffic
iptables -A INPUT -p icmp -j ACCEPT
iptables -A OUTPUT -p icmp -j ACCEPT

# Allow WireGuard ports
iptables -A INPUT -p udp --dport 51820 -j ACCEPT
iptables -A INPUT -p tcp --dport 51821 -j ACCEPT

# Allow established and related connections in FORWARD
iptables -A FORWARD -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT

# NAT rule for VPN client traffic from the 10.8.0.0/24 subnet out via ens5
iptables -t nat -A POSTROUTING -s 10.8.0.0/24 -o ens5 -j MASQUERADE

# Allow forwarding for the VPN client subnet
iptables -A FORWARD -s 10.8.0.0/24 -j ACCEPT
iptables -A FORWARD -d 10.8.0.0/24 -j ACCEPT