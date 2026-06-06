#ifndef __L4_TUNTAP_H__
#define __L4_TUNTAP_H__

#include "bb_api.h"
#include "tuntap++.hh"
#include "string.h"
#include <memory>
struct bb_tun_cfg {
    // bb field
    int bb_fd = -1;

    bb_host_t*       phost   = nullptr;
    bb_dev_handle_t* pdev    = nullptr;
    bb_slot_e        slot_id = BB_SLOT_0;
    int              port_id = 3;
    // tun field
    int  ipset_flg    = 0;
    int  mtu          = 4000;
    char devname[128] = { 0 };
    char ip[128]      = { 0 };
    char mask[128]    = { 0 };
    // int  tun_fd       = -1;
    std::unique_ptr<tuntap::tap> dev;
    // common
    int debugflg = 0;
    int buff_max = 4096;
    uint32_t rx_buf_len = 40000;
    uint32_t tx_buf_len = 60000;

    bb_tun_cfg()
    {
        strcpy(devname, "tap0");
        ip[0] = '\0';
        strcpy(mask, "255.255.255.0");
    }
};

#endif
