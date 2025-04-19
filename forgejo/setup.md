## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

#### User Tasks
- Sockets go under `~/.config/systemd/user/`
- Containerfile goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now caddy-http.socket caddy-https.socket`
- Manually start container `systemctl --user start caddy`