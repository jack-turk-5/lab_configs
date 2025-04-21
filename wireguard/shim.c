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
#include <net/if.h>              // struct ifreq, IFF_* flags :contentReference[oaicite:2]{index=2}
#include <linux/if_tun.h>        // TUNSETIFF, IFF_TUN, IFF_NO_PI :contentReference[oaicite:3]{index=3}
#include <stdint.h>
#include "wireguard.h"           // embeddable‑wg‑library API :contentReference[oaicite:4]{index=4}

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME       "wg0"
#define BUF_SIZE      65536

// 1) Manual socket activation (svc passes LISTEN_FDS ≥1) :contentReference[oaicite:5]{index=5}
static int parse_activation(void) {
    char *fds_env = getenv("LISTEN_FDS");
    int fds = fds_env ? atoi(fds_env) : 0;
    unsetenv("LISTEN_FDS");
    unsetenv("LISTEN_PID");
    return (fds >= 1) ? 3 : -1;
}

// 2) Create WireGuard interface (kernel module) or skip if already exists
static int create_wg0(void) {
    int ret = wg_add_device(IF_NAME);              // netlink: ip link add wg0 type wireguard :contentReference[oaicite:6]{index=6}
    int err = errno;
    if (ret != 0 && err != EEXIST) {               // EEXIST means wg0 already existed 
        fprintf(stderr, "ERROR: wg_add_device failed (%d: %s)\n", err, strerror(err));
        return -1;
    }
    if (ret == 0) {
        fprintf(stderr, "[DEBUG] wg0 created\n");
    } else {
        fprintf(stderr, "[DEBUG] wg0 already exists, continuing\n");
    }
    return 0;
}

// 3) Configure keys & port via libwg, then apply
static int configure_wg(void) {
    struct wg_device *dev = NULL;
    if (wg_get_device(&dev, IF_NAME) != 0 || !dev) {  // retrieve handle :contentReference[oaicite:7]{index=7}
        return -1;
    }
    // Private key from env (base64)
    char *b64 = getenv("WG_PRIVATE_KEY");
    if (!b64 || wg_key_from_base64(dev->private_key, b64) != 0) {
        wg_free_device(dev);
        return -1;
    }
    dev->flags |= WGDEVICE_HAS_PRIVATE_KEY;

    // Optional listen port
    char *port_env = getenv("WG_LISTEN_PORT");
    if (port_env) {
        int p = atoi(port_env);
        if (p>0 && p<=65535) {
            dev->listen_port = (uint16_t)p;
            dev->flags |= WGDEVICE_HAS_LISTEN_PORT;
        }
    }

    if (wg_set_device(dev) != 0) {                  // netlink: wg setconf :contentReference[oaicite:8]{index=8}
        wg_free_device(dev);
        return -1;
    }
    wg_free_device(dev);
    return 0;
}

// 4) Open TUN only after wg0 exists, to avoid name collision
static int open_tun(const char *ifname) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEVICE, O_RDWR);
    if (fd < 0) { perror("open /dev/net/tun"); exit(EXIT_FAILURE); }
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) { perror("ioctl(TUNSETIFF)"); close(fd); exit(EXIT_FAILURE); }
    fprintf(stderr, "[DEBUG] TUN %s opened on fd %d\n", ifr.ifr_name, fd);
    return fd;
}

int main(void) {
    // Acquire UDP socket
    int udp_fd = parse_activation();
    if (udp_fd < 0) {
        fprintf(stderr, "ERROR: No socket activated\n");
        return EXIT_FAILURE;
    }

    // Create & configure wg0
    if (create_wg0() < 0 || configure_wg() < 0) {
        fprintf(stderr, "ERROR: WireGuard setup failed\n");
        return EXIT_FAILURE;
    }

    // Assign interface address and bring link up
    if (getenv("WG_ADDRESS")) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "ip address add dev %s %s", IF_NAME, getenv("WG_ADDRESS"));
        system(cmd);
    }
    system("ip link set up dev " IF_NAME);

    // Open TUN after wg0 exists
    int tun_fd = open_tun(IF_NAME);

    // Packet loop: TUN <-> UDP
    struct pollfd fds[2] = {
        { .fd = tun_fd, .events = POLLIN },
        { .fd = udp_fd, .events = POLLIN }
    };
    unsigned char buf[BUF_SIZE];
    while (1) {
        if (poll(fds, 2, -1) < 0 && errno!=EINTR) { perror("poll"); break; }
        if (fds[0].revents & POLLIN) {
            ssize_t n = read(tun_fd, buf, BUF_SIZE);
            if (n>0 && send(udp_fd, buf, n, 0)<0) perror("send udp");
        }
        if (fds[1].revents & POLLIN) {
            ssize_t n = recv(udp_fd, buf, BUF_SIZE, 0);
            if (n>0 && write(tun_fd, buf, n)<0) perror("write tun");
        }
    }

    close(tun_fd);
    close(udp_fd);
    return EXIT_SUCCESS;
}