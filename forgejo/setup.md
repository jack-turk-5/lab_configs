## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

#### User Tasks
- Socket goes under `~/.config/systemd/user/`
- `forgejo.container` goes under `~/.config/containers/systemd/`
- Build Containerfile with `podman build -t localhost/forgejo/forgejo-custom:latest .`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now forgejo.socket`
- Manually start container `systemctl --user start forgejo`