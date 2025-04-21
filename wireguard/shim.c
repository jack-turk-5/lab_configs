// shim.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <net/if.h>              // struct ifreq, IFF_* flags
#include <linux/if_tun.h>        // TUNSETIFF, IFF_TUN, IFF_NO_PI
#include "wireguard.h"           // embeddable‑wg‑library API

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME      "wg0"
#define BUF_SIZE     65536

static int open_tun(const char *ifname) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEVICE, O_RDWR);
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

static int parse_activation() {
    // Manual systemd socket activation parser:
    // LISTEN_PID must match our pid, LISTEN_FDS >=1
    char *pid_env = getenv("LISTEN_PID");
    char *fds_env = getenv("LISTEN_FDS");
    if (!pid_env || !fds_env) {
        fprintf(stderr, "ERROR: LISTEN_PID or LISTEN_FDS not set\n");
        return -1;
    }
    pid_t pid = (pid_t)atoi(pid_env);
    int fds = atoi(fds_env);
    // clear to avoid child inheritance confusion
    unsetenv("LISTEN_PID");
    unsetenv("LISTEN_FDS");
    return 3;  // first activated FD is 3
}

int main(void) {
    // 1) Acquire UDP socket (FD 3) via socket activation
    int udp_fd = parse_activation();
    if (udp_fd < 0) {
        return EXIT_FAILURE;
    }

    // 2) Open or create the TUN device wg0
    int tun_fd = open_tun(IF_NAME);

    // 3) Create the kernel WireGuard interface
    if (wg_add_device(IF_NAME) != 0) {
        fprintf(stderr, "ERROR: wg_add_device failed\n");
        return EXIT_FAILURE;
    }

    // 4) Retrieve the device handle
    struct wg_device *dev = NULL;
    if (wg_get_device(&dev, IF_NAME) != 0 || dev == NULL) {
        fprintf(stderr, "ERROR: wg_get_device failed\n");
        return EXIT_FAILURE;
    }

    // 5) Load private key from WG_PRIVATE_KEY (base64)
    char *b64 = getenv("WG_PRIVATE_KEY");
    if (!b64) {
        fprintf(stderr, "ERROR: WG_PRIVATE_KEY not set\n");
        wg_free_device(dev);
        return EXIT_FAILURE;
    }
    wg_key key;
    if (wg_key_from_base64(key, b64) != 0) {
        fprintf(stderr, "ERROR: invalid private key\n");
        wg_free_device(dev);
        return EXIT_FAILURE;
    }
    memcpy(dev->private_key, key, sizeof(key));
    dev->flags |= WGDEVICE_HAS_PRIVATE_KEY;

    // 6) Optional: set listen port from WG_LISTEN_PORT
    char *port_env = getenv("WG_LISTEN_PORT");
    if (port_env) {
        int port = atoi(port_env);
        if (port > 0 && port <= 65535) {
            dev->listen_port = (uint16_t)port;
            dev->flags |= WGDEVICE_HAS_LISTEN_PORT;
        }
    }

    // 7) Apply WireGuard configuration
    if (wg_set_device(dev) != 0) {
        fprintf(stderr, "ERROR: wg_set_device failed\n");
        wg_free_device(dev);
        return EXIT_FAILURE;
    }
    wg_free_device(dev);

    // 8) Assign interface address from WG_ADDRESS and bring wg0 up
    char *addr = getenv("WG_ADDRESS"); // e.g. "10.8.0.1/24"
    if (addr) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "ip address add dev %s %s", IF_NAME, addr);
        system(cmd);
    }
    system("ip link set up dev " IF_NAME);

    // 9) Poll loop: shuttle packets between TUN and UDP socket
    struct pollfd fds[2] = {
        { .fd = tun_fd, .events = POLLIN },
        { .fd = udp_fd,  .events = POLLIN }
    };
    unsigned char buf[BUF_SIZE];

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret < 0 && errno != EINTR) {
            perror("poll");
            break;
        }
        if (fds[0].revents & POLLIN) {
            ssize_t n = read(tun_fd, buf, BUF_SIZE);
            if (n > 0 && send(udp_fd, buf, n, 0) < 0)
                perror("send udp");
        }
        if (fds[1].revents & POLLIN) {
            ssize_t n = recv(udp_fd, buf, BUF_SIZE, 0);
            if (n > 0 && write(tun_fd, buf, n) < 0)
                perror("write tun");
        }
    }

    close(tun_fd);
    close(udp_fd);
    return EXIT_SUCCESS;
}