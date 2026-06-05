#include "bb_demo_common.h"

#include "getopt.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("  -a <addr>       host address, default: 127.0.0.1\n");
    printf("  -p <port>       host port, default: %d\n", BB_PORT_DEFAULT);
    printf("  -i <index>      device index, default: 0\n");
    printf("  -B <0|1>        set BB_SET_BAND_MODE, 1=auto, 0=manual\n");
    printf("  -b <band>       set BB_SET_BAND, band: 1g/2g/5g or 0/1/2\n");
    printf("  -C <0|1>        set BB_SET_CHAN_MODE, 1=auto, 0=manual\n");
    printf("  -c <index>      set BB_SET_CHAN channel index, range: 0-255\n");
    printf("  -d <dir>        channel direction for -c, tx/rx or 0/1, default: rx\n");
    printf("  -h              show this help\n");
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

static int parse_int_range(const char *text, int min_value, int max_value, const char *name, int *out)
{
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno || end == text || *end != '\0' || value < min_value || value > max_value) {
        printf("invalid %s: %s, valid range: %d-%d\n", name, text, min_value, max_value);
        return -1;
    }

    *out = (int)value;
    return 0;
}

static int parse_auto_mode(const char *text, const char *name, int *out)
{
    return parse_int_range(text, 0, 1, name, out);
}

static int parse_dir(const char *text, int *out)
{
    if (str_eq_ignore_case(text, "tx")) {
        *out = BB_DIR_TX;
        return 0;
    }

    if (str_eq_ignore_case(text, "rx")) {
        *out = BB_DIR_RX;
        return 0;
    }

    return parse_int_range(text, BB_DIR_TX, BB_DIR_RX, "dir", out);
}

static int parse_band(const char *text, int *out)
{
    if (str_eq_ignore_case(text, "1g") || str_eq_ignore_case(text, "1.4g") ||
        str_eq_ignore_case(text, "14g")) {
        *out = BB_BAND_1G;
        return 0;
    }

    if (str_eq_ignore_case(text, "2g")) {
        *out = BB_BAND_2G;
        return 0;
    }

    if (str_eq_ignore_case(text, "5g")) {
        *out = BB_BAND_5G;
        return 0;
    }

    return parse_int_range(text, BB_BAND_1G, BB_BAND_5G, "band", out);
}

static const char *dir_name(int dir)
{
    switch (dir) {
    case BB_DIR_TX:
        return "TX";
    case BB_DIR_RX:
        return "RX";
    default:
        return "UNKNOWN";
    }
}

static const char *band_name(int band)
{
    switch (band) {
    case BB_BAND_1G:
        return "1G";
    case BB_BAND_2G:
        return "2G";
    case BB_BAND_5G:
        return "5G";
    default:
        return "UNKNOWN";
    }
}

static int set_band_mode(bb_dev_handle_t *handle, int auto_mode)
{
    bb_set_band_mode_t input;
    int ret;

    memset(&input, 0, sizeof(input));
    input.auto_mode = (uint8_t)auto_mode;

    ret = bb_ioctl(handle, BB_SET_BAND_MODE, &input, NULL);
    printf("BB_SET_BAND_MODE auto_mode=%u ret=%d\n", input.auto_mode, ret);
    return ret;
}

static int set_band(bb_dev_handle_t *handle, int target_band)
{
    bb_set_band_t input;
    int ret;

    memset(&input, 0, sizeof(input));
    input.target_band = (uint8_t)target_band;

    ret = bb_ioctl(handle, BB_SET_BAND, &input, NULL);
    printf("BB_SET_BAND target_band=%s(%u) ret=%d\n",
           band_name(input.target_band),
           input.target_band,
           ret);
    return ret;
}

static int set_chan_mode(bb_dev_handle_t *handle, int auto_mode)
{
    bb_set_chan_mode_t input;
    int ret;

    memset(&input, 0, sizeof(input));
    input.auto_mode = (uint8_t)auto_mode;

    ret = bb_ioctl(handle, BB_SET_CHAN_MODE, &input, NULL);
    printf("BB_SET_CHAN_MODE auto_mode=%u ret=%d\n", input.auto_mode, ret);
    return ret;
}

static int set_chan(bb_dev_handle_t *handle, int chan_dir, int chan_index)
{
    bb_set_chan_t input;
    int ret;

    memset(&input, 0, sizeof(input));
    input.chan_dir = (uint8_t)chan_dir;
    input.chan_index = (uint8_t)chan_index;

    ret = bb_ioctl(handle, BB_SET_CHAN, &input, NULL);
    printf("BB_SET_CHAN dir=%s(%u) chan_index=%u ret=%d\n",
           dir_name(input.chan_dir),
           input.chan_dir,
           input.chan_index,
           ret);
    return ret;
}

int main(int argc, char **argv)
{
    const char *addr = "127.0.0.1";
    int port = BB_PORT_DEFAULT;
    int dev_index = 0;
    int chan_mode = 0;
    int chan_index = 0;
    int chan_dir = BB_DIR_RX;
    int band_mode = 0;
    int target_band = 0;
    int do_chan_mode = 0;
    int do_chan = 0;
    int do_band_mode = 0;
    int do_band = 0;
    int opt;
    int ret = 0;
    bb_demo_context_t ctx;

    while ((opt = getopt(argc, argv, "ha:p:i:C:c:d:B:b:")) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'a':
            addr = optarg;
            break;
        case 'p':
            if (parse_int_range(optarg, 1, 65535, "port", &port)) {
                return -1;
            }
            break;
        case 'i':
            if (parse_int_range(optarg, 0, 255, "device index", &dev_index)) {
                return -1;
            }
            break;
        case 'C':
            if (parse_auto_mode(optarg, "chan auto_mode", &chan_mode)) {
                return -1;
            }
            do_chan_mode = 1;
            break;
        case 'c':
            if (parse_int_range(optarg, 0, 255, "channel index", &chan_index)) {
                return -1;
            }
            do_chan = 1;
            break;
        case 'd':
            if (parse_dir(optarg, &chan_dir)) {
                return -1;
            }
            break;
        case 'B':
            if (parse_auto_mode(optarg, "band auto_mode", &band_mode)) {
                return -1;
            }
            do_band_mode = 1;
            break;
        case 'b':
            if (parse_band(optarg, &target_band)) {
                return -1;
            }
            do_band = 1;
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    if (!do_chan_mode && !do_chan && !do_band_mode && !do_band) {
        printf("no link config action specified\n");
        usage(argv[0]);
        return -1;
    }

    ret = bb_demo_open(&ctx, addr, port, dev_index);
    if (ret) {
        return -1;
    }

    if (do_band_mode) {
        ret = set_band_mode(ctx.handle, band_mode);
        if (ret) {
            goto done;
        }
    }

    if (do_band) {
        ret = set_band(ctx.handle, target_band);
        if (ret) {
            goto done;
        }
    }

    if (do_chan_mode) {
        ret = set_chan_mode(ctx.handle, chan_mode);
        if (ret) {
            goto done;
        }
    }

    if (do_chan) {
        ret = set_chan(ctx.handle, chan_dir, chan_index);
        if (ret) {
            goto done;
        }
    }

    printf("\nl4 link config finished\n");

done:
    bb_demo_close(&ctx);
    return ret ? -1 : 0;
}
