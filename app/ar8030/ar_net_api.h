#ifndef _AR_NET_API_H_
#define _AR_NET_API_H_

#include "bb_api.h"

/** 网络设备定义 */
#define AR_NET_MAX_NET_NAME_LEN             (16)  /**<@note 网络设备名最长长度*/
#define AR_NET_MAC_LEN                      (6)   /**<@note 网络mac地址长度*/

#define AR_NET_NETIF_T_PORT                 (0)   /**<@note 网络设备类型port*/
#define AR_NET_NETIF_T_VLAN                 (1)   /**<@note 网络设备类型vlan*/

/** 定义网络设备 ioctl */
#define AR_CMD_MAGIC 'A'

#define AR_CHARDEV_IOCTL_NET_GET_VERSION     _IO(AR_CMD_MAGIC, 0)    /** Get the version of net dev */
#define AR_CHARDEV_IOCTL_NET_HANDLE_NETIF    _IO(AR_CMD_MAGIC, 1)    /** Create/Destroy the net ddev */
#define AR_CHARDEV_IOCTL_NET_RELEASE         _IO(AR_CMD_MAGIC, 2)    /** Release the char dev resource request by user */

#define AR_CHARDEV_IOCTL_GET_VERSION         _IO(AR_CMD_MAGIC, 10)   /** Get the version of driver */
#define VERSION_STR_LEN                     (64)

/**网络设备创建数据结构(linux)*/
PACK(typedef struct ar_netif_s
{
    unsigned char op_type;                          /** <@note Operations, refers to AR_NET_OP_xxx */
    unsigned char netif_id;                         /** <@note (OUT)netif id (create or lookup) */
    unsigned char type;                             /** <@note Port/Vlan, refers to AR_NET_NETIF_T_xxx */
    unsigned char slot;                             /** <@note bb socket slot */
    unsigned short vlan;                            /** <@note vlan when type is AR_NET_NETIF_T_VLAN */
    unsigned short socket_port;                     /** <@note bb socket port */
    unsigned char mac[AR_NET_MAC_LEN];              /** <@note mac of the net device */
    unsigned char name[AR_NET_MAX_NET_NAME_LEN];    /** <@note name of the net device */
    unsigned long tx_buf_size;                      /** <@note Tx buffer of the socket */
    unsigned long rx_buf_size;                      /** <@note Rx buffer of the socket */
}) ar_netif_t;

/** 网络设备驱动板本号(linux) */
PACK(typedef struct _ar_chrdev_get_version_s {
    char version[VERSION_STR_LEN];
}) ar_chrdev_get_version_t;

/** 网络设备命令(linux) */
enum ar_net_operate_code_e
{
    AR_NET_OP_CREATE,                               /**<@note 创建设备*/
    AR_NET_OP_DESTORY,                              /**<@note 销毁设备*/
    AR_NET_OP_GET,                                  /**<@note 获取设备信息*/
    AR_NET_OP_BUF_RESIZE,                           /**<@note 调整设备socket buffer*/
    AR_NET_OP_MAX,
};

/** 网络设备接口(linux) */
AR8030_API int bb_net_dev_open(bb_dev_handle_t* dev);
AR8030_API int bb_net_dev_close(int net_dev_fd);
AR8030_API int bb_net_dev_create(int net_dev_fd, ar_netif_t *ar_netif);
AR8030_API int bb_net_dev_destroy(int net_dev_fd, unsigned char slot, unsigned short socket_port);
AR8030_API int bb_net_dev_buf_resize(int net_dev_fd, ar_netif_t *ar_netif);

#endif
