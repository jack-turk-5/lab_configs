## Setup WireGuard Pro

#### User Tasks
- Container quadlet goes under `~/.config/containers/systemd/`
- Socket goes under `~/.config/systemd/user/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Run `make deploy` from this directory (first time startup) or `make upgrade` (if already running)