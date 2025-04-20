## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

#### User Tasks
- Socket and `ssh.service` goes under `~/.config/systemd/user/`
- `forgejo.container` goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now ssh.service`
- Manually start container `systemctl --user start forgejo`