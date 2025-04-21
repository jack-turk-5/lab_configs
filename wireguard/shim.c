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
#include <net/if.h>              // for struct ifreq, IFF_TUN flags :contentReference[oaicite:1]{index=1}
#include <linux/if_tun.h>        // for TUNSETIFF ioctl :contentReference[oaicite:2]{index=2}
#include "wireguard.h"           // embeddable‑wg‑library API :contentReference[oaicite:3]{index=3}

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME      "wg0"
#define CFG_PATH     "/etc/wireguard/wg0.conf"
#define BUF_SIZE     65536

// Parse systemd socket activation (LISTEN_FDS)
static int parse_activation(void) {
    char *fds = getenv("LISTEN_FDS");
    int n = fds ? atoi(fds) : 0;
    unsetenv("LISTEN_FDS"); unsetenv("LISTEN_PID");
    return (n >= 1) ? 3 : -1;
}

// Create the wg0 interface (ignore EEXIST)
static int create_device(void) {
    int ret = wg_add_device(IF_NAME);
    int e = errno;
    if (ret != 0 && e != EEXIST) {
        fprintf(stderr, "ERROR: wg_add_device failed: %s\n", strerror(e));
        return -1;
    }
    fprintf(stderr, "[DEBUG] wg0 %s\n", ret==0 ? "created" : "already exists");
    return 0;
}

// Load full config from file via wg_setconf fallback
static int load_full_config(void) {
    // Attempt via library API (if available)
    FILE *f = fopen(CFG_PATH, "r");
    if (!f) {
        perror("fopen wg0.conf");
        return -1;
    }
    fclose(f);
    // Shell out to wg setconf, since libwg lacks a direct file API:
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "wg setconf %s %s", IF_NAME, CFG_PATH);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "ERROR: wg setconf returned %d\n", rc);
        return -1;
    }
    fprintf(stderr, "[DEBUG] wg setconf applied\n");
    return 0;
}

// Open or clone the TUN interface
static int open_tun(const char *name) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEVICE, O_RDWR);
    if (fd < 0) { perror("open /dev/net/tun"); exit(1); }
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) { perror("ioctl(TUNSETIFF)"); close(fd); exit(1); }
    fprintf(stderr, "[DEBUG] TUN %s opened (fd %d)\n", ifr.ifr_name, fd);
    return fd;
}

int main(void) {
    // 1) Grab UDP socket via systemd
    int udp_fd = parse_activation();
    if (udp_fd < 0) { fprintf(stderr, "ERROR: No socket activated\n"); return 1; }

    // 2) Create WireGuard interface
    if (create_device() < 0) return 1;

    // 3) Load entire WireGuard config from wg0.conf
    if (load_full_config() < 0) return 1;

    // 4) Assign IP/netmask and bring wg0 up
    system("ip link set up dev " IF_NAME);

    // 5) Open TUN after wg0 exists
    int tun_fd = open_tun(IF_NAME);

    // 6) Proxy packets between UDP and TUN
    struct pollfd pfds[2] = {
        {.fd = tun_fd, .events = POLLIN},
        {.fd = udp_fd,  .events = POLLIN}
    };
    unsigned char buf[BUF_SIZE];
    while (1) {
        if (poll(pfds, 2, -1) < 0 && errno!=EINTR) { perror("poll"); break; }
        if (pfds[1].revents & POLLIN) {
            ssize_t n = recv(udp_fd, buf, BUF_SIZE, 0);
            if (n > 0 && write(tun_fd, buf, n) < 0) perror("write tun");
        }
        if (pfds[0].revents & POLLIN) {
            ssize_t n = read(tun_fd, buf, BUF_SIZE);
            if (n > 0 && send(udp_fd, buf, n, 0) < 0) perror("send udp");
        }
    }

    close(tun_fd);
    close(udp_fd);
    return 0;
}