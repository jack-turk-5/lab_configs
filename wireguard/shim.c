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
#include "wireguard.h"           // embeddable‑wg‑library API 

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME      "wg0"
#define BUF_SIZE     65536

// 1) Parse socket activation: require LISTEN_FDS ≥1, return first FD (3)
static int parse_activation(void) {
    char *fds_env = getenv("LISTEN_FDS");
    int fds = fds_env ? atoi(fds_env) : 0;
    unsetenv("LISTEN_FDS"); unsetenv("LISTEN_PID");
    if (fds < 1) {
        fprintf(stderr, "ERROR: No socket activated (LISTEN_FDS=%d)\n", fds);
        return -1;
    }
    return 3;
}

// 2) Create or skip wg0 interface
static int create_device(void) {
    int ret = wg_add_device(IF_NAME);                   // ip link add wg0 type wireguard 
    int e = errno;
    if (ret != 0 && e != EEXIST) {
        fprintf(stderr, "ERROR: wg_add_device failed (%s)\n", strerror(e));
        return -1;
    }
    fprintf(stderr, "[DEBUG] wg0 %s\n", ret==0 ? "created" : "already exists");
    return 0;
}

// 3) Apply stripped config to avoid "Line unrecognized: Address=" errors
static int apply_config(void) {
    char cmd[256];
    // wg-quick strip wg0 outputs only wg‑compatible lines :contentReference[oaicite:4]{index=4}
    snprintf(cmd, sizeof(cmd), "wg-quick strip %s | wg setconf %s -", IF_NAME, IF_NAME);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "ERROR: wg setconf returned %d\n", rc);
        return -1;
    }
    fprintf(stderr, "[DEBUG] wg setconf applied\n");
    return 0;
}

// 4) Bring up the interface at the IP layer
static void setup_link(void) {
    // Address= lines are handled separately by ip(8)
    system("ip link set up dev " IF_NAME);             // bring wg0 up 
}

// 5) Open the TUN device for packet shuttling
static int open_tun(const char *ifname) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEVICE, O_RDWR);
    if (fd < 0) { perror("open /dev/net/tun"); exit(EXIT_FAILURE); }
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) { perror("ioctl(TUNSETIFF)"); close(fd); exit(EXIT_FAILURE); }
    fprintf(stderr, "[DEBUG] TUN %s opened on fd %d\n", ifname, fd);
    return fd;
}

int main(void) {
    // Acquire UDP socket (fd 3)
    int udp_fd = parse_activation();
    if (udp_fd < 0) return EXIT_FAILURE;

    // Create/link config
    if (create_device() < 0) return EXIT_FAILURE;
    if (apply_config() < 0)   return EXIT_FAILURE;
    setup_link();

    // Open TUN after wg0 exists
    int tun_fd = open_tun(IF_NAME);

    // 6) Bidirectional proxy: TUN <-> UDP
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