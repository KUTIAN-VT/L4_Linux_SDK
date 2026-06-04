#ifndef __UART_ENDIAN__
#define __UART_ENDIAN__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdio.h>

static inline bool is_local_little_edian(void)
{
    uint32_t val = 0xaabbccdd;
    uint8_t *ptr = (uint8_t *)&val;

    if(ptr[0] == 0xdd)
        return true;
    else if(ptr[0] == 0xaa)
        return false;

    return false;
}

static inline uint16_t swap16(uint16_t data)
{
    return ((data<<8)&0xff00) | ((data>>8)&0xff);
}

static inline uint32_t swap32(uint32_t data)
{
    return ((data>>24)&0xff) | // move byte 3 to byte 0
        ((data<<8)&0xff0000) | // move byte 1 to byte 2
        ((data>>8)&0xff00) | // move byte 2 to byte 1
        ((data<<24)&0xff000000); // byte 0 to byte 3
}



#ifdef __cplusplus
}
#endif

#endif
