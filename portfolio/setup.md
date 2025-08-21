## Setup Portfolio Site

#### User Tasks
- Container quadlet goes under `~/.config/containers/systemd/`
- Socket goes under `~/.config/systemd/user/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually enable the sockets `systemctl --user enable --now portfolio.socket`
- Manually start container `systemctl --user start portfolio`