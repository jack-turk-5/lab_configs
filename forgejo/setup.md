## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

#### Root Tasks
- Allow SSH on 222 (make sure `Port 22` is explicitly written out before this in the file)
```bash
# Port 22 # Often commented out, amke sure it is present first
Port 222
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes

```

-  Delegate to Forgejo for authenticating git SSH requests
```bash
Match LocalPort 222
  AllowUsers git
  PermitRootLogin no
  PasswordAuthentication no
  X11Forwarding no
  AllowTcpForwarding no
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