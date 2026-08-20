#include "bb_demo_common.h"

#include "getopt.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define POWER_FORCED_TX_CMD_INTERVAL_US (200 * 1000)

static void usage(const char *prog)
{
    printf("Usage: %s -f <freq_khz> -P <power_dbm> [options]\n", prog);
    printf("\n");
    printf("Required options:\n");
    printf("  -f <freq_khz>   TX frequency in KHz, for example: 5100000\n");
    printf("  -P <power_dbm>  TX power, range: 0-31 dBm\n");
    printf("\n");
    printf("Other options:\n");
    printf("  -h              show this help\n");
    printf("  -a <addr>       daemon address, default: 127.0.0.1\n");
    printf("  -p <port>       daemon port, default: %d\n", BB_PORT_DEFAULT);
    printf("  -i <index>      device index, default: 0\n");
}

static int parse_u32_range(const char *text,
                           uint32_t min_value,
                           uint32_t max_value,
                           const char *name,
                           uint32_t *out)
{
    char *end = NULL;
    unsigned long long value;

    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno || end == text || *end != '\0' || text[0] == '-' ||
        value < min_value || value > max_value) {
        printf("invalid %s: %s, valid range: %u-%u\n",
               name,
               text,
               min_value,
               max_value);
        return -1;
    }

    *out = (uint32_t)value;
    return 0;
}

static int set_debug_mode(bb_dev_handle_t *handle)
{
    bb_set_dbg_mode_t input = {0};
    int ret;

    input.enable = 1;
    ret = bb_ioctl(handle, BB_SET_DBG_MODE, &input, NULL);
    if (ret) {
        printf("BB_SET_DBG_MODE failed, ret=%d\n", ret);
        return ret;
    }

    printf("BB_SET_DBG_MODE ok: enable=%u\n", input.enable);
    return 0;
}

static int disable_power_auto(bb_dev_handle_t *handle)
{
    bb_set_pwr_auto_in_t input = {0};
    int ret;

    input.pwr_auto = 0;
    ret = bb_ioctl(handle, BB_SET_POWER_AUTO, &input, NULL);
    if (ret) {
        printf("BB_SET_POWER_AUTO failed, ret=%d\n", ret);
        return ret;
    }

    printf("BB_SET_POWER_AUTO ok: pwr_auto=%u\n", input.pwr_auto);
    return 0;
}

static int set_tx_frequency(bb_dev_handle_t *handle, uint32_t freq_khz)
{
    bb_set_freq_t input = {0};
    int ret;

    input.user = BB_USER_BR_CS;
    input.dir_bmp = 1U << BB_DIR_TX;
    input.freq_khz = freq_khz;
    ret = bb_ioctl(handle, BB_SET_FREQ, &input, NULL);
    if (ret) {
        printf("BB_SET_FREQ failed, ret=%d\n", ret);
        return ret;
    }

    printf("BB_SET_FREQ ok: user=%u dir=TX freq=%u KHz\n",
           input.user,
           input.freq_khz);
    return 0;
}

static int set_tx_power(bb_dev_handle_t *handle, uint32_t power_dbm)
{
    bb_set_pwr_in_t input = {0};
    int ret;

    input.usr = BB_USER_BR_CS;
    input.pwr = (uint8_t)power_dbm;
    ret = bb_ioctl(handle, BB_SET_POWER, &input, NULL);
    if (ret) {
        printf("BB_SET_POWER failed, ret=%d\n", ret);
        return ret;
    }

    printf("BB_SET_POWER ok: user=%u power=%u dBm\n", input.usr, input.pwr);
    return 0;
}

static int enter_power_test_mode(bb_dev_handle_t *handle)
{
    int ret;

    ret = bb_ioctl(handle, BB_SET_POWER_TEST_MODE, NULL, NULL);
    if (ret) {
        printf("BB_SET_POWER_TEST_MODE failed, ret=%d\n", ret);
        return ret;
    }

    printf("BB_SET_POWER_TEST_MODE ok\n");
    return 0;
}

static void wait_before_next_command(void)
{
    usleep(POWER_FORCED_TX_CMD_INTERVAL_US);
}

int main(int argc, char **argv)
{
    const char *addr = "127.0.0.1";
    uint32_t port_value = BB_PORT_DEFAULT;
    uint32_t dev_index_value = 0;
    uint32_t freq_khz = 0;
    uint32_t power_dbm = 0;
    int freq_specified = 0;
    int power_specified = 0;
    int opt;
    int ret;
    bb_demo_context_t ctx;

    while ((opt = getopt(argc, argv, "ha:p:i:f:P:")) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'a':
            addr = optarg;
            break;
        case 'p':
            if (parse_u32_range(optarg, 1, 65535, "port", &port_value)) {
                return -1;
            }
            break;
        case 'i':
            if (parse_u32_range(optarg, 0, 255, "device index", &dev_index_value)) {
                return -1;
            }
            break;
        case 'f':
            if (parse_u32_range(optarg, 1, UINT32_MAX, "frequency", &freq_khz)) {
                return -1;
            }
            freq_specified = 1;
            break;
        case 'P':
            if (parse_u32_range(optarg, 0, 31, "power", &power_dbm)) {
                return -1;
            }
            power_specified = 1;
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    if (optind != argc) {
        printf("unexpected argument: %s\n", argv[optind]);
        usage(argv[0]);
        return -1;
    }

    if (!freq_specified || !power_specified) {
        printf("both -f <freq_khz> and -P <power_dbm> are required\n");
        usage(argv[0]);
        return -1;
    }

    ret = bb_demo_open(&ctx, addr, (int)port_value, (int)dev_index_value);
    if (ret) {
        return -1;
    }

    ret = set_debug_mode(ctx.handle);
    if (ret) {
        goto done;
    }

    wait_before_next_command();
    ret = disable_power_auto(ctx.handle);
    if (ret) {
        goto done;
    }

    wait_before_next_command();
    ret = set_tx_frequency(ctx.handle, freq_khz);
    if (ret) {
        goto done;
    }

    wait_before_next_command();
    ret = set_tx_power(ctx.handle, power_dbm);
    if (ret) {
        goto done;
    }

    wait_before_next_command();
    ret = enter_power_test_mode(ctx.handle);
    if (!ret) {
        printf("\npower forced TX started: user=%u freq=%u KHz power=%u dBm\n",
               BB_USER_BR_CS,
               freq_khz,
               power_dbm);
        printf("warning: the device remains in debug/power test mode after this program exits\n");
    }

done:
    bb_demo_close(&ctx);
    return ret ? -1 : 0;
}
