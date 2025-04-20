## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

#### Root Tasks
- Allow SSH on 222 (make sure `Port 22` is explicitly written out before this in the file) by adding to following to `/etc/ssh/sshd_config`
```bash
# Port 22 # Often commented out, amke sure it is present first
Port 222
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes

```

-  Delegate to Forgejo for authenticating git SSH requests by adding the following to `/etc/ssh/sshd_config`
```bash
Match LocalPort 222
  AllowUsers git
  AuthorizedKeysCommand /usr/local/bin/gitea keys -e git -u %u -t %t -k %k
  AuthorizedKeysCommandUser root
  ForceCommand         /usr/local/bin/gitea keys -e git -u %u -t %t -k %k
  AllowTcpForwarding   no
  X11Forwarding        no
```
- Download Gitea CLI so that ssh keys can be validated
```
wget -O gitea https://github.com/go-gitea/gitea/releases/download/v1.23.4/gitea-1.23.4-linux-amd64
chmod +x gitea
sudo mv gitea /usr/local/bin/
sudo chown root:root /usr/local/bin/gitea
sudo chmod 0755   /usr/local/bin/gitea
```

#### User Tasks
- Socket and `ssh.service` goes under `~/.config/systemd/user/`
- `forgejo.container` goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now forgejo.sockt`
- Manually enable the ssh service `systemctl --user enable --now ssh.service`
- Manually start container `systemctl --user start forgejo`