## This guide is based on the information found here [Rootless Podman in WSL](https://vivekdhami.com/posts/share-wsl-rootless-podman-instance-with-windows/)

#### Follow only the Ubuntu related steps if setting up remote

- Add below definitions to .bashrc (user level)
```bash
export XDG_RUNTIME_DIR=/run/user/$(id -u)
export DBUS_SESSION_BUS_ADDRESS="unix:path=${XDG_RUNTIME_DIR}/bus"
```
- Run sudo loginctl enable-linger <username> 