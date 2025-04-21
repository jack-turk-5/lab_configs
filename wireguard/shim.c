// shim.c  (debug build)
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/capability.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include "wireguard.h"

#define TUN_DEVICE   "/dev/net/tun"
#define IF_NAME      "wg0"
#define BUF_SIZE     65536

// print capabilities for debugging
static void dump_caps(void) {
    cap_t caps = cap_get_proc();
    if (!caps) return;
    char *s = cap_to_text(caps, NULL);
    fprintf(stderr, "[DEBUG] Capabilities: %s\n", s);
    cap_free(s);
    cap_free(caps);
}

// list inherited FDs up to 10
static void list_fds(void) {
    fprintf(stderr, "[DEBUG] Checking FDs:\n");
    for (int fd = 0; fd < 10; fd++) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
        char target[128];
        ssize_t len = readlink(path, target, sizeof(target)-1);
        if (len>0) {
            target[len]=0;
            fprintf(stderr, "  fd %d -> %s\n", fd, target);
        }
    }
}

// parse LISTEN_FDS (but skip PID check to allow backgrounding)
static int parse_activation(void) {
    char *env = getenv("LISTEN_FDS");
    int fds = env ? atoi(env) : 0;
    fprintf(stderr, "[DEBUG] LISTEN_FDS=%s -> %d fds\n", env?:"(null)", fds);
    if (fds < 1) {
        fprintf(stderr, "ERROR: No socket activated (LISTEN_FDS=%d)\n", fds);
        return -1;
    }
    unsetenv("LISTEN_FDS");
    unsetenv("LISTEN_PID");
    return 3;
}

static int open_tun(const char *ifname) {
    fprintf(stderr, "[DEBUG] Opening TUN %s\n", ifname);
    int fd = open(TUN_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        exit(EXIT_FAILURE);
    }
    struct ifreq ifr = {0};
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

static void show_link(const char *name) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "ip link show %s 2>&1", name);
    fprintf(stderr, "[DEBUG] %s\n", cmd);
    system(cmd);
}

int main(void) {
    fprintf(stderr, "=== wg-shim-debug starting (pid=%d) ===\n", getpid());
    dump_caps();
    list_fds();

    // 1) socket activation
    int udp_fd = parse_activation();
    if (udp_fd < 0) return EXIT_FAILURE;
    fprintf(stderr, "[DEBUG] using UDP fd=%d\n", udp_fd);

    // 2) open tun
    int tun_fd = open_tun(IF_NAME);

    // 3) show current wg0 link (should not exist)
    show_link(IF_NAME);

    // 4) create wg0
    fprintf(stderr, "[DEBUG] Calling wg_add_device(%s)\n", IF_NAME);
    int add_ret = wg_add_device(IF_NAME);
    fprintf(stderr, "[DEBUG] wg_add_device returned %d, errno=%d (%s)\n",
            add_ret, errno, strerror(errno));
    if (add_ret != 0) {
        fprintf(stderr, "ERROR: wg_add_device failed—cannot proceed\n");
        return EXIT_FAILURE;
    }

    // 5) show wg0 link (should now exist)
    show_link(IF_NAME);

    // 6) get and configure dev
    struct wg_device *dev = NULL;
    if (wg_get_device(&dev, IF_NAME) != 0 || !dev) {
        fprintf(stderr, "ERROR: wg_get_device failed\n");
        return EXIT_FAILURE;
    }
    // configure keys, port…
    // (omitted here for brevity)

    // 7) apply config
    int set_ret = wg_set_device(dev);
    fprintf(stderr, "[DEBUG] wg_set_device returned %d\n", set_ret);
    wg_free_device(dev);

    // 8) assign IP & up
    char *addr = getenv("WG_ADDRESS");
    if (addr) {
        char cmd[128];
        snprintf(cmd,sizeof(cmd),"ip address add dev %s %s",IF_NAME,addr);
        system(cmd);
    }
    system("ip link set up dev " IF_NAME);

    // 9) proxy loop
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