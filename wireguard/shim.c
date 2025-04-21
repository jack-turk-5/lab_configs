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

#define TUN_DEV    "/dev/net/tun"
#define IFACE      "wg0"
#define BUF_SIZE   65536

// 1) Parse systemd socket activation: require LISTEN_FDS ≥1, return FD 3
static int parse_activation(void) {
    char *e = getenv("LISTEN_FDS");
    int fds = e ? atoi(e) : 0;
    unsetenv("LISTEN_FDS"); unsetenv("LISTEN_PID");
    if (fds < 1) {
        fprintf(stderr, "ERROR: No socket activated (LISTEN_FDS=%d)\n", fds);
        return -1;
    }
    return 3;
}

// 2) Create wg0 idempotently (ignore EEXIST)
static int create_device(void) {
    int r = wg_add_device(IFACE);                  // ip link add wg0 type wireguard :contentReference[oaicite:2]{index=2}
    int err = errno;
    if (r != 0 && err != EEXIST) {                  // only fatal on unexpected errors 
        fprintf(stderr, "ERROR: wg_add_device failed (%s)\n", strerror(err));
        return -1;
    }
    fprintf(stderr, "[DEBUG] %s %s\n", IFACE, r==0? "created":"already exists");
    return 0;
}

// 3) Strip helper directives and apply config
static int apply_config(void) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "wg-quick strip %s | wg setconf %s -", IFACE, IFACE);
    int rc = system(cmd);                          // pipeline via shell :contentReference[oaicite:3]{index=3}
    if (rc != 0) {
        fprintf(stderr, "ERROR: wg setconf returned %d\n", rc);
        return -1;
    }
    fprintf(stderr, "[DEBUG] wg setconf applied\n");
    return 0;
}

// 4) Bring interface up at L2/L3
static void setup_link(void) {
    system("ip link set up dev " IFACE);           // bring wg0 up 
}

// 5) Open TUN device for packet I/O
static int open_tun(const char *name) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEV, O_RDWR);
    if (fd < 0) { perror("open /dev/net/tun"); exit(EXIT_FAILURE); }
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "[DEBUG] TUN %s opened on fd %d\n", name, fd);
    return fd;
}

int main(void) {
    // Acquire UDP socket (FD 3) via systemd
    int udp_fd = parse_activation();
    if (udp_fd < 0) return EXIT_FAILURE;

    // Create & configure WireGuard interface
    if (create_device() < 0) return EXIT_FAILURE;
    if (apply_config() < 0)   return EXIT_FAILURE;
    setup_link();

    // Open TUN after wg0 exists
    int tun_fd = open_tun(IFACE);

    // 6) Bidirectional proxy: TUN ↔ UDP using poll()
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