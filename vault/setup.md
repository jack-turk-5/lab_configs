## Setup VaultWarden

#### User Tasks
- Set `DOMAIN` in env file at `~/.config/vault/env`
- Create `ADMIN_TOKEN` podman secret to access admin dashboard
    - [Create a Podman Secret](https://docs.podman.io/en/latest/markdown/podman-secret-create.1.html#examples) 
- Container quadlet goes under `~/.config/containers/systemd/`
- Make sure lingering is enabled (see `../podman/setup.md`)
- Reload systemd user daemon `systemctl --user daemon-reload`
- Manually start container `systemctl --user start vault`