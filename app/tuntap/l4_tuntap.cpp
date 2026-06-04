#include "getopt.h"

#include "com_log.h"
#include "l4_tuntap.h"
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

// print usage
int usage(void)
{
    printf("This is a test program to demo baseband transport function!\n");
    printf("Option List:\n");
    printf(" -h --help              print usage which you're seeing\n");
    printf(" -u --user              specify the baseband user id\n");
    printf(" -p --port              specify the transport id\n");
    printf(" -i --ip                specify the tun device ip\n");
    printf(" -d --dev               specify the tun device name\n");
    printf(" -v --debug             debug mode in ethernet transfer\n");
    printf(" -r --rx_buf            buffer len of rx\n");
    printf(" -t --tx_buf            buffer len of tx\n");

    return 0;
}

#define BBCOM_SESSION_DATA_HEADER_SIZE      15
#define BBCOM_RX_BUF_SIZE                   (4000)
#define BBCOM_TX_FIFO_SIZE                  (4000)

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;


#define USR_PACKET_SYNC0                    0xFF
#define USR_PACKET_SYNC1                    0xA5
#define USR_PACKET_SYNC2                    0xAA
#define USR_PACKET_SYNC3                    0x5A
#define USR_PACKET_SYNC4                    0xFF

typedef struct
{
    uint8_t     sync[5];
    uint16_t    data_len;
    uint8_t     msg_id;
    uint8_t     rsv;
    uint8_t     user_id;
    uint8_t     channel;
    uint8_t     sum_num;
    uint8_t     cur_num;
    uint16_t    check_sum;
    uint8_t     payload[BBCOM_RX_BUF_SIZE];
} __attribute__((__packed__))  STRU_PACKET_MSG;




#define USR_FRAME_LEN                       sizeof(STRU_PACKET_MSG)

static uint8_t frame_buffer[USR_FRAME_LEN] = {0};


#define PACKET_HEADER_SIZE              15

typedef struct
{
    uint8_t                         find_header;
    uint8_t                         receiving_data;
    uint8_t                         data_length_index;
    uint8_t                         header_buf_index;
    uint8_t                         header_buf[BBCOM_SESSION_DATA_HEADER_SIZE];
    uint8_t                         rx_state;
    uint16_t                        data_buf_index;
    uint16_t                        data_length;
    uint16_t                        check_sum;
} STRU_BBComRxFIFOHeader;


typedef struct
{
    STRU_BBComRxFIFOHeader          rx_fifo_header;
    uint8_t                         rx_data_buf[BBCOM_RX_BUF_SIZE + 1];
} STRU_BBComRxFIFO;

typedef enum
{
    BB_COM_RX_HEADER = 0,
    BB_COM_RX_DATALENGTH,
    BB_COM_RX_MSG_ID,
    BB_COM_RX_HEADER_CHECKSUM,
    BB_COM_RX_DATABUFFER,
    BB_COM_RX_CHECKSUM,
} ENUM_BBComRxState;

static STRU_BBComRxFIFO network_BB_ComRxFIFO = {0};

static STRU_BBComRxFIFO network_BB_ComRxFIFO_List[4] = {0};

static uint8_t header[] = {0xFF, 0xA5, 0xAA, 0x5A, 0xFF};
#define HEADER_SYNC_SIZE                sizeof(header)



#define ARLINK_PACKET_CHECKSUM_POS      15


static void tun_2_bb_thread(bb_tun_cfg& cfg)
{
    // allocate buffer
    unsigned char* pkg_buf = (unsigned char*)malloc(cfg.buff_max + PACKET_HEADER_SIZE);

    if (!pkg_buf) {
        printf("tun2bb alloc memory error exit !!\n");
        return;
    }
 
    while (1) {
        int rdlen = cfg.dev.read(pkg_buf + PACKET_HEADER_SIZE, cfg.buff_max);

        if (rdlen <= 0) {
            continue;
        }

        if (cfg.debugflg) {
            int t = 0;
            int idx = 0;
            char buffer[300];
            memset(buffer, 0x00, 300);
            for (t = 0; t < 24; t++) {
                idx += std::sprintf(buffer + idx, "%02X ", pkg_buf[6 + t]);
            }
        
            printf("---------> tun read = %d,  %s\n", rdlen, buffer);
        }
        

        if (cfg.debugflg) {
            com_log(COM_SOCKET_DATA, "tun read = %d", rdlen);
        }


        memcpy(pkg_buf, header, sizeof(header));

        pkg_buf[5] = rdlen & 0x0FF;
        pkg_buf[6] = (rdlen >> 8) & 0x0FF;

        pkg_buf[7]  = 0x00;
        pkg_buf[8]  = 0x00;
        pkg_buf[9]  = 0x00;
        pkg_buf[10] = 0x00;
        pkg_buf[11] = 0x00;
        pkg_buf[12] = 0x00;
        

        uint16_t u16_checkSum = 0;
        /* header checksum */
        for (int i = 0; i < rdlen; i++) {
            u16_checkSum += pkg_buf[i + PACKET_HEADER_SIZE];
            /*
            u16_checkSum = u16_checkSum & 0x0FFFF;
            **/
        }
        
        /* header checksum */
        pkg_buf[13] = (u16_checkSum & 0x0FF);
        pkg_buf[14] = (u16_checkSum >> 8) & 0x0FF;
        
        int wrlen = bb_socket_write(cfg.bb_fd, pkg_buf, rdlen + PACKET_HEADER_SIZE, -1);
        if (cfg.debugflg) {
            com_log(COM_SOCKET_DATA, "bb write = %d", wrlen);
        }
    }
}


uint8_t Network_ComFindHeader(uint8_t u8_data, STRU_BBComRxFIFO *pstBBComRxFIFO)
{
    uint32_t                    check_sum = 0;
    uint8_t                     i;
    uint8_t                     ret_value = 0;
    STRU_BBComRxFIFOHeader     *rx_header = &(pstBBComRxFIFO->rx_fifo_header);

    switch (rx_header->rx_state)
    {
    case BB_COM_RX_HEADER:
        #if 0
        if (u8_data == header[0])    // Reset flag
        {
            rx_header->header_buf_index = 0;
            rx_header->header_buf[rx_header->header_buf_index++] = u8_data;
        }
        else if (rx_header->header_buf_index < sizeof(header))    // Get header
        {
            rx_header->header_buf[rx_header->header_buf_index++] = u8_data;

            if ((rx_header->header_buf_index == sizeof(header)) &&
                 (header[0] == rx_header->header_buf[0]) &&
                 (header[1] == rx_header->header_buf[1]) &&
                 (header[2] == rx_header->header_buf[2]) &&
                 (header[3] == rx_header->header_buf[3]) &&
                 (header[4] == rx_header->header_buf[4]))
            {
                rx_header->rx_state = BB_COM_RX_DATALENGTH;
                rx_header->data_length_index = 0;
            }
        }
        #else
        if (rx_header->header_buf_index >= HEADER_SYNC_SIZE) {
            printf("---------------------------> ERR: header_buf_index >= HEADER_SYNC_SIZE \n");
            rx_header->header_buf_index = 0;
        }

        rx_header->header_buf[rx_header->header_buf_index++] = u8_data;
        if (memcmp(rx_header->header_buf, header, rx_header->header_buf_index) != 0) {
            //memset(rx_header->header_buf, 0x00, HEADER_SYNC_SIZE);
            rx_header->header_buf[0] = u8_data;
            rx_header->header_buf_index = 1;
            break;
        }

        if (rx_header->header_buf_index == HEADER_SYNC_SIZE) {
            rx_header->rx_state = BB_COM_RX_DATALENGTH;
            rx_header->data_length_index = 0;
        }
        #endif
        break;

    case BB_COM_RX_DATALENGTH:
        rx_header->header_buf[rx_header->header_buf_index++] = u8_data;

        rx_header->data_length_index++;
        if (rx_header->data_length_index >= sizeof(uint16_t))
        {
            rx_header->data_length_index = 0;

            rx_header->rx_state = BB_COM_RX_MSG_ID;
        }
        break;

    case BB_COM_RX_MSG_ID:
        rx_header->header_buf[rx_header->header_buf_index++] = u8_data;
        rx_header->rx_state = BB_COM_RX_HEADER_CHECKSUM;
        break;
    case BB_COM_RX_HEADER_CHECKSUM:
        rx_header->header_buf[rx_header->header_buf_index++] = u8_data;
        if (rx_header->header_buf_index == PACKET_HEADER_SIZE) {
            check_sum = ((rx_header->header_buf[PACKET_HEADER_SIZE - 1] & 0x0FF) << 8) + (rx_header->header_buf[PACKET_HEADER_SIZE - 2] & 0x0FF);
        
            rx_header->data_length = (rx_header->header_buf[6] << 8) + rx_header->header_buf[5];
        
            if (rx_header->data_length <= BBCOM_RX_BUF_SIZE) {
                ret_value = 1;
            }

            rx_header->check_sum = (uint16_t)check_sum;
            rx_header->rx_state = BB_COM_RX_HEADER;
            rx_header->header_buf_index = 0;
        }
        break;

    default:
        rx_header->rx_state = BB_COM_RX_HEADER;
        rx_header->header_buf_index = 0;

        break;

    }

    return ret_value;
}


uint32_t BB_ComPacketDataAnalyze(bb_tun_cfg *cfg, uint8_t *u8_RxBuf, int u8_RxLen, STRU_BBComRxFIFO *pstBBComRxFIFO)
{
    uint16_t                    i = 0;
    uint16_t                    j = 0;
    uint8_t                     chData = '\0';
    uint16_t                    check_sum = 0;
    STRU_BBComRxFIFOHeader     *rx_header = &(pstBBComRxFIFO->rx_fifo_header);

    while (u8_RxLen)
    {
        chData = *(u8_RxBuf + i);

        i++;
        u8_RxLen--;

        
        if (rx_header->find_header == 0) {
            rx_header->find_header = Network_ComFindHeader(chData, pstBBComRxFIFO);
        }

        if (1 == rx_header->find_header)
        {
            if (rx_header->data_length > BBCOM_RX_BUF_SIZE)
            {
                printf("len should not exceed: %d\n", BBCOM_RX_BUF_SIZE);

                continue;
            }

            if (0 == rx_header->receiving_data)
            {
                /* begin to receive data */
                rx_header->receiving_data  = 1;

                rx_header->data_buf_index  = 0;

                //printf("find header, data_length: %d, index i =  %d ...\n", rx_header->data_length, i);
                
            }
            else
            {
                /* go on receiving data */
                pstBBComRxFIFO->rx_data_buf[rx_header->data_buf_index++] = chData;

                /* user data all received */
                if (rx_header->data_buf_index == rx_header->data_length) //2: checksum bytes
                {
                    int wrlen = cfg->dev.write(pstBBComRxFIFO->rx_data_buf, rx_header->data_buf_index);
                    if (cfg->debugflg) {
                        com_log(COM_SOCKET_DATA, "tun write = %d", wrlen);
                    }

                    //printf("receiving_data finished,  data_length: %d, i = %d  ...\n", rx_header->data_length, i);
                    rx_header->data_buf_index = 0;
                    rx_header->receiving_data = 0;
                    rx_header->find_header = 0;
                }
            }
        }
    }

    return 0;
}

static void bb_2_tun_thread(bb_tun_cfg& cfg)
{
    // allocate buffer
    uint8_t * pkg_buf = (uint8_t *)malloc(cfg.buff_max);
    if (!pkg_buf) {
        printf("bb2tun alloc memory error exit !!\n");
        return;
    }

    int offset = 0;
    while (1) {
        int len = bb_socket_read(cfg.bb_fd, pkg_buf, cfg.buff_max, -1);
        if (len <= 0) {
            // 基带断开链接
            printf("---------------------> bb_socket_read < 0 \n");
            sleep(2);
            continue;
        }

        if (cfg.debugflg) {
            com_log(COM_SOCKET_DATA, "bb read raw= %d", len);
        }


        if (cfg.debugflg)
            printf("----------------------------------------> start bb tun write = %d 0x%x\n", len, len);

        #if 1
        if (len > 0) {
            BB_ComPacketDataAnalyze(&cfg, pkg_buf, len, &network_BB_ComRxFIFO);
            if (cfg.debugflg) {
                printf("-----------------------------> BB_ComPacketDataAnalyze finished\n");
            }
        }

        #else
        int wrlen = cfg.dev.write(pkg_buf, len);
        if (cfg.debugflg) {
            com_log(COM_SOCKET_DATA, "tun write = %d", wrlen);
        }
        #endif
    }
}

static int tun_test(bb_tun_cfg& cfg)
{
    int ret = bb_host_connect(&cfg.phost, "127.0.0.1", BB_PORT_DEFAULT);
    if (ret) {
        printf("connect failed = %d\n", ret);
        return ret;
    }
    bb_sock_opt_t opt;

    bb_dev_t** devs;

    int sz = bb_dev_getlist(cfg.phost, &devs);
    if (sz <= 0) {
        printf("dev cnt = 0\n");
        return sz;
    }

    cfg.pdev = bb_dev_open(devs[0]);
    if (!cfg.pdev) {
        printf("can't open dev!!!\n");
        return -1;
    }

#if 0
    if (cfg.ipset_flg) {
        cfg.tun_fd = tun_alloc(cfg.devname, cfg.ip, "255.255.255.0", cfg.mtu);
        printf("dev = %s,ip = %s,tun_fd = %d\n", cfg.devname, cfg.ip, cfg.tun_fd);
    } else {
        cfg.tun_fd = tun_alloc(cfg.devname, nullptr, nullptr, cfg.mtu);
        printf("dev = %s,tun_fd = %d\n", cfg.devname, cfg.tun_fd);
    }
#else
    cfg.dev.name(cfg.devname);
    if (cfg.ipset_flg) {
        cfg.dev.ip(cfg.ip, 24);
        cfg.dev.mtu(cfg.mtu);
        printf("dev = %s,ip = %s,mtu = %d , tun_fd = %" PRIi64 "\n",
               cfg.devname,
               cfg.ip,
               cfg.dev.mtu(),
               (uint64_t)cfg.dev.native_handle());
    }
    cfg.dev.up();
#endif

    // open bb port
    opt.rx_buf_size = cfg.rx_buf_len;
    opt.tx_buf_size = cfg.tx_buf_len;

    cfg.bb_fd = bb_socket_open(cfg.pdev,
                               cfg.slot_id,
                               cfg.port_id,
                               BB_SOCK_FLAG_RX | BB_SOCK_FLAG_TX,
                               &opt);
    if (cfg.bb_fd < 0) {
        printf("create bb socket failed!\n");
		return 0;
    } else {
        printf("lgeng - 0  ar_bb_socket_fd = %d\n", cfg.bb_fd);
    }

    std::thread tun_bb(tun_2_bb_thread, std::ref(cfg));
    std::thread bb_tun(bb_2_tun_thread, std::ref(cfg));

    tun_bb.join();
    bb_tun.join();

    return 0;
}

int main(int argc, char* argv[])
{
    int         opt           = 0;
    int         flag_help     = 0;
    const char* short_options = "hp:i:u:d:vr:t:";
    uint32_t    rx_buffer     = 40000;
    uint32_t    tx_buffer     = 60000;


    bb_tun_cfg    cfg;
    struct option long_options[] = {
        {"help",   no_argument,       NULL, 'h'},
        { "port",  required_argument, NULL, 'p'},
        { "ip",    required_argument, NULL, 'i'},
        { "user",  required_argument, NULL, 'u'},
        { "dev",   required_argument, NULL, 'e'},
        { "debug", no_argument,       NULL, 'v'},
        { "rx_buf",required_argument, NULL, 'r'},
        { "tx_buf",required_argument, NULL, 't'},
        { 0,       0,                 0,    0  },
    };

    while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            flag_help = 1;
            break;
        case 'p':
            cfg.port_id = strtoul(optarg, NULL, 10);
            break;
        case 'i':
            strcpy(cfg.ip, optarg);
            cfg.ipset_flg = 1;
            break;
        case 'u':
            cfg.slot_id = (bb_slot_e)strtoul(optarg, NULL, 10);
            break;
        case 'd':
            strcpy(cfg.devname, optarg);
            break;
        case 'v':
            printf("Set cfg.debugflg = 1 \n");
            cfg.debugflg = 1;
            break;
        case 'r':
            cfg.rx_buf_len = strtoul(optarg, NULL, 10);
            printf("set   cfg.rx_buf_len  %d \n",  cfg.rx_buf_len );
            break;
        case 't':
            cfg.tx_buf_len = strtoul(optarg, NULL, 10);
            printf("set   cfg.tx_buf_len  %d \n",  cfg.tx_buf_len );
            break;
        default:
            printf("unknown option\n");
            break;
        }
    }

    if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }

    if (flag_help) {
        return usage();
    }

    return tun_test(cfg);
}

