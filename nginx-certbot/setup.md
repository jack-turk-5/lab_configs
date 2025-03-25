# ENTIRE SERVICE IS TODO WORK IN PROGRESS
`All files have boilerplate code`

## Prerequisites
- build `containerfile.nginx` to `localhost/nginx-rootless:latest`
- build `containerfile.certbot` to `localhost/certbot-rootless:latest`

## One Shot Certbot to Obtain Initial Certs 
```bash
podman run --rm \
-v /path/on/host/letsencrypt:/etc/letsencrypt \
-v /path/on/host/www:/var/www/certbot \
-p 80:8080 \
certbot/certbot:latest certonly \
--webroot -w /var/www/certbot \
-d example.com -d www.example.com \
--email admin@example.com --agree-tos --no-eff-email
```


## Binding to privileged ports
`podman run -d -p 80:8080 -p 443:8443 localhost/nginx-rootless:latest`

## Starting
```bash
podman play kube nginx-certbot/nginx-certbot.yaml \ 
--port 80:8080 \
--port 443:8443
```