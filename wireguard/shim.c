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
#include <stdint.h>
#include "wireguard.h"           // embeddable‑wg‑library API

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME       "wg0"
#define BUF_SIZE      65536

// Open or create the TUN device named IF_NAME
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

// Parse systemd socket activation: expect at least 1 FD
// returns first socket FD (3) or -1 on error
static int parse_activation(void) {
    char *fds_env = getenv("LISTEN_FDS");
    int fds = fds_env ? atoi(fds_env) : 0;
    unsetenv("LISTEN_FDS");
    unsetenv("LISTEN_PID");
    return (fds >= 1) ? 3 : -1;
}

int main(void) {
    // 1) Acquire UDP socket via activation
    int udp_fd = parse_activation();
    if (udp_fd < 0) {
        fprintf(stderr, "ERROR: No socket activated\n");
        return EXIT_FAILURE;
    }

    // 2) Open TUN device wg0
    int tun_fd = open_tun(IF_NAME);

    // 3) Create WireGuard interface (or skip if exists)
    int add_ret = wg_add_device(IF_NAME);
    int err = errno;
    if (add_ret != 0 && err != EEXIST) {
        fprintf(stderr, "ERROR: wg_add_device failed (errno=%d %s)\n", err, strerror(err));
        return EXIT_FAILURE;
    }

    // 4) Retrieve device handle
    struct wg_device *dev = NULL;
    if (wg_get_device(&dev, IF_NAME) != 0 || dev == NULL) {
        fprintf(stderr, "ERROR: wg_get_device failed\n");
        return EXIT_FAILURE;
    }

    // 5) Load private key from env
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

    // 6) Optional: set listen port from env
    char *port_env = getenv("WG_LISTEN_PORT");
    if (port_env) {
        int port = atoi(port_env);
        if (port > 0 && port <= 65535) {
            dev->listen_port = (uint16_t)port;
            dev->flags |= WGDEVICE_HAS_LISTEN_PORT;
        }
    }

    // 7) Apply config
    if (wg_set_device(dev) != 0) {
        fprintf(stderr, "ERROR: wg_set_device failed\n");
        wg_free_device(dev);
        return EXIT_FAILURE;
    }
    wg_free_device(dev);

    // 8) Assign IP and bring interface up
    char *addr = getenv("WG_ADDRESS");
    if (addr) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "ip address add dev %s %s", IF_NAME, addr);
        system(cmd);
    }
    system("ip link set up dev " IF_NAME);

    // 9) Packet loop: TUN <-> UDP
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
            if (n > 0 && send(udp_fd, buf, n, 0) < 0) perror("send udp");
        }
        if (fds[1].revents & POLLIN) {
            ssize_t n = recv(udp_fd, buf, BUF_SIZE, 0);
            if (n > 0 && write(tun_fd, buf, n) < 0) perror("write tun");
        }
    }

    close(tun_fd);
    close(udp_fd);
    return EXIT_SUCCESS;
}