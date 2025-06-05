## Instructions for running Forgejo for VCS/Artifactory with Rootless Podman and Sockets

- Use [this guide](https://unpipeetaulit.fr/en/posts/forgejo_podman/)
- Make sure `XDG_RUNTIME_DIR` is pointed at the right uid/gid in .shrc
- Make sure that `sudo loginctl enable-linger git` is run on root
    - Reboot after enabling linger
- Use the following modified script for setting up ssh-shell (add XDG_RUNTIME_DIR)
- To apply changes once default shell is set to ssh-shell, run `sudo su - git -s /usr/bin/zsh`
```bash
#!/bin/sh
export XDG_RUNTIME_DIR="/run/user/$(id -u)"
/usr/bin/podman exec \
  --interactive \
  --env SSH_ORIGINAL_COMMAND="$SSH_ORIGINAL_COMMAND" \
  forgejo \
    sh "$@"
```