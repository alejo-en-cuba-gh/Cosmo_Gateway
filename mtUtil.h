/*
 * mtUtil.h
 *
 *  Created on: Aug 30, 2024
 *      Author: HelierDavidCamejoAra
 */

#ifndef FRAMEWORK_MT_UTIL_MTUTIL_H_
#define FRAMEWORK_MT_UTIL_MTUTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"
#include <stdint.h>

/***************************************************************************************************
 * UTIL COMMANDS
 ***************************************************************************************************/

/* SREQ/SRSP */
#define MT_UTIL_GET_DEVICE_INFO                 0x00
#define MT_UTIL_GET_NV_INFO                     0x01
#define MT_UTIL_SET_PANID                       0x02
#define MT_UTIL_SET_CHANNELS                    0x03
#define MT_UTIL_SET_SECLEVEL                    0x04
#define MT_UTIL_SET_PRECFGKEY                   0x05
#define MT_UTIL_CALLBACK_SUB_CMD                0x06
#define MT_UTIL_KEY_EVENT                       0x07
#define MT_UTIL_TIME_ALIVE                      0x09
#define MT_UTIL_LED_CONTROL                     0x0A
#define MT_UTIL_LOOPBACK                        0x10
#define MT_UTIL_DATA_REQ                        0x11
#define MT_UTIL_SRC_MATCH_ENABLE                0x20
#define MT_UTIL_SRC_MATCH_ADD_ENTRY             0x21
#define MT_UTIL_SRC_MATCH_DEL_ENTRY             0x22
#define MT_UTIL_SRC_MATCH_CHECK_SRC_ADDR        0x23
#define MT_UTIL_SRC_MATCH_ACK_ALL_PENDING       0x24
#define MT_UTIL_SRC_MATCH_CHECK_ALL_PENDING     0x25
#define MT_UTIL_ADDRMGR_EXT_ADDR_LOOKUP         0x40
#define MT_UTIL_ADDRMGR_NWK_ADDR_LOOKUP         0x41
#define MT_UTIL_APSME_LINK_KEY_DATA_GET         0x44
#define MT_UTIL_ASSOC_COUNT                     0x48
#define MT_UTIL_ASSOC_FIND_DEVICE               0x49
#define MT_UTIL_ASSOC_GET_WITH_ADDRESS          0x4A
#define MT_UTIL_APSME_REQUEST_KEY_CMD           0x4B
#define MT_UTIL_SRNG_GEN                        0x4C
#define MT_UTIL_BIND_ADD_ENTRY                  0x4D
#define MT_UTIL_ZCL_KEY_EST_INIT_EST            0x80
#define MT_UTIL_ZCL_KEY_EST_SIGN                0x81

/* AREQ optional, but no AREQ response. */
#define MT_UTIL_SYNC_REQ                        0xE0
#define MT_UTIL_ZCL_KEY_ESTABLISH_IND           0xE1

typedef struct
{
    uint8_t Status;
    uint64_t IEEEAddr;
    uint32_t ScanChannels;
    uint16_t PanId;
    uint8_t SecurityLevel;
    uint8_t PreConfigKey[16];
} GetNVInfoFormat_t;

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_MT_UTIL_MTUTIL_H_ */
