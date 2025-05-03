## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

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