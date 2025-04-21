// shim.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <systemd/sd-daemon.h>   // for sd_listen_fds() :contentReference[oaicite:0]{index=0}
#include "wireguard.h"           // embeddable‑wg‑library header :contentReference[oaicite:1]{index=1}

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME      "wg0"
#define BUF_SIZE     65536

static int open_tun(const char *ifname) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEVICE, O_RDWR);  // requires CAP_NET_ADMIN & /dev/net/tun mount 
    if (fd < 0) {
        perror("open /dev/net/tun");
        exit(EXIT_FAILURE);
    }
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        exit(EXIT_FAILURE);
    }
    return fd;
}

int main(void) {
    // 1) Grab systemd‑activated sockets (UDP arrives on FD 3) :contentReference[oaicite:2]{index=2}
    int n_fds = sd_listen_fds(0);
    if (n_fds < 1) {
        fprintf(stderr, "No sockets passed by systemd\n");
        return EXIT_FAILURE;
    }
    int udp_fd = SD_LISTEN_FDS_START;

    // 2) Open the TUN device "wg0"
    int tun_fd = open_tun(IF_NAME);

    // 3) Create the kernel WireGuard device
    if (wg_add_device(IF_NAME) != 0) {   // ip link add ... type wireguard :contentReference[oaicite:3]{index=3}
        fprintf(stderr, "Error: wg_add_device failed\n");
        return EXIT_FAILURE;
    }

    // 4) Retrieve a wg_device handle
    struct wg_device *dev = NULL;
    if (wg_get_device(&dev, IF_NAME) != 0 || dev == NULL) {
        fprintf(stderr, "Error: wg_get_device failed\n");
        return EXIT_FAILURE;
    }

    // 5) Populate the private key (base64 from ENV "WG_PRIVATE_KEY")
    char *b64 = getenv("WG_PRIVATE_KEY");
    if (b64) {
        wg_key key;
        if (wg_key_from_base64(key, (wg_key_b64_string)b64) != 0) {
            fprintf(stderr, "Error: invalid private key\n");
            return EXIT_FAILURE;
        }
        memcpy(dev->private_key, key, sizeof(key));
        dev->flags |= WGDEVICE_HAS_PRIVATE_KEY;  // mark that we set it :contentReference[oaicite:4]{index=4}
    }

    // 6) Optional: set listen port from ENV "WG_LISTEN_PORT" (default 51820)
    char *port = getenv("WG_LISTEN_PORT");
    if (port) {
        dev->listen_port = (uint16_t)atoi(port);
        dev->flags |= WGDEVICE_HAS_LISTEN_PORT;  // mark that we set it :contentReference[oaicite:5]{index=5}
    }

    // 7) Apply our configuration to the kernel
    if (wg_set_device(dev) != 0) {   // netlink call to configure WireGuard :contentReference[oaicite:6]{index=6}
        fprintf(stderr, "Error: wg_set_device failed\n");
        wg_free_device(dev);
        return EXIT_FAILURE;
    }
    wg_free_device(dev);

    // 8) Assign the interface IP from ENV "WG_ADDRESS" and bring it up
    char *addr = getenv("WG_ADDRESS");  // e.g. "10.8.0.1/24"
    if (addr) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "ip address add dev %s %s", IF_NAME, addr);
        system(cmd);   // libc system call for ip assignment 
    }
    system("ip link set up dev " IF_NAME);  // bring wg0 up 

    // 9) Poll loop: shuttle packets TUN ↔ UDP
    struct pollfd fds[2] = {
        { .fd = tun_fd, .events = POLLIN },
        { .fd = udp_fd,  .events = POLLIN }
    };
    unsigned char buf[BUF_SIZE];
    while (1) {
        if (poll(fds, 2, -1) < 0) {
            perror("poll");
            break;
        }
        // TUN → UDP
        if (fds[0].revents & POLLIN) {
            ssize_t len = read(tun_fd, buf, BUF_SIZE);
            if (len > 0 && send(udp_fd, buf, len, 0) < 0)
                perror("send udp");
        }
        // UDP → TUN
        if (fds[1].revents & POLLIN) {
            ssize_t len = recv(udp_fd, buf, BUF_SIZE, 0);
            if (len > 0 && write(tun_fd, buf, len) < 0)
                perror("write tun");
        }
    }

    close(tun_fd);
    close(udp_fd);
    return EXIT_SUCCESS;
}