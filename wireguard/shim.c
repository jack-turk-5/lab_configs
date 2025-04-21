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
#include <linux/if_tun.h>        // TUNSETIFF, IFF_TUN, IFF_NO_PI :contentReference[oaicite:7]{index=7}
#include "wireguard.h"           // embeddable‑wg‑library API

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME      "wg0"
#define BUF_SIZE     65536

// 1) Parse socket activation: require LISTEN_FDS ≥1, return FD 3
static int parse_activation(void) {
    char *fds_env = getenv("LISTEN_FDS");
    int fds = fds_env ? atoi(fds_env) : 0;
    unsetenv("LISTEN_FDS");
    unsetenv("LISTEN_PID");
    if (fds < 1) {
        fprintf(stderr, "ERROR: No socket activated (LISTEN_FDS=%d)\n", fds);
        return -1;
    }
    return 3; // first socket FD
}

// 2) Create or skip wg0 interface (ignore EEXIST)
static int create_device(void) {
    int ret = wg_add_device(IF_NAME);               // ip link add wg0 type wireguard :contentReference[oaicite:8]{index=8}
    int e = errno;
    if (ret != 0 && e != EEXIST) {
        fprintf(stderr, "ERROR: wg_add_device failed (%s)\n", strerror(e));
        return -1;
    }
    fprintf(stderr, "[DEBUG] wg0 %s\n", ret==0 ? "created" : "already exists");
    return 0;
}

// 3) Strip unsupported lines and apply config via wg-quick | wg setconf
static int apply_config(void) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "wg-quick strip %s | wg setconf %s -", IF_NAME, IF_NAME);
    int rc = system(cmd);                          // pipeline via /bin/sh :contentReference[oaicite:9]{index=9}
    if (rc != 0) {
        fprintf(stderr, "ERROR: wg setconf returned %d\n", rc);
        return -1;
    }
    fprintf(stderr, "[DEBUG] wg setconf applied\n");
    return 0;
}

// 4) Bring the interface up (IP layer only)
static void setup_link(void) {
    system("ip link set up dev " IF_NAME);         // bring wg0 up :contentReference[oaicite:10]{index=10}
}

// 5) Open the TUN device for packet I/O
static int open_tun(const char *ifname) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEVICE, O_RDWR);
    if (fd < 0) { perror("open /dev/net/tun"); exit(EXIT_FAILURE); }
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "[DEBUG] TUN %s opened on fd %d\n", ifr.ifr_name, fd);
    return fd;
}

int main(void) {
    // Acquire UDP socket (fd=3) via systemd activation
    int udp_fd = parse_activation();
    if (udp_fd < 0) return EXIT_FAILURE;

    // Create WireGuard interface
    if (create_device() < 0) return EXIT_FAILURE;

    // Apply configuration safely (stripped of unsupported lines)
    if (apply_config() < 0)   return EXIT_FAILURE;

    // Bring wg0 up at L2/L3
    setup_link();

    // Open TUN after wg0 exists
    int tun_fd = open_tun(IF_NAME);

    // 6) Bidirectional packet proxy: TUN <-> UDP
    struct pollfd fds[2] = {
      { .fd = tun_fd, .events = POLLIN },
      { .fd = udp_fd,  .events = POLLIN }
    };
    unsigned char buf[BUF_SIZE];
    while (1) {
        if (poll(fds, 2, -1) < 0 && errno != EINTR) {
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