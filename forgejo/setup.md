## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

#### Root Tasks
- Allow SSH on 3022 (make sure `Port 22` is explicitly written out before this in the file) by adding to following to `/etc/ssh/sshd_config`
```bash
# Port 22 # Often commented out, amke sure it is present first
Port 3022
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes
```

-  Delegate to Forgejo for authenticating git SSH requests by adding the following to `/etc/ssh/sshd_config`
```bash
Match LocalPort 3022
  AllowUsers git
  AuthorizedKeysCommand /usr/local/bin/git-ssh-keys %f %k
  AuthorizedKeysCommandUser root
  ForceCommand /usr/local/bin/git-ssh-keys %f %k
  AllowTcpForwarding no
  X11Forwarding no
```
- Stop here to start Forgejo and create user git-ssh in Forgejo
- Add a file to hold git-ssh user login token
```bash
sudo mkdir -p /etc/forgejo
sudo nano /etc/forgejo/git-ssh # Copy access token for git-ssh to this file
sudo chown root:root /etc/forgejo/git-ssh
sudo chmod 0400 /etc/forgejo/git-ssh # Make file readonly
```
- Copy `git-ssh-keys` to `/usr/local/bin/git-ssh-keys`
```bash
sudo chown root:root /usr/local/bin/git-ssh-keys
 sudo chmod 0755 /usr/local/bin/git-ssh-keys # Make executable
```
- Add a minimally scoped git user to facilitate SSH
```bash
sudo mkdir -p /var/lib/git
sudo chown git:git /var/lib/git
sudo chmod 0755 /var/lib/git
sudo adduser --system \
            --group \
            --home /var/lib/git \
            --shell /usr/sbin/nologin \
            git
```

#### User Tasks
- Socket and `ssh.service` goes under `~/.config/systemd/user/`
- `forgejo.container` goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now forgejo.sockt`
- Manually start container `systemctl --user start forgejo`