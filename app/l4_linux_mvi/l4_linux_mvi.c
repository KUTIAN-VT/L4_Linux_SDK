#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "bb_api.h"
#include "getopt.h"

static void usage(const char *prog)
{
    printf("Usage: %s -v\n", prog);
    printf("\t-v       get version\n");
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

    while ((opt = getopt(argc, argv, "hv")) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 0;
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

    ret = bb_host_connect(&host, "127.0.0.1", BB_PORT_DEFAULT);
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

    handle = bb_dev_open(devs[0]);
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
