#ifndef ZBMTAPP_CONFIG_H
#define ZBMTAPP_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "mtParser.h"
#include "rpc.h"

enum
{
    _10_SEG = 0x00,
    _2_MIN = 0x01,
    _4_MIN = 0x02,
    _8_MIN = 0x03,
    _16_MIN = 0x04,
    _32_MIN = 0x05,
    _64_MIN = 0x06,
    _128_MIN = 0x07,
    _256_MIN = 0x08,
    _512_MIN = 0x09,
    _1028_MIN = 0x0A,
    _2048_MIN = 0x0B,
    _4096_MIN = 0x0C,
    _8192_MIN = 0x0D,
    _16384_MIN = 0x0E,
} timeout_index;

enum
{
    Initialization = 0x00,
    TouchLink = 0x01,
    Network_Steering = 0x02,
    Network_Formation = 0x04,
    Finding_Binding = 0x08
} Start_Commisioning;

enum
{
    channel_11 = 0x00000800,
    channel_12 = 0x00001000,
    channel_13 = 0x00002000,
    channel_14 = 0x00004000,
    channel_15 = 0x00008000,
    channel_16 = 0x00010000,
    channel_17 = 0x00020000,
    channel_18 = 0x00040000,
    channel_19 = 0x00080000,
    channel_20 = 0x00100000,
    channel_21 = 0x00200000,
    channel_22 = 0x00400000,
    channel_23 = 0x00800000,
    channel_24 = 0x01000000,
    channel_25 = 0x02000000,
    channel_26 = 0x04000000
} Channel;

/***************************************************************************************************
 * SYS COMMANDS
 ***************************************************************************************************/

/* AREQ from host */

/* SREQ/SRSP */
#define APP_CNF_SET_DEFAULT_REMOTE_ENDDEVICE_TIMEOUT    0x01
#define APP_CNF_SET_ENDDEVICETIMEOUT                    0x02
#define APP_CNF_SET_ALLOWREJOIN_TC_POLICY               0x03
#define APP_CNF_BDB_ADD_INSTALLCODE                     0x04
#define APP_CNF_BDB_START_COMMISSIONING                 0x05
#define APP_CNF_BDB_SET_JOINUSESINSTALLCODEKEY          0x06
#define APP_CNF_BDB_SET_ACTIVE_DEFAULT_CENTRALIZED_KEY  0x07
#define APP_CNF_BDB_SET_CHANNEL                         0x08
#define APP_CNF_BDB_SET_TC_REQUIRE_KEY_EXCHANGE         0x09
#define APP_CNF_BDB_ZED_ATTEMPT_RECOVER_NWK             0x0A
#define APP_CNF_SET_NWK_FRAME_COUNTER                   0xff

/* AREQ to host */
#define APP_CNF_BDB_COMMISSIONING_NOTIFICATION          0x80

typedef struct
{
    uint8_t status;
    uint8_t commisioning_mode;
    uint8_t remaining_commisioning_modes;
} CommisioningNotificationAreqFormat_t;

void appcfgProcess(uint8_t *rpcBuff, uint8_t rpcLen);
int8_t appCnfBDBsetChannel(bool isPrimary, uint8_t channel);
int8_t appCnfBDBStartCommisioning(uint8_t commisioning_mode);
int8_t appCnfBDBSetTCRequireKeyExchange(uint8_t TC_keyexchange);

#ifdef __cplusplus
}
#endif

#endif /* ZBMTSYS_H */
