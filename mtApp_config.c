/*
 * mtApp.c
 *
 *  Created on: Aug 14, 2024
 *      Author: HelierDavidCamejoAra
 */

#include "mtApp_config.h"

extern uint8_t srspRpcBuff[RPC_MAX_LEN];
extern uint8_t srspRpcLen;

int8_t appCnfBDBsetChannel(bool isPrimary, uint8_t channel)
{
    uint32_t channel_array = 0;
    if (isPrimary)
    {
        channel_array = 1 << (channel+1);
    }

    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 5;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = isPrimary;
        cmd[cmInd++] = BREAK_UINT32(channel_array, 0);
        cmd[cmInd++] = BREAK_UINT32(channel_array, 1);
        cmd[cmInd++] = BREAK_UINT32(channel_array, 2);
        cmd[cmInd++] = BREAK_UINT32(channel_array, 3);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_APP_CONFIG), APP_CNF_BDB_SET_CHANNEL, cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(50);
        }

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return -1;
    }
}
int8_t appCnfBDBStartCommisioning(uint8_t commisioning_mode)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 1;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = commisioning_mode;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_APP_CONFIG), APP_CNF_BDB_START_COMMISSIONING, cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(50);
        }

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return -1;
    }
}
int8_t appCnfBDBSetTCRequireKeyExchange(uint8_t TC_keyexchange)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 1;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = TC_keyexchange;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_APP_CONFIG), APP_CNF_BDB_SET_TC_REQUIRE_KEY_EXCHANGE, cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(50);
        }

        free(cmd);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return -1;
    }
}
void processCommisioningNotificationSrsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    char mac[18];
    db_get_mac(mac);

    if (rpcLen < 2)
    {
        printf("MT_RPC_ERR_LENGTH\n");

    }
    return;
}
void appCfgProcess(uint8_t *rpcBuff, uint8_t rpcLen)
{
    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sysProcess: processing CMD0:%x, CMD1:%x\n", rpcBuff[0], rpcBuff[1]);

    //process the synchronous SRSP from SREQ
    if ((rpcBuff[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP)
    {
        //processSrsp(rpcBuff, rpcLen);
    }
    else
    {
        //Read CMD1 and processes the specific SREQ
        switch (rpcBuff[1])
        {
        case APP_CNF_BDB_COMMISSIONING_NOTIFICATION:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "appCfgProcess: APP_CNF_BDB_COMMISSIONING_NOTIFICATION\n");
            processCommisioningNotificationSrsp(&rpcBuff[2], rpcLen);
            break;
        default:
            //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "processRpcSys: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[0], rpcBuff[1]);
            break;
        }
    }
}
