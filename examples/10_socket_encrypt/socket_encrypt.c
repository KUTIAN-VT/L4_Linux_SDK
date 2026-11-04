#include "bb_demo_common.h"

#include "getopt.h"
#include "prj_rpc.h"
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINIDB_UNSET_RET (-8)
#define MINIDB_RESULT_UNSET 1

typedef enum {
    ACTION_NONE = 0,
    ACTION_ENABLE,
    ACTION_DISABLE,
} action_t;

typedef struct {
    const char *addr;
    int daemon_port;
    int dev_index;
    int remote_slot;
    int slot;
    int socket_port;
    bool get_mode;
    bool minidb;
    action_t action;
    bb_sock_encrypt_mode_t encrypt_mode;
    const char *encrypt_key_text;
    uint8_t encrypt_key[BB_SOCK_ENCRYPT_KEY_LEN];
} options_t;

static void usage(const char *prog)
{
    printf("Usage: %s [options] <action>\n", prog);
    printf("\n");
    printf("Common options:\n");
    printf("  -h, --help                 show this help\n");
    printf("  -a, --addr <addr>          daemon address, default: 127.0.0.1\n");
    printf("  -p, --port <port>          daemon port, default: %d\n", BB_PORT_DEFAULT);
    printf("  -i, --index <index>        device index, default: 0\n");
    printf("  -R [slot], --remote[=<slot>]\n");
    printf("                             operate peer by remote ioctl, default remote slot: 0\n");
    printf("  -s, --slot <slot>          target slot, default: 0; DEV uses slot 0 as AP\n");
    printf("  -P, --socket-port <port>   target socket port, required\n");
    printf("  -g, --get                  query the current config of the socket port\n");
    printf("      --minidb               read/write persistent MiniDB config only\n");
    printf("\n");
    printf("Actions:\n");
    printf("  -e, --encrypt-mode <mode>  enable/update encryption: default, aes128 or aes256\n");
    printf("  -d, --disable              disable encryption\n");
    printf("\n");
    printf("Encryption options:\n");
    printf("  -k, --encrypt-key <hex>    AES key: 32 hex digits for AES128, 64 for AES256\n");
}

static void init_options(options_t *opts)
{
    memset(opts, 0, sizeof(*opts));
    opts->addr = "127.0.0.1";
    opts->daemon_port = BB_PORT_DEFAULT;
    opts->dev_index = 0;
    opts->remote_slot = -1;
    opts->slot = BB_SLOT_0;
    opts->socket_port = -1;
    opts->get_mode = false;
    opts->minidb = false;
    opts->action = ACTION_NONE;
    opts->encrypt_mode = BB_SOCK_ENCRYPT_MODE_DEFAULT;
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

static int set_action(options_t *opts, action_t action)
{
    if (opts->action != ACTION_NONE) {
        printf("only one action can be specified\n");
        return -1;
    }

    opts->action = action;
    return 0;
}

static int parse_encrypt_mode(const char *text, bb_sock_encrypt_mode_t *mode)
{
    if (strcmp(text, "default") == 0) {
        *mode = BB_SOCK_ENCRYPT_MODE_DEFAULT;
    } else if (strcmp(text, "aes128") == 0) {
        *mode = BB_SOCK_ENCRYPT_MODE_AES128;
    } else if (strcmp(text, "aes256") == 0) {
        *mode = BB_SOCK_ENCRYPT_MODE_AES256;
    } else {
        printf("invalid encryption mode: %s; use default, aes128 or aes256\n", text);
        return -1;
    }

    return 0;
}

static int is_hex_separator(char c)
{
    return isspace((unsigned char)c) || c == ':' || c == ',';
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

static int parse_hex_key(const char *text, uint8_t *key, size_t expected_len)
{
    const char *p = text;
    size_t len = 0;

    while (*p) {
        int hi;
        int lo;

        while (is_hex_separator(*p)) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }

        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            p += 2;
        }

        hi = hex_value(*p);
        if (hi < 0) {
            printf("invalid encryption key hex character: %c\n", *p ? *p : '?');
            return -1;
        }
        ++p;

        lo = hex_value(*p);
        if (lo < 0) {
            printf("encryption key must use two hex digits per byte\n");
            return -1;
        }
        ++p;

        if (len >= expected_len) {
            printf("encryption key is longer than %zu bytes\n", expected_len);
            return -1;
        }
        key[len++] = (uint8_t)((hi << 4) | lo);
    }

    if (len != expected_len) {
        printf("invalid encryption key length: got %zu bytes, expected %zu\n", len, expected_len);
        return -1;
    }

    return 0;
}

static const char *short_option_name(int opt)
{
    switch (opt) {
    case 'a':
        return "-a/--addr";
    case 'p':
        return "-p/--port";
    case 'i':
        return "-i/--index";
    case 'R':
        return "-R/--remote";
    case 's':
        return "-s/--slot";
    case 'P':
        return "-P/--socket-port";
    case 'g':
        return "-g/--get";
    case 'e':
        return "-e/--encrypt-mode";
    case 'k':
        return "-k/--encrypt-key";
    default:
        return "option";
    }
}

static int validate_options(options_t *opts)
{
    size_t key_len;

    if (opts->socket_port < 0) {
        printf("-P/--socket-port is required\n");
        return -1;
    }
    if (opts->minidb && opts->socket_port >= BB_CONFIG_MAX_TRANSPORT_PER_SLOT) {
        printf("MiniDB socket port must be in range 0-%d\n",
               BB_CONFIG_MAX_TRANSPORT_PER_SLOT - 1);
        return -1;
    }
    if (opts->get_mode) {
        if (opts->action != ACTION_NONE || opts->encrypt_key_text) {
            printf("-g/--get cannot be combined with -e, -d or -k\n");
            return -1;
        }
        return 0;
    }
    if (opts->action == ACTION_NONE) {
        printf("one action is required: -e/--encrypt-mode or -d/--disable\n");
        return -1;
    }
    if (opts->action == ACTION_DISABLE) {
        if (opts->encrypt_key_text) {
            printf("-d/--disable does not use -k/--encrypt-key\n");
            return -1;
        }
        return 0;
    }
    if (opts->encrypt_mode == BB_SOCK_ENCRYPT_MODE_DEFAULT) {
        if (opts->encrypt_key_text) {
            printf("default encryption mode does not use -k/--encrypt-key\n");
            return -1;
        }
        return 0;
    }
    if (!opts->encrypt_key_text) {
        printf("%s requires -k/--encrypt-key\n",
               opts->encrypt_mode == BB_SOCK_ENCRYPT_MODE_AES128 ? "aes128" : "aes256");
        return -1;
    }

    key_len = opts->encrypt_mode == BB_SOCK_ENCRYPT_MODE_AES128 ? 16 : BB_SOCK_ENCRYPT_KEY_LEN;
    return parse_hex_key(opts->encrypt_key_text, opts->encrypt_key, key_len);
}

static int parse_options(int argc, char **argv, options_t *opts)
{
    static const struct option long_options[] = {
        {"addr", required_argument, NULL, 'a'},
        {"port", required_argument, NULL, 'p'},
        {"index", required_argument, NULL, 'i'},
        {"remote", optional_argument, NULL, 'R'},
        {"slot", required_argument, NULL, 's'},
        {"socket-port", required_argument, NULL, 'P'},
        {"get", no_argument, NULL, 'g'},
        {"minidb", no_argument, NULL, 1000},
        {"encrypt-mode", required_argument, NULL, 'e'},
        {"encrypt-key", required_argument, NULL, 'k'},
        {"disable", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0},
    };
    int opt;

    init_options(opts);
    opterr = 0;

    while ((opt = getopt_long(argc, argv, ":ha:p:i:R::s:P:e:k:dg", long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 1;
        case 'a':
            opts->addr = optarg;
            break;
        case 'p':
            if (parse_int_range(optarg, 1, 65535, "daemon port", &opts->daemon_port)) {
                return -1;
            }
            break;
        case 'i':
            if (parse_int_range(optarg, 0, 255, "device index", &opts->dev_index)) {
                return -1;
            }
            break;
        case 'R': {
            const char *remote_slot_text = optarg;

            if (opts->remote_slot >= 0) {
                printf("remote option may only be specified once\n");
                return -1;
            }

            opts->remote_slot = BB_SLOT_0;
            if (!remote_slot_text && optind < argc &&
                strcmp(argv[optind - 1], "-R") == 0 &&
                argv[optind][0] != '-') {
                remote_slot_text = argv[optind++];
            }

            if (remote_slot_text &&
                parse_int_range(remote_slot_text,
                                BB_SLOT_0,
                                BB_SLOT_MAX - 1,
                                "remote slot",
                                &opts->remote_slot)) {
                return -1;
            }
            break;
        }
        case 's':
            if (parse_int_range(optarg, BB_SLOT_0, BB_SLOT_MAX - 1, "slot", &opts->slot)) {
                return -1;
            }
            break;
        case 'P':
            if (parse_int_range(optarg, 0, 255, "socket port", &opts->socket_port)) {
                return -1;
            }
            break;
        case 'g':
            opts->get_mode = true;
            break;
        case 1000:
            opts->minidb = true;
            break;
        case 'e':
            if (parse_encrypt_mode(optarg, &opts->encrypt_mode) || set_action(opts, ACTION_ENABLE)) {
                return -1;
            }
            break;
        case 'k':
            opts->encrypt_key_text = optarg;
            break;
        case 'd':
            if (set_action(opts, ACTION_DISABLE)) {
                return -1;
            }
            break;
        case ':':
            printf("missing argument for %s\n", short_option_name(optopt));
            return -1;
        case '?':
            if (optopt) {
                printf("unknown option: -%c\n", optopt);
            } else {
                printf("unknown option\n");
            }
            return -1;
        default:
            return -1;
        }
    }

    if (optind < argc) {
        printf("unexpected argument: %s\n", argv[optind]);
        return -1;
    }

    return validate_options(opts);
}

static const char *encrypt_mode_name(bb_sock_encrypt_mode_t mode)
{
    switch (mode) {
    case BB_SOCK_ENCRYPT_MODE_DEFAULT:
        return "default";
    case BB_SOCK_ENCRYPT_MODE_AES128:
        return "aes128";
    case BB_SOCK_ENCRYPT_MODE_AES256:
        return "aes256";
    default:
        return "unknown";
    }
}

static int socket_encrypt_ioctl(bb_dev_handle_t *handle,
                                uint32_t request,
                                const void *input,
                                uint16_t input_len,
                                void *output,
                                uint16_t output_len,
                                int remote_slot)
{
    bb_remote_ioctl_in_t remote_in = {0};
    bb_remote_ioctl_out_t remote_out = {0};
    int ret;

    if (remote_slot < 0) {
        return bb_ioctl(handle, request, input, output);
    }

    if ((input_len && !input) || (output_len && !output) ||
        input_len > sizeof(remote_in.data) || output_len > sizeof(remote_out.data)) {
        printf("remote ioctl buffer invalid, request=0x%x in_len=%u out_len=%u\n",
               request,
               input_len,
               output_len);
        return -1;
    }

    if (input_len) {
        memcpy(remote_in.data, input, input_len);
    }
    remote_in.slot = (uint8_t)remote_slot;
    remote_in.len = input_len;
    remote_in.msg_id = request;

    ret = bb_ioctl(handle, BB_REMOTE_IOCTL_REQ, &remote_in, &remote_out);
    if (!ret && output_len) {
        memcpy(output, remote_out.data, output_len);
    }

    memset(&remote_in, 0, sizeof(remote_in));
    memset(&remote_out, 0, sizeof(remote_out));
    return ret;
}

static int minidb_set_dispatch(bb_dev_handle_t *handle,
                               const prj_cmd_set_sock_encrypt_t *config,
                               int remote_slot)
{
    bb_set_prj_dispatch_in_t request = {0};
    prj_rpc_hdr_t *hdr = (prj_rpc_hdr_t *)request.data;
    size_t max_payload = sizeof(request.data) - sizeof(*hdr);
    int ret;

    if (sizeof(*config) > max_payload) {
        printf("MiniDB set payload is too large\n");
        return -1;
    }

    hdr->cmdid = PRJ_CMD_SET_SOCK_ENCRYPT;
    memcpy(hdr->data, config, sizeof(*config));
    ret = socket_encrypt_ioctl(handle,
                               BB_SET_PRJ_DISPATCH,
                               &request,
                               sizeof(request),
                               NULL,
                               0,
                               remote_slot);
    memset(&request, 0, sizeof(request));
    return ret;
}

static int minidb_get_dispatch(bb_dev_handle_t *handle,
                               const prj_cmd_get_sock_encrypt_in_t *query,
                               prj_cmd_get_sock_encrypt_out_t *result,
                               int remote_slot)
{
    bb_get_prj_dispatch_in_t request = {0};
    bb_get_prj_dispatch_out_t response = {0};
    prj_rpc_hdr_t *hdr = (prj_rpc_hdr_t *)request.data;
    const prj_rpc_hdr_t *out_hdr;
    size_t max_payload = sizeof(request.data) - sizeof(*hdr);
    int ret;

    if (sizeof(*query) > max_payload || sizeof(*result) > max_payload) {
        printf("MiniDB get payload is too large\n");
        return -1;
    }

    hdr->cmdid = PRJ_CMD_GET_SOCK_ENCRYPT;
    memcpy(hdr->data, query, sizeof(*query));
    ret = socket_encrypt_ioctl(handle,
                               BB_GET_PRJ_DISPATCH,
                               &request,
                               sizeof(request),
                               &response,
                               sizeof(response),
                               remote_slot);
    if (ret == MINIDB_UNSET_RET) {
        ret = MINIDB_RESULT_UNSET;
        goto out;
    }
    if (ret) {
        goto out;
    }

    out_hdr = (const prj_rpc_hdr_t *)response.data;
    memcpy(result, out_hdr->data, sizeof(*result));

out:
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    return ret;
}

static void print_minidb_result(const options_t *opts,
                                const prj_cmd_get_sock_encrypt_out_t *result)
{
    size_t key_len = 0;

    if (result->encrypt.encrypt_en &&
        result->encrypt.encrypt_mode == BB_SOCK_ENCRYPT_MODE_AES128) {
        key_len = 16;
    } else if (result->encrypt.encrypt_en &&
               result->encrypt.encrypt_mode == BB_SOCK_ENCRYPT_MODE_AES256) {
        key_len = BB_SOCK_ENCRYPT_KEY_LEN;
    }

    printf("%spersistent socket encryption: slot=%d port=%d enabled=%u mode=%s key=",
           opts->remote_slot >= 0 ? "remote " : "",
           opts->slot,
           opts->socket_port,
           result->encrypt.encrypt_en,
           encrypt_mode_name((bb_sock_encrypt_mode_t)result->encrypt.encrypt_mode));
    if (key_len == 0) {
        printf("none\n");
        return;
    }

    for (size_t i = 0; i < key_len; ++i) {
        printf("%02x", result->encrypt.key[i]);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    options_t opts;
    bb_demo_context_t ctx;
    bb_set_force_upd_socket_enc_t request = {0};
    bb_get_socket_enc_in_t query = {0};
    bb_get_socket_enc_out_t result = {0};
    prj_cmd_set_sock_encrypt_t minidb_config = {0};
    prj_cmd_get_sock_encrypt_in_t minidb_query = {0};
    prj_cmd_get_sock_encrypt_out_t minidb_result = {0};
    int ret;

    ret = parse_options(argc, argv, &opts);
    if (ret > 0) {
        memset(opts.encrypt_key, 0, sizeof(opts.encrypt_key));
        return 0;
    }
    if (ret < 0) {
        memset(opts.encrypt_key, 0, sizeof(opts.encrypt_key));
        return 1;
    }

    if (opts.minidb && opts.get_mode) {
        minidb_query.slot = (uint8_t)opts.slot;
        minidb_query.port = (uint8_t)opts.socket_port;
        printf("query %spersistent socket encryption: slot=%d port=%d\n",
               opts.remote_slot >= 0 ? "remote " : "",
               opts.slot,
               opts.socket_port);
    } else if (opts.get_mode) {
        query.slot = (uint8_t)opts.slot;
        query.port = (uint8_t)opts.socket_port;
        if (opts.remote_slot >= 0) {
            printf("query remote socket encryption: remote_slot=%d slot=%d port=%d\n",
                   opts.remote_slot,
                   opts.slot,
                   opts.socket_port);
        } else {
            printf("query socket encryption: slot=%d port=%d\n",
                   opts.slot,
                   opts.socket_port);
        }
    } else if (opts.minidb) {
        minidb_config.slot = (uint8_t)opts.slot;
        minidb_config.port = (uint8_t)opts.socket_port;
        if (opts.action == ACTION_ENABLE) {
            minidb_config.encrypt.encrypt_en = 1;
            minidb_config.encrypt.encrypt_mode = (uint8_t)opts.encrypt_mode;
            memcpy(minidb_config.encrypt.key,
                   opts.encrypt_key,
                   sizeof(minidb_config.encrypt.key));
        }
        printf("write %spersistent socket encryption: slot=%d port=%d action=%s mode=%s\n",
               opts.remote_slot >= 0 ? "remote " : "",
               opts.slot,
               opts.socket_port,
               opts.action == ACTION_ENABLE ? "enable" : "disable",
               opts.action == ACTION_ENABLE ? encrypt_mode_name(opts.encrypt_mode) : "none");
    } else {
        request.slot = (uint8_t)opts.slot;
        request.port = (uint8_t)opts.socket_port;
        if (opts.action == ACTION_ENABLE) {
            request.sock_upd_enc.encrypt_en = 1;
            request.sock_upd_enc.encrypt_mode = (uint8_t)opts.encrypt_mode;
            memcpy(request.sock_upd_enc.key, opts.encrypt_key, sizeof(request.sock_upd_enc.key));
            if (opts.remote_slot >= 0) {
                printf("update remote socket encryption: remote_slot=%d slot=%d port=%d action=enable mode=%s\n",
                       opts.remote_slot,
                       opts.slot,
                       opts.socket_port,
                       encrypt_mode_name(opts.encrypt_mode));
            } else {
                printf("update socket encryption: slot=%d port=%d action=enable mode=%s\n",
                       opts.slot,
                       opts.socket_port,
                       encrypt_mode_name(opts.encrypt_mode));
            }
        } else if (opts.remote_slot >= 0) {
            printf("update remote socket encryption: remote_slot=%d slot=%d port=%d action=disable\n",
                   opts.remote_slot,
                   opts.slot,
                   opts.socket_port);
        } else {
            printf("update socket encryption: slot=%d port=%d action=disable\n",
                   opts.slot,
                   opts.socket_port);
        }
    }

    ret = bb_demo_open(&ctx, opts.addr, opts.daemon_port, opts.dev_index);
    if (ret) {
        memset(opts.encrypt_key, 0, sizeof(opts.encrypt_key));
        memset(&request, 0, sizeof(request));
        memset(&minidb_config, 0, sizeof(minidb_config));
        return 1;
    }

    if (opts.minidb && opts.get_mode) {
        ret = minidb_get_dispatch(ctx.handle,
                                  &minidb_query,
                                  &minidb_result,
                                  opts.remote_slot);
        if (ret == MINIDB_RESULT_UNSET) {
            printf("%spersistent socket encryption: slot=%d port=%d state=unset\n",
                   opts.remote_slot >= 0 ? "remote " : "",
                   opts.slot,
                   opts.socket_port);
            ret = 0;
        } else if (ret) {
            printf("query persistent socket encryption failed, ret=%d\n", ret);
        } else {
            print_minidb_result(&opts, &minidb_result);
        }
    } else if (opts.get_mode) {
        ret = socket_encrypt_ioctl(ctx.handle,
                                   BB_GET_SOCKET_ENCRYPT,
                                   &query,
                                   sizeof(query),
                                   &result,
                                   sizeof(result),
                                   opts.remote_slot);
        if (ret) {
            printf("query socket encryption failed, ret=%d\n", ret);
        } else if (opts.remote_slot >= 0) {
            printf("remote socket encryption: remote_slot=%d slot=%d port=%d enabled=%u mode=%s\n",
                   opts.remote_slot,
                   opts.slot,
                   opts.socket_port,
                   result.encrypt_en,
                   encrypt_mode_name((bb_sock_encrypt_mode_t)result.encrypt_mode));
        } else {
            printf("socket encryption: slot=%d port=%d enabled=%u mode=%s\n",
                   opts.slot,
                   opts.socket_port,
                   result.encrypt_en,
                   encrypt_mode_name((bb_sock_encrypt_mode_t)result.encrypt_mode));
        }
    } else if (opts.minidb) {
        ret = minidb_set_dispatch(ctx.handle, &minidb_config, opts.remote_slot);
        if (ret) {
            printf("write persistent socket encryption failed, ret=%d\n", ret);
        } else {
            printf("write %spersistent socket encryption success: slot=%d port=%d; "
                   "takes effect after socket recreation or device reboot\n",
                   opts.remote_slot >= 0 ? "remote " : "",
                   opts.slot,
                   opts.socket_port);
        }
    } else {
        ret = socket_encrypt_ioctl(ctx.handle,
                                   BB_SET_FORCE_UPD_SOCKET_ENC,
                                   &request,
                                   sizeof(request),
                                   NULL,
                                   0,
                                   opts.remote_slot);
        if (ret) {
            if (opts.remote_slot >= 0) {
                printf("update remote socket encryption failed: remote_slot=%d slot=%d port=%d ret=%d\n",
                       opts.remote_slot,
                       opts.slot,
                       opts.socket_port,
                       ret);
            } else {
                printf("update socket encryption failed, ret=%d\n", ret);
            }
        } else if (opts.remote_slot >= 0) {
            printf("update remote socket encryption success: remote_slot=%d slot=%d port=%d\n",
                   opts.remote_slot,
                   opts.slot,
                   opts.socket_port);
        } else {
            printf("update socket encryption success: slot=%d port=%d\n",
                   opts.slot,
                   opts.socket_port);
        }
    }

    memset(opts.encrypt_key, 0, sizeof(opts.encrypt_key));
    memset(&request, 0, sizeof(request));
    memset(&minidb_config, 0, sizeof(minidb_config));
    memset(&minidb_result, 0, sizeof(minidb_result));
    bb_demo_close(&ctx);
    return ret ? 1 : 0;
}
