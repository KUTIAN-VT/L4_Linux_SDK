#include "bb_demo_common.h"

#include "getopt.h"
#include "prj_rpc.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PWR_VALUE_MIN 10
#define PWR_VALUE_MAX 27
#define MINIDB_UNSET_RET (-8)
#define MINIDB_RET_UNSET 1

typedef enum {
    ACTION_NONE = 0,
    ACTION_GET_ALL,
    ACTION_GET_ROLE,
    ACTION_GET_AP_MAC,
    ACTION_GET_SLOT_MAC,
    ACTION_GET_BAND,
    ACTION_GET_PWR,
    ACTION_SET_ROLE,
    ACTION_SET_AP_MAC,
    ACTION_SET_SLOT_MAC,
    ACTION_SET_BAND,
    ACTION_SET_PWR,
    ACTION_RESET,
    ACTION_REBOOT,
} action_t;

typedef struct {
    const char *addr;
    int port;
    int dev_index;
    action_t action;
    int reboot;
    int slot;
    int role;
    int band;
    int has_pwr_init;
    int has_pwr_range;
    int pwr_init;
    int pwr_min;
    int pwr_max;
    bb_mac_t mac;
} options_t;

static void usage(const char *prog)
{
    printf("Usage: %s [options] <one action>\n", prog);
    printf("\n");
    printf("Common options:\n");
    printf("  -h, --help              show this help\n");
    printf("  -a, --addr <addr>       daemon address, default: 127.0.0.1\n");
    printf("  -p, --port <port>       daemon port, default: %d\n", BB_PORT_DEFAULT);
    printf("  -i, --index <index>     device index, default: 0\n");
    printf("\n");
    printf("Query actions:\n");
    printf("  -A, --get-all           get role, AP MAC, slot 0 MAC, band and power\n");
    printf("  -r, --get-role          get role\n");
    printf("  -m, --get-ap-mac        get AP MAC\n");
    printf("  -s, --get-slot-mac [n]  get slot MAC, default slot: 0\n");
    printf("  -b, --get-band          get band bitmap\n");
    printf("  -w, --get-pwr           get power\n");
    printf("\n");
    printf("Set actions:\n");
    printf("  -R, --set-role <role>   set role: ap/dev/0/1\n");
    printf("  -M, --set-ap-mac <mac>  set AP MAC\n");
    printf("  -S, --set-slot-mac <mac|slot,mac>\n");
    printf("                           set slot MAC, example: 11:22:33:44 or 1,11:22:33:44\n");
    printf("  -B, --set-band <band>   set band: auto/2g/5g/0x07/0x02/0x04\n");
    printf("  -W, --set-pwr <pwr|min,max>\n");
    printf("                           set fixed power or adaptive range, range: %d-%d\n", PWR_VALUE_MIN, PWR_VALUE_MAX);
    printf("  -D, --reset             reset MiniDB\n");
    printf("\n");
    printf("Other options:\n");
    printf("  -H, --reboot            reboot device, or reboot after successful set/reset\n");
}

static int str_eq_ignore_case(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        ++a;
        ++b;
    }

    return *a == '\0' && *b == '\0';
}

static void init_options(options_t *opts)
{
    memset(opts, 0, sizeof(*opts));
    opts->addr = "127.0.0.1";
    opts->port = BB_PORT_DEFAULT;
    opts->dev_index = 0;
    opts->slot = -1;
    opts->role = -1;
    opts->band = -1;
    opts->pwr_init = -1;
    opts->pwr_min = -1;
    opts->pwr_max = -1;
}

static int set_action(options_t *opts, action_t action)
{
    if (opts->action != ACTION_NONE) {
        printf("only one action can be specified\n");
        return -1;
    }

    opts->action = action;
    return 0;
}

static int set_pwr_action(options_t *opts)
{
    if (opts->action == ACTION_NONE) {
        opts->action = ACTION_SET_PWR;
        return 0;
    }

    if (opts->action != ACTION_SET_PWR) {
        printf("only one action can be specified\n");
        return -1;
    }

    return 0;
}

static int parse_int_range(const char *text, int min_value, int max_value, const char *name, int *out)
{
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(text, &end, 0);
    if (errno || end == text || *end != '\0' || value < min_value || value > max_value) {
        printf("invalid %s: %s, valid range: %d-%d\n", name, text, min_value, max_value);
        return -1;
    }

    *out = (int)value;
    return 0;
}

static int parse_role(const char *text, int *out)
{
    if (str_eq_ignore_case(text, "ap")) {
        *out = BB_ROLE_AP;
        return 0;
    }

    if (str_eq_ignore_case(text, "dev")) {
        *out = BB_ROLE_DEV;
        return 0;
    }

    return parse_int_range(text, BB_ROLE_AP, BB_ROLE_DEV, "role", out);
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}

static int parse_mac(const char *text, bb_mac_t *mac)
{
    int nibble = -1;
    int count = 0;

    memset(mac, 0, sizeof(*mac));

    for (const char *p = text; *p; ++p) {
        int hex;

        if (*p == ':' || *p == '-') {
            continue;
        }

        hex = hex_value(*p);
        if (hex < 0) {
            printf("invalid MAC: %s\n", text);
            return -1;
        }

        if (nibble < 0) {
            nibble = hex;
            continue;
        }

        if (count >= BB_MAC_LEN) {
            printf("invalid MAC length: %s\n", text);
            return -1;
        }

        mac->addr[count++] = (uint8_t)((nibble << 4) | hex);
        nibble = -1;
    }

    if (nibble >= 0 || count != BB_MAC_LEN) {
        printf("invalid MAC length: %s\n", text);
        return -1;
    }

    return 0;
}

static int parse_band_bitmap(const char *text, int *out)
{
    if (str_eq_ignore_case(text, "auto")) {
        *out = 0x07;
        return 0;
    }

    if (str_eq_ignore_case(text, "2g")) {
        *out = 0x02;
        return 0;
    }

    if (str_eq_ignore_case(text, "5g")) {
        *out = 0x04;
        return 0;
    }

    return parse_int_range(text, 0, 0xff, "band bitmap", out);
}

static int parse_pwr_value(const char *text, const char *name, int *out)
{
    return parse_int_range(text, PWR_VALUE_MIN, PWR_VALUE_MAX, name, out);
}

static int parse_pwr_range(const char *text, int *pwr_min, int *pwr_max)
{
    char copy[64];
    char *comma;

    if (strlen(text) >= sizeof(copy)) {
        printf("power range argument is too long\n");
        return -1;
    }

    strcpy(copy, text);
    comma = strchr(copy, ',');
    if (!comma || strchr(comma + 1, ',')) {
        printf("invalid power range: %s, expected: min,max\n", text);
        return -1;
    }

    *comma = '\0';
    if (parse_pwr_value(copy, "pwr_min", pwr_min) ||
        parse_pwr_value(comma + 1, "pwr_max", pwr_max)) {
        return -1;
    }

    if (*pwr_min > *pwr_max) {
        printf("invalid power range: min %d is greater than max %d\n", *pwr_min, *pwr_max);
        return -1;
    }

    return 0;
}

static int parse_pwr_arg(const char *text, options_t *opts)
{
    if (strchr(text, ',')) {
        if (parse_pwr_range(text, &opts->pwr_min, &opts->pwr_max)) {
            return -1;
        }
        opts->has_pwr_range = 1;
        return 0;
    }

    if (parse_pwr_value(text, "pwr_init", &opts->pwr_init)) {
        return -1;
    }
    opts->has_pwr_init = 1;
    return 0;
}

static int parse_slot_mac_arg(const char *text, int *slot, bb_mac_t *mac)
{
    char copy[128];
    char *comma;

    if (strlen(text) >= sizeof(copy)) {
        printf("slot MAC argument is too long\n");
        return -1;
    }

    strcpy(copy, text);
    comma = strchr(copy, ',');
    if (!comma) {
        *slot = 0;
        return parse_mac(copy, mac);
    }

    *comma = '\0';
    if (parse_int_range(copy, 0, 255, "slot", slot)) {
        return -1;
    }

    return parse_mac(comma + 1, mac);
}

static const char *role_name(int role)
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

static const char *band_bitmap_name(int band)
{
    switch (band) {
    case 0x07:
        return "auto";
    case 0x02:
        return "2g";
    case 0x04:
        return "5g";
    default:
        return "custom";
    }
}

static const char *pwr_auto_name(int pwr_auto)
{
    return pwr_auto ? "on" : "off";
}

static void print_mac(const bb_mac_t *mac)
{
    for (int i = 0; i < BB_MAC_LEN; ++i) {
        printf("%s%02x", i == 0 ? "" : ":", mac->addr[i]);
    }
}

static int minidb_set_dispatch(bb_dev_handle_t *handle, uint8_t cmdid, const void *payload, size_t payload_size)
{
    bb_set_prj_dispatch_in_t request;
    prj_rpc_hdr_t *hdr;
    size_t max_payload = sizeof(request.data) - sizeof(*hdr);
    int ret;

    if (payload_size > max_payload) {
        printf("payload too large: %zu > %zu\n", payload_size, max_payload);
        return -1;
    }

    memset(&request, 0, sizeof(request));
    hdr = (prj_rpc_hdr_t *)request.data;
    hdr->cmdid = cmdid;
    if (payload && payload_size > 0) {
        memcpy(hdr->data, payload, payload_size);
    }

    ret = bb_ioctl(handle, BB_SET_PRJ_DISPATCH, &request, NULL);
    if (ret) {
        printf("BB_SET_PRJ_DISPATCH cmd=%u failed, ret=%d\n", cmdid, ret);
        return ret;
    }

    return 0;
}

static int minidb_get_dispatch(bb_dev_handle_t *handle,
                               uint8_t cmdid,
                               const void *input,
                               size_t input_size,
                               void *output,
                               size_t output_size)
{
    bb_get_prj_dispatch_in_t request;
    bb_get_prj_dispatch_out_t response;
    prj_rpc_hdr_t *hdr;
    prj_rpc_hdr_t *out_hdr;
    size_t max_payload = sizeof(request.data) - sizeof(*hdr);
    int ret;

    if (input_size > max_payload || output_size > max_payload) {
        printf("payload too large\n");
        return -1;
    }

    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    hdr = (prj_rpc_hdr_t *)request.data;
    hdr->cmdid = cmdid;
    if (input && input_size > 0) {
        memcpy(hdr->data, input, input_size);
    }

    ret = bb_ioctl(handle, BB_GET_PRJ_DISPATCH, &request, &response);
    if (ret == MINIDB_UNSET_RET) {
        return MINIDB_RET_UNSET;
    }
    if (ret) {
        printf("BB_GET_PRJ_DISPATCH cmd=%u failed, ret=%d\n", cmdid, ret);
        return ret;
    }

    out_hdr = (prj_rpc_hdr_t *)response.data;
    if (output && output_size > 0) {
        memcpy(output, out_hdr->data, output_size);
    }

    return 0;
}

static int get_role(bb_dev_handle_t *handle, int compact)
{
    prj_cmd_get_role_t role;
    int ret = minidb_get_dispatch(handle, PRJ_CMD_GET_ROLE, NULL, 0, &role, sizeof(role));

    if (ret == MINIDB_RET_UNSET) {
        if (compact) {
            printf("role     : unset\n");
        } else {
            printf("[PRJ_CMD_GET_ROLE]\n");
            printf("role=unset\n");
        }
        return 0;
    }
    if (ret) {
        return ret;
    }

    if (role.role == 255) {
        if (compact) {
            printf("role     : unset\n");
        } else {
            printf("[PRJ_CMD_GET_ROLE]\n");
            printf("role=unset\n");
        }
        return 0;
    }

    if (compact) {
        printf("role     : %s(%u)\n", role_name(role.role), role.role);
    } else {
        printf("[PRJ_CMD_GET_ROLE]\n");
        printf("role=%s(%u)\n", role_name(role.role), role.role);
    }

    return 0;
}

static int get_ap_mac(bb_dev_handle_t *handle, int compact)
{
    prj_cmd_get_ap_mac_t mac;
    int ret = minidb_get_dispatch(handle, PRJ_CMD_GET_AP_MAC, NULL, 0, &mac, sizeof(mac));

    if (ret == MINIDB_RET_UNSET) {
        if (compact) {
            printf("ap_mac   : unset\n");
        } else {
            printf("[PRJ_CMD_GET_AP_MAC]\n");
            printf("ap_mac=unset\n");
        }
        return 0;
    }
    if (ret) {
        return ret;
    }

    if (compact) {
        printf("ap_mac   : ");
    } else {
        printf("[PRJ_CMD_GET_AP_MAC]\n");
        printf("ap_mac=");
    }
    print_mac(&mac.ap_mac);
    printf("\n");

    return 0;
}

static int get_slot_mac(bb_dev_handle_t *handle, int slot, int compact)
{
    prj_cmd_get_slot_mac_in_t input;
    prj_cmd_get_slot_mac_out_t output;
    int ret;

    memset(&input, 0, sizeof(input));
    input.slot_id = (uint8_t)slot;

    ret = minidb_get_dispatch(handle, PRJ_CMD_GET_SLOT_MAC, &input, sizeof(input), &output, sizeof(output));
    if (ret == MINIDB_RET_UNSET) {
        if (compact) {
            printf("slot_mac%d: unset\n", slot);
        } else {
            printf("[PRJ_CMD_GET_SLOT_MAC]\n");
            printf("slot=%d mac=unset\n", slot);
        }
        return 0;
    }
    if (ret) {
        return ret;
    }

    if (compact) {
        printf("slot_mac%d: ", slot);
    } else {
        printf("[PRJ_CMD_GET_SLOT_MAC]\n");
        printf("slot=%d mac=", slot);
    }
    print_mac(&output.mac);
    printf("\n");
    return 0;
}

static int get_band(bb_dev_handle_t *handle, int compact)
{
    prj_cmd_get_band_t band;
    int ret = minidb_get_dispatch(handle, PRJ_CMD_GET_BAND, NULL, 0, &band, sizeof(band));

    if (ret == MINIDB_RET_UNSET) {
        if (compact) {
            printf("band     : unset\n");
        } else {
            printf("[PRJ_CMD_GET_BAND]\n");
            printf("band=unset\n");
        }
        return 0;
    }
    if (ret) {
        return ret;
    }

    if (compact) {
        printf("band     : %s(0x%02x)\n", band_bitmap_name(band.band_bmp), band.band_bmp);
    } else {
        printf("[PRJ_CMD_GET_BAND]\n");
        printf("band=%s(0x%02x)\n", band_bitmap_name(band.band_bmp), band.band_bmp);
    }

    return 0;
}

static int get_pwr(bb_dev_handle_t *handle, int compact)
{
    bb_phy_pwr_basic_t pwr;
    int ret = minidb_get_dispatch(handle, PRJ_CMD_GET_PWR, NULL, 0, &pwr, sizeof(pwr));

    if (ret == MINIDB_RET_UNSET) {
        if (compact) {
            printf("power    : unset\n");
        } else {
            printf("[PRJ_CMD_GET_PWR]\n");
            printf("power=unset\n");
        }
        return 0;
    }
    if (ret) {
        return ret;
    }

    if (compact) {
        if (pwr.pwr_auto) {
            printf("power    : auto range=%u-%u dBm\n", pwr.pwr_min, pwr.pwr_max);
        } else {
            printf("power    : fixed init=%u dBm\n", pwr.pwr_init);
        }
    } else {
        printf("[PRJ_CMD_GET_PWR]\n");
        printf("pwr_auto=%s(%u)\n", pwr_auto_name(pwr.pwr_auto), pwr.pwr_auto);
        if (pwr.pwr_auto) {
            printf("pwr_min=%u dBm\n", pwr.pwr_min);
            printf("pwr_max=%u dBm\n", pwr.pwr_max);
        } else {
            printf("pwr_init=%u dBm\n", pwr.pwr_init);
        }
    }

    return 0;
}

static int get_all(bb_dev_handle_t *handle)
{
    int ret = 0;

    printf("[MiniDB]\n");
    ret |= get_role(handle, 1);
    ret |= get_ap_mac(handle, 1);
    ret |= get_slot_mac(handle, 0, 1);
    ret |= get_band(handle, 1);
    ret |= get_pwr(handle, 1);

    return ret ? -1 : 0;
}

static int set_role(bb_dev_handle_t *handle, int role)
{
    prj_cmd_set_role_t payload;
    int ret;

    memset(&payload, 0, sizeof(payload));
    payload.role = (uint8_t)role;

    ret = minidb_set_dispatch(handle, PRJ_CMD_SET_ROLE, &payload, sizeof(payload));
    if (ret) {
        return ret;
    }

    printf("[PRJ_CMD_SET_ROLE]\n");
    printf("role=%s(%u)\n", role_name(payload.role), payload.role);
    printf("set minidb role ok\n");
    return 0;
}

static int set_ap_mac(bb_dev_handle_t *handle, const bb_mac_t *mac)
{
    prj_cmd_set_ap_mac_t payload;
    int ret;

    memset(&payload, 0, sizeof(payload));
    payload.ap_mac = *mac;

    ret = minidb_set_dispatch(handle, PRJ_CMD_SET_AP_MAC, &payload, sizeof(payload));
    if (ret) {
        return ret;
    }

    printf("[PRJ_CMD_SET_AP_MAC]\n");
    printf("ap_mac=");
    print_mac(&payload.ap_mac);
    printf("\n");
    printf("set minidb AP MAC ok\n");
    return 0;
}

static int set_slot_mac(bb_dev_handle_t *handle, int slot, const bb_mac_t *mac)
{
    prj_cmd_set_slot_mac_t payload;
    int ret;

    memset(&payload, 0, sizeof(payload));
    payload.slot_id = (uint8_t)slot;
    payload.slot_mac = *mac;

    ret = minidb_set_dispatch(handle, PRJ_CMD_SET_SLOT_MAC, &payload, sizeof(payload));
    if (ret) {
        return ret;
    }

    printf("[PRJ_CMD_SET_SLOT_MAC]\n");
    printf("slot=%u mac=", payload.slot_id);
    print_mac(&payload.slot_mac);
    printf("\n");
    printf("set minidb slot MAC ok\n");
    return 0;
}

static int set_band(bb_dev_handle_t *handle, int band)
{
    prj_cmd_set_band_t payload;
    int ret;

    memset(&payload, 0, sizeof(payload));
    payload.band_bmp = (uint8_t)band;

    ret = minidb_set_dispatch(handle, PRJ_CMD_SET_BAND, &payload, sizeof(payload));
    if (ret) {
        return ret;
    }

    printf("[PRJ_CMD_SET_BAND]\n");
    printf("band=%s(0x%02x)\n", band_bitmap_name(payload.band_bmp), payload.band_bmp);
    printf("set minidb band ok\n");
    return 0;
}

static int set_pwr(bb_dev_handle_t *handle, const options_t *opts)
{
    bb_phy_pwr_basic_t payload;
    int ret;

    memset(&payload, 0, sizeof(payload));
    ret = minidb_get_dispatch(handle, PRJ_CMD_GET_PWR, NULL, 0, &payload, sizeof(payload));
    if (ret == MINIDB_RET_UNSET) {
        memset(&payload, 0, sizeof(payload));
    } else if (ret) {
        return ret;
    }

    if (opts->has_pwr_range) {
        payload.pwr_auto = 1;
        payload.pwr_min = (uint8_t)opts->pwr_min;
        payload.pwr_max = (uint8_t)opts->pwr_max;
    } else {
        payload.pwr_auto = 0;
        payload.pwr_init = (uint8_t)opts->pwr_init;
        payload.pwr_min = payload.pwr_init;
        payload.pwr_max = payload.pwr_init;
    }

    ret = minidb_set_dispatch(handle, PRJ_CMD_SET_PWR, &payload, sizeof(payload));
    if (ret) {
        return ret;
    }

    printf("[PRJ_CMD_SET_PWR]\n");
    printf("pwr_auto=%s(%u)\n", pwr_auto_name(payload.pwr_auto), payload.pwr_auto);
    if (payload.pwr_auto) {
        printf("pwr_min=%u dBm\n", payload.pwr_min);
        printf("pwr_max=%u dBm\n", payload.pwr_max);
    } else {
        printf("pwr_init=%u dBm\n", payload.pwr_init);
    }
    printf("set minidb power ok\n");
    return 0;
}

static int reset_minidb(bb_dev_handle_t *handle)
{
    int ret = minidb_set_dispatch(handle, PRJ_CMD_SET_RESET_DB, NULL, 0);

    if (ret) {
        return ret;
    }

    printf("[PRJ_CMD_SET_RESET_DB]\n");
    printf("reset minidb ok\n");
    return 0;
}

static int reboot_device(bb_dev_handle_t *handle)
{
    bb_set_reboot_t reboot;
    int ret;

    memset(&reboot, 0, sizeof(reboot));
    reboot.tim_ms = 0;

    ret = bb_ioctl(handle, BB_SET_SYS_REBOOT, &reboot, NULL);
    if (ret) {
        printf("BB_SET_SYS_REBOOT failed, ret=%d\n", ret);
        return ret;
    }

    printf("[BB_SET_SYS_REBOOT]\n");
    printf("reboot requested\n");
    return 0;
}

static int parse_options(int argc, char **argv, options_t *opts)
{
    enum {
        OPT_GET_ALL = 1000,
        OPT_GET_ROLE,
        OPT_GET_AP_MAC,
        OPT_GET_SLOT_MAC,
        OPT_GET_BAND,
        OPT_GET_PWR,
        OPT_SET_ROLE,
        OPT_SET_AP_MAC,
        OPT_SET_SLOT_MAC,
        OPT_SET_BAND,
        OPT_SET_PWR,
        OPT_RESET,
        OPT_REBOOT,
        OPT_ADDR,
        OPT_PORT,
        OPT_INDEX,
        OPT_HELP,
    };

    static const struct option long_options[] = {
        {"get-all", no_argument, NULL, OPT_GET_ALL},
        {"get-role", no_argument, NULL, OPT_GET_ROLE},
        {"get-ap-mac", no_argument, NULL, OPT_GET_AP_MAC},
        {"get-slot-mac", optional_argument, NULL, OPT_GET_SLOT_MAC},
        {"get-band", no_argument, NULL, OPT_GET_BAND},
        {"get-pwr", no_argument, NULL, OPT_GET_PWR},
        {"set-role", required_argument, NULL, OPT_SET_ROLE},
        {"set-ap-mac", required_argument, NULL, OPT_SET_AP_MAC},
        {"set-slot-mac", required_argument, NULL, OPT_SET_SLOT_MAC},
        {"set-band", required_argument, NULL, OPT_SET_BAND},
        {"set-pwr", required_argument, NULL, OPT_SET_PWR},
        {"reset", no_argument, NULL, OPT_RESET},
        {"reboot", no_argument, NULL, OPT_REBOOT},
        {"addr", required_argument, NULL, OPT_ADDR},
        {"port", required_argument, NULL, OPT_PORT},
        {"index", required_argument, NULL, OPT_INDEX},
        {"help", no_argument, NULL, OPT_HELP},
        {NULL, 0, NULL, 0},
    };

    int opt;

    init_options(opts);

    while ((opt = getopt_long(argc, argv, "ha:p:i:Arms::bwR:M:S:B:W:DH", long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
        case OPT_HELP:
            usage(argv[0]);
            return 1;
        case 'a':
        case OPT_ADDR:
            opts->addr = optarg;
            break;
        case 'p':
        case OPT_PORT:
            if (parse_int_range(optarg, 1, 65535, "port", &opts->port)) {
                return -1;
            }
            break;
        case 'i':
        case OPT_INDEX:
            if (parse_int_range(optarg, 0, 255, "index", &opts->dev_index)) {
                return -1;
            }
            break;
        case 'A':
        case OPT_GET_ALL:
            if (set_action(opts, ACTION_GET_ALL)) {
                return -1;
            }
            break;
        case 'r':
        case OPT_GET_ROLE:
            if (set_action(opts, ACTION_GET_ROLE)) {
                return -1;
            }
            break;
        case 'm':
        case OPT_GET_AP_MAC:
            if (set_action(opts, ACTION_GET_AP_MAC)) {
                return -1;
            }
            break;
        case 's':
        case OPT_GET_SLOT_MAC:
            if (set_action(opts, ACTION_GET_SLOT_MAC)) {
                return -1;
            }
            opts->slot = 0;
            if (!optarg && optind < argc && argv[optind][0] != '-') {
                optarg = argv[optind++];
            }
            if (optarg && parse_int_range(optarg, 0, 255, "slot", &opts->slot)) {
                return -1;
            }
            break;
        case 'b':
        case OPT_GET_BAND:
            if (set_action(opts, ACTION_GET_BAND)) {
                return -1;
            }
            break;
        case 'w':
        case OPT_GET_PWR:
            if (set_action(opts, ACTION_GET_PWR)) {
                return -1;
            }
            break;
        case 'R':
        case OPT_SET_ROLE:
            if (set_action(opts, ACTION_SET_ROLE) || parse_role(optarg, &opts->role)) {
                return -1;
            }
            break;
        case 'M':
        case OPT_SET_AP_MAC:
            if (set_action(opts, ACTION_SET_AP_MAC) || parse_mac(optarg, &opts->mac)) {
                return -1;
            }
            break;
        case 'S':
        case OPT_SET_SLOT_MAC:
            if (set_action(opts, ACTION_SET_SLOT_MAC) || parse_slot_mac_arg(optarg, &opts->slot, &opts->mac)) {
                return -1;
            }
            break;
        case 'B':
        case OPT_SET_BAND:
            if (set_action(opts, ACTION_SET_BAND) || parse_band_bitmap(optarg, &opts->band)) {
                return -1;
            }
            break;
        case 'W':
        case OPT_SET_PWR:
            if (set_pwr_action(opts)) {
                return -1;
            }
            if (opts->has_pwr_init || opts->has_pwr_range) {
                printf("power already specified\n");
                return -1;
            }
            if (parse_pwr_arg(optarg, opts)) {
                return -1;
            }
            break;
        case 'D':
        case OPT_RESET:
            if (set_action(opts, ACTION_RESET)) {
                return -1;
            }
            break;
        case 'H':
        case OPT_REBOOT:
            opts->reboot = 1;
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    if (optind < argc) {
        printf("unexpected argument: %s\n", argv[optind]);
        return -1;
    }

    if (opts->reboot) {
        if (opts->action == ACTION_NONE) {
            opts->action = ACTION_REBOOT;
            return 0;
        }

        if (opts->action == ACTION_GET_ALL || opts->action == ACTION_GET_ROLE ||
            opts->action == ACTION_GET_AP_MAC || opts->action == ACTION_GET_SLOT_MAC ||
            opts->action == ACTION_GET_BAND || opts->action == ACTION_GET_PWR) {
            printf("reboot can only be used with set/reset actions\n");
            return -1;
        }
    }

    if (opts->action == ACTION_NONE) {
        usage(argv[0]);
        return -1;
    }

    return 0;
}

static int run_action(bb_dev_handle_t *handle, const options_t *opts)
{
    switch (opts->action) {
    case ACTION_GET_ALL:
        return get_all(handle);
    case ACTION_GET_ROLE:
        return get_role(handle, 0);
    case ACTION_GET_AP_MAC:
        return get_ap_mac(handle, 0);
    case ACTION_GET_SLOT_MAC:
        return get_slot_mac(handle, opts->slot, 0);
    case ACTION_GET_BAND:
        return get_band(handle, 0);
    case ACTION_GET_PWR:
        return get_pwr(handle, 0);
    case ACTION_SET_ROLE:
        return set_role(handle, opts->role);
    case ACTION_SET_AP_MAC:
        return set_ap_mac(handle, &opts->mac);
    case ACTION_SET_SLOT_MAC:
        return set_slot_mac(handle, opts->slot, &opts->mac);
    case ACTION_SET_BAND:
        return set_band(handle, opts->band);
    case ACTION_SET_PWR:
        return set_pwr(handle, opts);
    case ACTION_RESET:
        return reset_minidb(handle);
    case ACTION_REBOOT:
        return reboot_device(handle);
    default:
        return -1;
    }
}

int main(int argc, char **argv)
{
    options_t opts;
    bb_demo_context_t ctx;
    int ret;

    ret = parse_options(argc, argv, &opts);
    if (ret > 0) {
        return 0;
    }
    if (ret < 0) {
        return -1;
    }

    ret = bb_demo_open(&ctx, opts.addr, opts.port, opts.dev_index);
    if (ret) {
        return -1;
    }

    ret = run_action(ctx.handle, &opts);
    if (!ret && opts.reboot && opts.action != ACTION_REBOOT) {
        ret = reboot_device(ctx.handle);
    }

    bb_demo_close(&ctx);
    return ret ? -1 : 0;
}
