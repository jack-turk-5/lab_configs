## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

#### User Tasks
- Make mount directories
    - Run `podman unshare chown -R 1001:100 ~/.config/containers/systemd/forgejo` if running into permissions issues (assuming UID is 1001)
- Socket goes under `~/.config/systemd/user/`
- `forgejo.container` goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now forgejo.sockt`
- Manually start container `systemctl --user start forgejo`