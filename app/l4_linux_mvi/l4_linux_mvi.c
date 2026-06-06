#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "bb_api.h"
#include "getopt.h"

static void usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help             show this help\n");
    printf("  -a, --addr <addr>      daemon address, default: 127.0.0.1\n");
    printf("  -p, --port <port>      daemon port, default: %d\n", BB_PORT_DEFAULT);
    printf("  -i, --index <index>    device index, default: 0\n");
    printf("  -v, --version          get version\n");
}

static int get_version(bb_dev_handle_t *handle)
{
    bb_get_sys_info_out_t sys_info;
    int ret;

    memset(&sys_info, 0, sizeof(sys_info));

    ret = bb_ioctl(handle, BB_GET_SYS_INFO, NULL, &sys_info);
    if (ret) {
        printf("BB_GET_SYS_INFO failed ret = %d\n", ret);
        return ret;
    }

    printf("uptime       : %llu\n", (unsigned long long)sys_info.uptime);
    printf("compile_time : %s\n", sys_info.compile_time);
    printf("soft_ver     : %s\n", sys_info.soft_ver);
    printf("hardware_ver : %s\n", sys_info.hardware_ver);
    printf("firmware_ver : %s\n", sys_info.firmware_ver);
    return 0;
}

int main(int argc, char **argv)
{
    bool is_get_version = false;
    const char *addr = "127.0.0.1";
    int port = BB_PORT_DEFAULT;
    int dev_index = 0;
    bb_host_t *host = NULL;
    bb_dev_list_t *devs = NULL;
    bb_dev_handle_t *handle = NULL;
    int dev_num;
    int opt;
    int ret = 0;

    if (argc == 1) {
        usage(argv[0]);
        return -1;
    }

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"addr", required_argument, 0, 'a'},
        {"port", required_argument, 0, 'p'},
        {"index", required_argument, 0, 'i'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0},
    };

    while ((opt = getopt_long(argc, argv, "ha:p:i:v", long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'a':
            addr = optarg;
            break;
        case 'p':
            port = (int)strtoul(optarg, NULL, 10);
            break;
        case 'i':
            dev_index = (int)strtoul(optarg, NULL, 10);
            break;
        case 'v':
            is_get_version = true;
            printf("get version\n");
            break;
        default:
            printf("unknown cmd param!\n");
            usage(argv[0]);
            return -1;
        }
    }

    if (!is_get_version) {
        usage(argv[0]);
        return -1;
    }

    ret = bb_host_connect(&host, addr, port);
    if (ret) {
        printf("bb connect error, ret = %d\n", ret);
        return -1;
    }

    dev_num = bb_dev_getlist(host, &devs);
    if (dev_num <= 0) {
        printf("Get no device!\n");
        ret = -1;
        goto done;
    }

    if (dev_index < 0 || dev_index >= dev_num) {
        printf("invalid device index %d, valid range: 0-%d\n", dev_index, dev_num - 1);
        ret = -1;
        goto done;
    }

    handle = bb_dev_open(devs[dev_index]);
    if (!handle) {
        printf("bb_dev_open error\n");
        ret = -1;
        goto done;
    }

    ret = get_version(handle);

done:
    if (handle) {
        bb_dev_close(handle);
    }

    if (devs) {
        bb_dev_freelist(devs);
    }

    if (host) {
        bb_host_disconnect(host);
    }

    return ret ? -1 : 0;
}
