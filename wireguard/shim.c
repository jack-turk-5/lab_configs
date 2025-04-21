// shim.c (debug version without libcap dependency)
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <net/if.h>              // struct ifreq, IFF_* flags :contentReference[oaicite:0]{index=0}
#include <linux/if_tun.h>        // TUNSETIFF, IFF_TUN, IFF_NO_PI :contentReference[oaicite:1]{index=1}
#include "wireguard.h"           // embeddable‐wg‐library API 

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME      "wg0"
#define BUF_SIZE     65536

// Read and print CapEff from /proc/self/status
static void dump_proc_caps(void) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) { perror("fopen /proc/self/status"); return; }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "CapEff:", 7) == 0) {
            fprintf(stderr, "[DEBUG] %s", line);  // includes newline :contentReference[oaicite:2]{index=2}
            break;
        }
    }
    fclose(f);
}

// List inherited fds up to 10
static void list_fds(void) {
    char target[128];
    fprintf(stderr, "[DEBUG] inherited fds:\n");
    for (int fd = 0; fd < 10; fd++) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
        ssize_t len = readlink(path, target, sizeof(target)-1);
        if (len > 0) {
            target[len] = 0;
            fprintf(stderr, "  fd %d -> %s\n", fd, target);
        }
    }
}

// Manual socket activation: accept any LISTEN_FDS ≥1
static int parse_activation(void) {
    char *env = getenv("LISTEN_FDS");
    int fds = env ? atoi(env) : 0;
    fprintf(stderr, "[DEBUG] LISTEN_FDS=%d\n", fds);
    unsetenv("LISTEN_FDS");
    unsetenv("LISTEN_PID");
    return (fds >= 1) ? 3 : -1;
}

static int open_tun(const char *ifname) {
    struct ifreq ifr = {0};
    int fd = open(TUN_DEVICE, O_RDWR);
    if (fd < 0) { perror("open /dev/net/tun"); exit(EXIT_FAILURE); }
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl(TUNSETIFF)"); close(fd); exit(EXIT_FAILURE);
    }
    fprintf(stderr, "[DEBUG] TUN %s opened on fd %d\n", ifr.ifr_name, fd);
    return fd;
}

static void show_link(const char *name) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "ip link show %s", name);
    fprintf(stderr, "[DEBUG] %s\n", cmd);
    system(cmd);
}

int main(void) {
    fprintf(stderr, "=== wg-shim-debug starting pid=%d ===\n", getpid());
    dump_proc_caps();    // show effective capabilities :contentReference[oaicite:3]{index=3}
    list_fds();

    int udp_fd = parse_activation();
    if (udp_fd < 0) { fprintf(stderr, "ERROR: No socket activated\n"); return EXIT_FAILURE; }
    fprintf(stderr, "[DEBUG] using UDP fd=%d\n", udp_fd);

    int tun_fd = open_tun(IF_NAME);

    show_link(IF_NAME);  // before creation

    fprintf(stderr, "[DEBUG] Calling wg_add_device(%s)\n", IF_NAME);
    int add_ret = wg_add_device(IF_NAME);
    fprintf(stderr, "[DEBUG] wg_add_device -> %d errno=%d (%s)\n",
            add_ret, errno, strerror(errno));
    if (add_ret != 0) { fprintf(stderr, "ERROR: wg_add_device failed\n"); return EXIT_FAILURE; }

    show_link(IF_NAME);  // after creation

    struct wg_device *dev = NULL;
    if (wg_get_device(&dev, IF_NAME) != 0 || !dev) {
        fprintf(stderr, "ERROR: wg_get_device failed\n"); return EXIT_FAILURE;
    }
    // configure dev->private_key, dev->listen_port, peers...
    // e.g. wg_key_from_base64, set flags, etc.
    if (wg_set_device(dev) != 0) {
        fprintf(stderr, "ERROR: wg_set_device failed\n");
        wg_free_device(dev);
        return EXIT_FAILURE;
    }
    wg_free_device(dev);

    // assign IP & bring up
    char *addr = getenv("WG_ADDRESS");
    if (addr) {
        char cmd[128];
        snprintf(cmd,sizeof(cmd),"ip address add dev %s %s",IF_NAME,addr);
        system(cmd);
    }
    system("ip link set up dev " IF_NAME);

    struct pollfd fds_arr[2] = {
        {.fd = tun_fd, .events = POLLIN},
        {.fd = udp_fd,  .events = POLLIN}
    };
    unsigned char buf[BUF_SIZE];
    while (1) {
        int r = poll(fds_arr,2,-1);
        if (r<0 && errno!=EINTR) { perror("poll"); break; }
        if (fds_arr[0].revents & POLLIN) {
            ssize_t n = read(tun_fd,buf,BUF_SIZE);
            if (n>0 && send(udp_fd,buf,n,0)<0) perror("send udp");
        }
        if (fds_arr[1].revents & POLLIN) {
            ssize_t n = recv(udp_fd,buf,BUF_SIZE,0);
            if (n>0 && write(tun_fd,buf,n)<0) perror("write tun");
        }
    }

    close(tun_fd);
    close(udp_fd);
    return EXIT_SUCCESS;
}
