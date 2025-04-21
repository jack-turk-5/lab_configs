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
#include <systemd/sd-daemon.h>   // for sd_listen_fds() :contentReference[oaicite:3]{index=3}
#include "wireguard.h"           // embeddable‑wg-library header

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME      "wg0"
#define BUF_SIZE     65536

static int open_tun(const char *ifname) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun"); // requires CAP_NET_ADMIN & /dev/net/tun mount :contentReference[oaicite:4]{index=4}
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
    // 1) Grab socket(s) from systemd (UDP on FD 3) :contentReference[oaicite:5]{index=5}
    int n_fds = sd_listen_fds(0);
    if (n_fds < 1) {
        fprintf(stderr, "No sockets passed by systemd\n");
        return EXIT_FAILURE;
    }
    int udp_fd = SD_LISTEN_FDS_START;  // FD 3 by convention

    // 2) Create or open wg0
    int tun_fd = open_tun(IF_NAME);

    // 3) Configure kernel WireGuard via libwg (example calls)
    struct wg_device *dev = wg_new_device(IF_NAME);
    if (!dev) {
        fprintf(stderr, "Failed to create wg device\n");
        return EXIT_FAILURE;
    }
    // TODO: wg_set_private_key(dev, ...); wg_add_peer(dev, ...);

    // 4) Poll loop for bidirectional TUN ↔ UDP
    struct pollfd fds[2] = {
        { .fd = tun_fd, .events = POLLIN },
        { .fd = udp_fd,  .events = POLLIN }
    };
    unsigned char buf[BUF_SIZE];

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }
        if (fds[0].revents & POLLIN) {  // TUN → UDP
            ssize_t len = read(tun_fd, buf, BUF_SIZE);
            if (len > 0 && send(udp_fd, buf, len, 0) < 0)
                perror("send udp");
        }
        if (fds[1].revents & POLLIN) {  // UDP → TUN
            ssize_t len = recv(udp_fd, buf, BUF_SIZE, 0);
            if (len > 0 && write(tun_fd, buf, len) < 0)
                perror("write tun");
        }
    }

    wg_free_device(dev);
    close(tun_fd);
    close(udp_fd);
    return EXIT_SUCCESS;
}