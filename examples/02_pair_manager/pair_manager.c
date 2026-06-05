#include "bb_demo_common.h"

#include "getopt.h"
#include "prj_rpc.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_PAIR_TIMEOUT_SEC 100

static volatile sig_atomic_t g_pair_done;
static volatile sig_atomic_t g_stop_requested;
static int g_pair_ret;
static int g_pair_slot;

static void print_mac(const bb_mac_t *mac)
{
    for (int i = 0; i < BB_MAC_LEN; ++i) {
        printf("%s%02x", i == 0 ? "" : ":", mac->addr[i]);
    }
}

static const char *role_name(uint8_t role)
{
    switch (role) {
    case BB_ROLE_AP:
        return "AP";
    case BB_ROLE_DEV:
        return "DEV";
    default:
        return "UNKNOWN";
    }
}

static void usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("  -a <addr>     host address, default: 127.0.0.1\n");
    printf("  -p <port>     host port, default: %d\n", BB_PORT_DEFAULT);
    printf("  -i <index>    device index, default: 0\n");
    printf("  -s <slot>     pair slot, default: 0\n");
    printf("  -t <sec>      pair timeout, default: %d\n", DEFAULT_PAIR_TIMEOUT_SEC);
    printf("  -w <sec>      wait callback seconds, default: timeout + 5\n");
    printf("  -P            start pair and wait BB_EVENT_PAIR_RESULT\n");
    printf("  -X            stop pair\n");
    printf("  -m            get paired peer mac by role\n");
    printf("  -A            get AP mac by BB_GET_AP_MAC\n");
    printf("  -h            show this help\n");
}

static void pair_signal_handler(int signo)
{
    (void)signo;
    g_stop_requested = 1;
}

static void pair_result_cb(void *arg, void *user)
{
    bb_event_pair_result_t *event = (bb_event_pair_result_t *)arg;
    int *slot = (int *)user;

    g_pair_ret = event->ret;
    g_pair_done = 1;

    printf("\n[BB_EVENT_PAIR_RESULT]\n");
    printf("pair ret=%d slot=%d\n", event->ret, *slot);
    switch (event->ret) {
    case -2:
        printf("pair timeout\n");
        break;
    case 0:
        printf("pair ok\n");
        break;
    default:
        printf("pair finished with ret=%d\n", event->ret);
        break;
    }
}

static int get_role(bb_dev_handle_t *handle, uint8_t *role)
{
    bb_get_status_in_t input;
    bb_get_status_out_t status;
    int ret;

    memset(&input, 0, sizeof(input));
    memset(&status, 0, sizeof(status));

    input.user_bmp = 0;
    ret = bb_ioctl(handle, BB_GET_STATUS, &input, &status);
    if (ret) {
        printf("BB_GET_STATUS failed, ret=%d\n", ret);
        return ret;
    }

    *role = status.role;
    printf("device role: %s (%u)\n", role_name(status.role), status.role);
    return 0;
}

static int subscribe_pair_event(bb_dev_handle_t *handle, int *slot)
{
    bb_set_event_callback_t cb;
    int ret;

    memset(&cb, 0, sizeof(cb));
    cb.event = BB_EVENT_PAIR_RESULT;
    cb.callback = pair_result_cb;
    cb.user = slot;

    ret = bb_ioctl(handle, BB_SET_EVENT_SUBSCRIBE, &cb, NULL);
    if (ret) {
        printf("BB_SET_EVENT_SUBSCRIBE failed, ret=%d\n", ret);
        return ret;
    }

    printf("pair callback registered\n");
    return 0;
}

static int send_pair_stop(bb_dev_handle_t *handle)
{
    bb_set_prj_dispatch_in_t request;
    prj_rpc_hdr_t *hdr;
    int ret;

    memset(&request, 0, sizeof(request));
    hdr = (prj_rpc_hdr_t *)request.data;
    hdr->cmdid = PRJ_CMD_EVENT_PAIR_STOP;

    ret = bb_ioctl(handle, BB_SET_PRJ_DISPATCH, &request, NULL);
    if (ret) {
        printf("stop pair failed, ret=%d\n", ret);
        return ret;
    }

    printf("stop pair command sent\n");
    return 0;
}

static int get_candidates_mac(bb_dev_handle_t *handle, int slot)
{
    bb_get_candidates_in_t input;
    bb_get_candidates_out_t candidates;
    int count;
    int ret;

    memset(&input, 0, sizeof(input));
    memset(&candidates, 0, sizeof(candidates));

    input.slot = (uint8_t)slot;
    ret = bb_ioctl(handle, BB_GET_CANDIDATES, &input, &candidates);
    if (ret) {
        printf("BB_GET_CANDIDATES failed, ret=%d\n", ret);
        return ret;
    }

    printf("\n[BB_GET_CANDIDATES]\n");
    printf("slot: %d mac_num: %u\n", slot, candidates.mac_num);

    count = candidates.mac_num;
    if (count > BB_CONFIG_MAX_SLOT_CANDIDATE) {
        printf("mac_num exceeds max, clamp to %d\n", BB_CONFIG_MAX_SLOT_CANDIDATE);
        count = BB_CONFIG_MAX_SLOT_CANDIDATE;
    }

    if (count == 0) {
        printf("no dev mac found\n");
        return 0;
    }

    for (int i = 0; i < count; ++i) {
        printf("dev_mac[%d]: ", i);
        print_mac(&candidates.mac_tab[i]);
        printf("\n");
    }

    return 0;
}

static int get_ap_mac(bb_dev_handle_t *handle)
{
    bb_get_ap_mac_out_t ap_mac;
    int ret;

    memset(&ap_mac, 0, sizeof(ap_mac));
    ret = bb_ioctl(handle, BB_GET_AP_MAC, NULL, &ap_mac);
    if (ret) {
        printf("BB_GET_AP_MAC failed, ret=%d\n", ret);
        return ret;
    }

    printf("\n[BB_GET_AP_MAC]\n");
    printf("ap_mac: ");
    print_mac(&ap_mac.mac);
    printf("\n");
    return 0;
}

static int get_pair_result(bb_dev_handle_t *handle, int slot)
{
    uint8_t role;
    int ret;

    ret = get_role(handle, &role);
    if (ret) {
        return ret;
    }

    if (role == BB_ROLE_AP) {
        return get_candidates_mac(handle, slot);
    }

    if (role == BB_ROLE_DEV) {
        return get_ap_mac(handle);
    }

    printf("unsupported role %u, pair result not queried\n", role);
    return -1;
}

static int start_pair(bb_dev_handle_t *handle, int slot, int timeout_sec, int wait_sec)
{
    bb_set_prj_dispatch_in_t request;
    prj_rpc_hdr_t *hdr;
    prj_cmd_event_pair_t *pair;
    uint8_t role;
    int ret;

    ret = get_role(handle, &role);
    if (ret) {
        return ret;
    }

    if (role != BB_ROLE_AP && role != BB_ROLE_DEV) {
        printf("unsupported role %u, pair not started\n", role);
        return -1;
    }

    g_pair_done = 0;
    g_stop_requested = 0;
    g_pair_ret = 0;
    g_pair_slot = slot;

    ret = subscribe_pair_event(handle, &g_pair_slot);
    if (ret) {
        return ret;
    }

    memset(&request, 0, sizeof(request));
    hdr = (prj_rpc_hdr_t *)request.data;
    hdr->cmdid = PRJ_CMD_EVENT_PAIR;

    pair = (prj_cmd_event_pair_t *)hdr->data;
    pair->bitmap = (role == BB_ROLE_AP) ? (uint8_t)(1u << slot) : 0;
    pair->timeout = (int16_t)timeout_sec;
    pair->asyn = 1;

    printf("start pair: role=%s slot=%d bitmap=0x%02x timeout=%d asyn=%u\n",
           role_name(role),
           slot,
           pair->bitmap,
           pair->timeout,
           pair->asyn);

    ret = bb_ioctl(handle, BB_SET_PRJ_DISPATCH, &request, NULL);
    if (ret) {
        printf("start pair failed, ret=%d\n", ret);
        return ret;
    }

    signal(SIGINT, pair_signal_handler);

    for (int elapsed = 0; elapsed < wait_sec; ++elapsed) {
        if (g_pair_done) {
            break;
        }

        if (g_stop_requested) {
            printf("interrupt received, stopping pair\n");
            send_pair_stop(handle);
            break;
        }

        printf("waiting for pair result... %d/%d\n", elapsed + 1, wait_sec);
        sleep(1);
    }

    if (!g_pair_done && !g_stop_requested) {
        printf("wait timeout, stopping pair\n");
        send_pair_stop(handle);
        return -1;
    }

    if (g_pair_done && g_pair_ret == 0) {
        get_pair_result(handle, slot);
    }

    return g_pair_done ? g_pair_ret : -1;
}

int main(int argc, char **argv)
{
    const char *addr = "127.0.0.1";
    int port = BB_PORT_DEFAULT;
    int dev_index = 0;
    int slot = 0;
    int timeout_sec = DEFAULT_PAIR_TIMEOUT_SEC;
    int wait_sec = -1;
    int do_start = 0;
    int do_stop = 0;
    int do_get_pair = 0;
    int do_get_ap_mac = 0;
    int opt;
    int ret = 0;
    bb_demo_context_t ctx;

    while ((opt = getopt(argc, argv, "ha:p:i:s:t:w:PXmA")) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'a':
            addr = optarg;
            break;
        case 'p':
            port = (int)strtol(optarg, NULL, 10);
            break;
        case 'i':
            dev_index = (int)strtol(optarg, NULL, 10);
            break;
        case 's':
            slot = (int)strtol(optarg, NULL, 10);
            break;
        case 't':
            timeout_sec = (int)strtol(optarg, NULL, 10);
            break;
        case 'w':
            wait_sec = (int)strtol(optarg, NULL, 10);
            break;
        case 'P':
            do_start = 1;
            break;
        case 'X':
            do_stop = 1;
            break;
        case 'm':
            do_get_pair = 1;
            break;
        case 'A':
            do_get_ap_mac = 1;
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    if (!do_start && !do_stop && !do_get_pair && !do_get_ap_mac) {
        usage(argv[0]);
        return -1;
    }

    if (slot < 0 || slot >= BB_SLOT_MAX) {
        printf("invalid slot %d, valid range: 0-%d\n", slot, BB_SLOT_MAX - 1);
        return -1;
    }

    if (timeout_sec <= 0 || timeout_sec > 32767) {
        printf("invalid timeout %d, valid range: 1-32767\n", timeout_sec);
        return -1;
    }

    if (wait_sec < 0) {
        wait_sec = timeout_sec + 5;
    }

    ret = bb_demo_open(&ctx, addr, port, dev_index);
    if (ret) {
        return -1;
    }

    if (do_stop) {
        ret = send_pair_stop(ctx.handle);
        if (ret) {
            goto done;
        }
    }

    if (do_start) {
        ret = start_pair(ctx.handle, slot, timeout_sec, wait_sec);
        if (ret) {
            goto done;
        }
    }

    if (do_get_pair) {
        ret = get_pair_result(ctx.handle, slot);
        if (ret) {
            goto done;
        }
    }

    if (do_get_ap_mac) {
        ret = get_ap_mac(ctx.handle);
        if (ret) {
            goto done;
        }
    }

    printf("\nl4 pair manager finished\n");

done:
    bb_demo_close(&ctx);
    return ret ? -1 : 0;
}
