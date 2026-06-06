#include "bb_api.h"
#include "debug_rpc.h"
#include "getopt.h"
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

typedef struct {
    char ip[128];
    int  port;
    int  spmacflg;
    int  dev_index;
    int  dev_index_set;
    char mac[128];
    int  workflg;
} cmddbg_info;

static void copy_string(char* dst, size_t dst_size, const char* src)
{
    if (dst_size == 0) {
        return;
    }

    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

void dbg_recv_fun(struct dbg_hdl* hdl, void* priv, unsigned char* buff, int len)
{
    (void)hdl;
    (void)priv;

    if (len > 0) {
        fwrite(buff, 1, len, stdout);
        fflush(stdout);
    }
}

void bb_event_offline(void* arg, void* usr)
{
    cmddbg_info* pinfo = (cmddbg_info*)usr;
    (void)arg;

    printf("dev offline cb\n");
    pinfo->workflg = 0;
}

static int set_cb(bb_dev_handle_t* pdh, int evt, bb_event_callback pcb, void* priv)
{
    bb_set_event_callback_t cb;
    cb.callback = pcb;
    cb.event    = (bb_event_e)evt;
    cb.user     = priv;

    int ret = bb_ioctl(pdh, BB_SET_EVENT_SUBSCRIBE, &cb, 0);

    if (ret) {
        printf("set callback err!\n");
        return -1;
    }
    printf("set evt %d callback ok!\n", evt);

    return 0;
}

void print_help(void)
{
    printf("Usage: l4_cmd_dbg [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help            show this help\n");
    printf("  -a, --addr <addr>    daemon address, default: 127.0.0.1\n");
    printf("  -p, --port <port>    daemon port, default: %d\n", BB_PORT_DEFAULT);
    printf("  -i, --index <index>  device index, default: 0\n");
    printf("  -m, --mac <mac>      select device by MAC address\n");
    printf("  -l, --list            list devices and exit\n");
    printf("  -o, --output          output only, do not read commands from stdin\n");
}

int main(int argc, char* argv[])
{
    cmddbg_info info;
    info.workflg  = 1;
    info.port     = BB_PORT_DEFAULT;
    info.spmacflg = 0;
    info.dev_index = 0;
    info.dev_index_set = 0;
    copy_string(info.ip, sizeof(info.ip), "127.0.0.1");
    info.mac[0] = '\0';

    int listflg = 0;
    int output_only = 0;

    int c = 0;
    while (1) {
        int                  option_index   = 0;
        static struct option long_options[] = {
            {"help",    no_argument,       0, 'h'},
            { "addr",   required_argument, 0, 'a'},
            { "port",   required_argument, 0, 'p'},
            { "index",  required_argument, 0, 'i'},
            { "mac",    required_argument, 0, 'm'},
            { "list",   no_argument,       0, 'l'},
            { "output", no_argument,       0, 'o'},
            { 0,        0,                 0, 0  },
        };

        c = getopt_long(argc, argv, "a:p:i:m:hlo", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'a':
            copy_string(info.ip, sizeof(info.ip), optarg);
            break;
        case 'p':
            info.port = (int)strtoul(optarg, NULL, 10);
            break;
        case 'i':
            info.dev_index = (int)strtoul(optarg, NULL, 10);
            info.dev_index_set = 1;
            break;
        case 'm':
            info.spmacflg = 1;
            copy_string(info.mac, sizeof(info.mac), optarg);
            break;
        case 'l':
            listflg = 1;
            break;
        case 'o':
            output_only = 1;
            break;
        case 'h':
            print_help();
            exit(0);
            break;
        default:
            print_help();
            return -1;
        }
    }

    if (info.spmacflg && info.dev_index_set) {
        printf("-i/--index and -m/--mac cannot be used together\n");
        return -1;
    }

    bb_host_t* phost;

    int ret = bb_host_connect(&phost, info.ip, info.port);

    if (ret) {
        printf("connect failed = %d\n", ret);
        exit(-1);
    }

    bb_dev_t** devs;

    int sz = bb_dev_getlist(phost, &devs);
    if (sz <= 0) {
        printf("dev cnt = 0\n");
        bb_host_disconnect(phost);
        exit(-1);
    }

    bb_dev_handle_t* hbb = NULL;

    if (listflg) {
        for (int i = 0; i < sz; i++) {
            bb_dev_info_t dev_info;
            bb_dev_getinfo(devs[i], &dev_info);
            printf("dev %d mac :%s\n", i, (char*)dev_info.mac);
        }
        bb_dev_freelist(devs);
        bb_host_disconnect(phost);
        return 0;
    }

    if (info.spmacflg) {
        for (int i = 0; i < sz; i++) {
            bb_dev_info_t dev_info;
            bb_dev_getinfo(devs[i], &dev_info);
            if (!strcmp((char*)dev_info.mac, info.mac)) {
                hbb = bb_dev_open(devs[i]);
                break;
            }
        }
        if (!hbb) {
            printf("mac %s not found\n", info.mac);
            bb_dev_freelist(devs);
            bb_host_disconnect(phost);
            return -1;
        }
    } else {
        if (info.dev_index < 0 || info.dev_index >= sz) {
            printf("invalid device index %d, valid range: 0-%d\n", info.dev_index, sz - 1);
            bb_dev_freelist(devs);
            bb_host_disconnect(phost);
            return -1;
        }
        hbb = bb_dev_open(devs[info.dev_index]);
    }

    if (!hbb) {
        printf("dev probe err\n");
        bb_dev_freelist(devs);
        bb_host_disconnect(phost);
        return -1;
    }

    set_cb(hbb, BB_EVENT_OFFLINE, bb_event_offline, &info);

    struct dbg_hdl* dbg = dbg_setup(hbb, dbg_recv_fun, &info, -1);
    if (dbg) {
        while (1) {
            if (!output_only) {
                char  buff[1024];
                char* out = fgets(buff, 1024, stdin);

                if (!out) {
                    printf("get NULL output!!\n");
                    break;
                }

                int len = strlen(buff);

                dbg_write(dbg, (unsigned char*)buff, len);
            } else {
#ifdef WIN32
                Sleep(1);
#else
                sleep(1);
#endif
            }

            if (!info.workflg) {
                break;
            }
        }
    }

    bb_dev_close(hbb);

    bb_dev_freelist(devs);

    bb_host_disconnect(phost);

    return 0;
}
