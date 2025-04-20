## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

#### Root Tasks
- Allow SSH on 222
```bash
Port 222
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes

```

-  Delegate to Forgejo for authenticating git SSH requests
```bash
Match User git
  AllowTCPForwarding no
  X11Forwarding no
  ForceCommand /usr/local/bin/gitea keys -e git -u %u -t %t -k %k
  AuthorizedKeysCommand /usr/local/bin/gitea keys -e git -u %u -t %t -k %k
  AuthorizedKeysCommandUser git
```

#### User Tasks
- Socket and `ssh.service` goes under `~/.config/systemd/user/`
- `forgejo.container` goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now forgejo.sockt`
- Manually enable the ssh service `systemctl --user enable --now ssh.service`
- Manually start container `systemctl --user start forgejo`