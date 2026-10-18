/*
 * mtSys.c
 *
 * This module contains the API for the MT SYS Interface.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*********************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mtSys.h"
#include "mtParser.h"
#include "rpc.h"

/*********************************************************************
 * MACROS
 */
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

/*********************************************************************
 * LOCAL VARIABLE
 */
extern Mailbox_Handle m_mtSys;
extern uint8_t srspRpcBuff[RPC_MAX_LEN];
extern uint8_t srspRpcLen;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

uint8_t sysPing()
{
    uint8_t status;

    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
    MT_SYS_PING,
                          NULL, 0);

    if (status == MT_RPC_SUCCESS)
    {
        rpcWaitMqClientMsg(50);
    }

    return status;
}
uint8_t sysSetExtAddr(SetExtAddrFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 8;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        memcpy((cmd + cmInd), req->ExtAddr, 8);
        cmInd += 8;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_SET_EXTADDR,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysGetExtAddr()
{
    uint8_t status;

    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
    MT_SYS_GET_EXTADDR,
                          NULL, 0);

    if (status == MT_RPC_SUCCESS)
    {
        rpcWaitMqClientMsg(50);
    }

    return status;
}
uint8_t sysRamRead(RamReadFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 3;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->Address & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Address >> 8) & 0xFF);
        cmd[cmInd++] = req->Len;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_RAM_READ,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysRamWrite(RamWriteFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 4 + req->Len;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = (uint8_t) (req->Address & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Address >> 8) & 0xFF);
        cmd[cmInd++] = req->Len;
        for (idx = 0; idx < req->Len; idx++)
        {
            cmd[cmInd++] = req->Value[idx];
        }

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_RAM_WRITE,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysResetReq(ResetReqFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 1;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = req->Type;

        status = rpcSendFrame((MT_RPC_CMD_AREQ | MT_RPC_SYS_SYS), MT_SYS_RESET_REQ, cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysVersion()
{
    uint8_t status;

    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
    MT_SYS_VERSION,
                          NULL, 0);

    if (status == MT_RPC_SUCCESS)
    {
        rpcWaitMqClientMsg(50);
    }

    return status;
}
uint8_t sysOsalNvRead(OsalNvReadFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 3;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->Id & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Id >> 8) & 0xFF);
        cmd[cmInd++] = req->Offset;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS), MT_SYS_OSAL_NV_READ, cmd, cmdLen);

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
uint8_t sysOsalNvWrite(OsalNvWriteFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 4 + req->Len;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = (uint8_t) (req->Id & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Id >> 8) & 0xFF);
        cmd[cmInd++] = req->Offset;
        cmd[cmInd++] = req->Len;
        for (idx = 0; idx < req->Len; idx++)
        {
            cmd[cmInd++] = req->Value[idx];
        }

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS), MT_SYS_OSAL_NV_WRITE, cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            //rpcWaitMqClientMsg(200);
        }

        free(cmd);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
uint8_t sysOsalNvItemInit(OsalNvItemInitFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 5 + req->InitLen;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = (uint8_t) (req->Id & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Id >> 8) & 0xFF);
        cmd[cmInd++] = (uint8_t) (req->ItemLen & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->ItemLen >> 8) & 0xFF);
        cmd[cmInd++] = req->InitLen;
        for (idx = 0; idx < req->InitLen; idx++)
        {
            cmd[cmInd++] = req->InitData[idx];
        }

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS), MT_SYS_OSAL_NV_ITEM_INIT, cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysOsalNvDelete(OsalNvDeleteFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 4;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->Id & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Id >> 8) & 0xFF);
        cmd[cmInd++] = (uint8_t) (req->ItemLen & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->ItemLen >> 8) & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_OSAL_NV_DELETE,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysOsalNvLength(OsalNvLengthFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 2;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->Id & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Id >> 8) & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_OSAL_NV_LENGTH,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysOsalStartTimer(OsalStartTimerFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 3;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = req->Id;
        cmd[cmInd++] = (uint8_t) (req->Timeout & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Timeout >> 8) & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_OSAL_START_TIMER,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysOsalStopTimer(OsalStopTimerFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 1;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = req->Id;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_OSAL_STOP_TIMER,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysStackTune(StackTuneFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 2;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = req->Operation;
        cmd[cmInd++] = req->Value;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_STACK_TUNE,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysAdcRead(AdcReadFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 2;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = req->Channel;
        cmd[cmInd++] = req->Resolution;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_ADC_READ,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysGpio(GpioFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 2;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = req->Operation;
        cmd[cmInd++] = req->Value;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_GPIO,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysRandom()
{
    uint8_t status;

    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
    MT_SYS_RANDOM,
                          NULL, 0);

    if (status == MT_RPC_SUCCESS)
    {
        rpcWaitMqClientMsg(50);
    }

    return status;
}
uint8_t sysSetTime(SetTimeFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 11;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        memcpy((cmd + cmInd), req->UTCTime, 4);
        cmInd += 4;
        cmd[cmInd++] = req->Hour;
        cmd[cmInd++] = req->Minute;
        cmd[cmInd++] = req->Second;
        cmd[cmInd++] = req->Month;
        cmd[cmInd++] = req->Day;
        cmd[cmInd++] = (uint8_t) (req->Year & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Year >> 8) & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_SET_TIME,
                              cmd, cmdLen);

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
        return 1;
    }
}
uint8_t sysGetTime()
{
    uint8_t status;

    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
    MT_SYS_GET_TIME,
                          NULL, 0);

    if (status == MT_RPC_SUCCESS)
    {
        rpcWaitMqClientMsg(50);
    }

    return status;
}
uint8_t sysSetTxPower(SetTxPowerFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 1;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = req->TxPower;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS),
        MT_SYS_SET_TX_POWER,
                              cmd, cmdLen);

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
        return 1;
    }
}

void processResetInd(uint8_t *rpcBuff, uint8_t rpcLen)
{
    msg_mqtt_t msg;

    msg.buf = NULL;
    msg.size_buf = 0;
    msg.content = NULL;
    msg.size_content = 0;
    msg.topic = NULL;
    msg.size_topic = 0;

    uint8_t msgIdx = 2;
    ResetIndFormat_t rsp;
    if (rpcLen < 6)
    {
        printf("MT_RPC_ERR_LENGTH\n");

    }

    rsp.Reason = rpcBuff[msgIdx++];
    rsp.TransportRev = rpcBuff[msgIdx++];
    rsp.ProductId = rpcBuff[msgIdx++];
    rsp.MajorRel = rpcBuff[msgIdx++];
    rsp.MinorRel = rpcBuff[msgIdx++];
    rsp.HwRev = rpcBuff[msgIdx++];

    Mailbox_post(m_mtSys, &msg, BIOS_NO_WAIT);
}
void processOsalTimerExpired(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    OsalTimerExpiredFormat_t rsp;
    if (rpcLen < 1)
    {
        printf("MT_RPC_ERR_LENGTH\n");

    }
    rsp.Id = rpcBuff[msgIdx++];
}

void sysProcess(uint8_t *rpcBuff, uint8_t rpcLen)
{
    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sysProcess: processing CMD0:%x, CMD1:%x\n",rpcBuff[0], rpcBuff[1]);

    if ((rpcBuff[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_AREQ)
    {
        //Read CMD1 and processes the specific SREQ
        switch (rpcBuff[1])
        {
        case MT_SYS_RESET_IND:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sysProcess: MT_SYS_RESET_IND\n");
            processResetInd(rpcBuff, rpcLen);
            break;
        case MT_SYS_OSAL_TIMER_EXPIRED:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sysProcess: MT_SYS_OSAL_TIMER_EXPIRED\n");
            processOsalTimerExpired(rpcBuff, rpcLen);
            break;
        default:
            //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "processRpcSys: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[0], rpcBuff[1]);
            break;
        }
    }
}

