/*
 * rpc.c
 *
 * This module contains the RPC (Remote Procedure Call) API for the
 * ZigBee Network Processor (ZNP) Host Interface.
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
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
#include <ti/sysbios/knl/Clock.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include "rpc.h"
#include "rpcTransport.h"
#include "mtParser.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define SB_FORCE_BOOT              (0xF8)
#define SB_FORCE_RUN               (SB_FORCE_BOOT ^ 0xFF)

#define SRSP_TIMEOUT_MS            (5000) // 5000ms timeout
/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
Semaphore_Handle sem_rpc, sem_srsp;
extern Mailbox_Handle m_rpc_srsp, m_rpc_arsp;
extern Semaphore_Handle sem_systemprintf;
// expected SRSP command ID
static uint8_t expectedSrspCmdId;

static uint8_t calcFcs(uint8_t *msg, uint8_t len);

int32_t rpcOpen(char *_devicePath, uint32_t port)
{
    int fd;

    // open RPC transport
    fd = rpcTransportOpen(_devicePath, port);
    if (fd < 0)
    {
        //TODO LOG dbg_print(PRINT_LEVEL_ERROR, "rpcOpen: %s device open failed\n", _devicePath);
        return (-1);
    }
    //rpcForceRun();

    return fd;
}

int8_t rpcWaitMqClientMsg(uint32_t timeout_ms)
{
    msg_rpc_t msg;
    msg.device_id = NULL;
    msg.id_length = 0;
    msg.item_type = NULL;
    msg.type_length = 0;
    msg.read_write = 0;
    msg.data_length = 0;
    msg.data = NULL;

    char lm[70];

    timeout_ms = timeout_ms * 1000 / Clock_tickPeriod;

    Mailbox_pend(m_rpc_srsp, &msg, timeout_ms);

    if (msg.data)
    {
        mtProcess((uint8_t*) msg.data, msg.data_length);
        mem_free(msg.data, msg.data_length);
    }
    else
    {
        sprintf(lm, "zigbee - data request: rpcWaitMqClientMsg: Timed out\n");
        logevent(L_ERROR, lm);
        return -1;
    }

    return 0;
}

int32_t rpcProcess(void)
{
    uint8_t rpcLen, rpcTempLen, bytesRead, sofByte, rpcBuffIdx;
    uint8_t retryAttempts = 0, len, rpcBuff[RPC_MAX_LEN] = { 0 };
    uint8_t fcs;
    uint8_t i = 0;

    char lm[70];

    msg_rpc_t msg;

    msg.device_id = NULL;
    msg.id_length = 0;
    msg.item_type = NULL;
    msg.type_length = 0;
    msg.read_write = 0;
    msg.data_length = 0;
    msg.data = NULL;

    //read first byte and check it is a SOF
    bytesRead = rpcTransportRead(&sofByte, 1);

    if ((sofByte == MT_RPC_SOF) && (bytesRead == 1))
    {

        // clear retry counter
        retryAttempts = 0;

        // read length byte
        bytesRead = rpcTransportRead(&rpcLen, 1);

        if (bytesRead == 1)
        {
            len = rpcLen;
            rpcBuff[0] = rpcLen;

            //allocating RPC payload (+ cmd0, cmd1 and fcs)
            rpcLen += RPC_CMD0_FIELD_LEN + RPC_CMD1_FIELD_LEN + RPC_UART_FCS_LEN;

            //non blocking read, so we need to wait for the rpc to be read
            rpcBuffIdx = 1;
            rpcTempLen = rpcLen;
            while (rpcTempLen > 0)
            {
                // read RPC frame
                bytesRead = rpcTransportRead(&(rpcBuff[rpcBuffIdx]), rpcTempLen);
                if (bytesRead == 0)
                {
                    //TODO LOG
                }

                // check for error
                if (bytesRead > rpcTempLen)
                {
                    //there was an error
                    sprintf(lm, "zigbee - rpcProcess: read of %d bytes failed - %s\n", rpcTempLen, strerror(errno));
                    logevent(L_INFO, lm);

                    // check whether retry limits has been reached
                    if (retryAttempts++ < 5)
                    {
                        // sleep for 10ms
                        Task_sleep(10);

                        // try again
                        bytesRead = 0;
                    }
                    else
                    {
                        // something went wrong, abort
                        sprintf(lm, "zigbee - rpcProcess: transport read failed too many times\n");
                        logevent(L_ERROR, lm);
                        return -1;
                    }
                }

                // update counters
                if (rpcTempLen > bytesRead)
                {
                    rpcTempLen -= bytesRead;
                }
                else
                {
                    rpcTempLen = 0;
                }
                rpcBuffIdx += bytesRead;
            }

            //Verify FCS of incoming MT frames
            fcs = calcFcs(&rpcBuff[0], (len + 3));
            if (rpcBuff[len + 3] != fcs)
            {
                return -1;
            }

            if ((rpcBuff[1] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP)
            {
                // SRSP command ID deteced
                if (expectedSrspCmdId == (rpcBuff[1] & MT_RPC_SUBSYSTEM_MASK))
                {
                    //On rpc_send unblock waiting sreq
                    Semaphore_post(sem_srsp);

                    sprintf(lm, "zigbee : writing %d bytes SRSP to head of the queue\n", rpcLen);
                    logevent(L_INFO, lm);
                    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcProcess: writing %d bytes SRSP to head of the queue\n", rpcLen);

                    //TODO SRPS function

                    msg.data_length = rpcLen + 1;
                    msg.data = mem_alloc(msg.data_length);
                    memcpy(msg.data, (char*) &rpcBuff[1], rpcLen);

                    Mailbox_post(m_rpc_srsp, &msg, BIOS_WAIT_FOREVER);
                }
                else
                {
                    Semaphore_post(sem_srsp);
                    return 0;
                }
            }
            else if ((rpcBuff[1] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_AREQ)
            {
                // should be AREQ frame
                sprintf(lm, "zigbee - rpcProcess: writing %d bytes AREQ to tail of the que\n", rpcLen);
                logevent(L_INFO, lm);

                //TODO ARSP function
                msg.data_length = rpcLen + 1;
                msg.data = mem_alloc(msg.data_length);
                memcpy(msg.data, (char*) &rpcBuff[1], rpcLen);

                Mailbox_post(m_rpc_arsp, &msg, BIOS_NO_WAIT);
            }
            return 0;
        }
        else
        {
        }
    }
    else
    {
        sprintf(lm, "zigbee - rpcProcess: No valid Start Of Frame found [%x:%x]\n", sofByte, bytesRead);
        logevent(L_ERROR, lm);
    }

    return -1;
}

uint8_t rpcSendFrame(uint8_t cmd0, uint8_t cmd1, uint8_t *payload, uint8_t payload_len)
{
    uint8_t i;
    uint8_t buf[RPC_MAX_LEN];
    int32_t status = MT_RPC_SUCCESS;

    // block here if SREQ is in progress
    Semaphore_pend(sem_rpc, BIOS_WAIT_FOREVER);

    // fill in header bytes
    buf[0] = MT_RPC_SOF;
    buf[1] = payload_len;
    buf[2] = cmd0;
    buf[3] = cmd1;

    if ((cmd0 & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SREQ)
    {
        // calculate expected SRSP
        expectedSrspCmdId = (cmd0 & MT_RPC_SUBSYSTEM_MASK);
    }

    if (payload_len > 0)
    {
        // copy payload to buffer
        memcpy(buf + RPC_UART_HDR_LEN, payload, payload_len);
    }

    // calculate FCS field
    buf[payload_len + RPC_UART_HDR_LEN] = calcFcs(&buf[RPC_UART_FRAME_START_IDX], payload_len + RPC_HDR_LEN);
    // send out RPC  message
    rpcTransportWrite(buf, payload_len + RPC_UART_HDR_LEN + RPC_UART_FCS_LEN);

    // wait for SRSP if necessary
    if ((cmd0 & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SREQ)
    {
        // calculate timeout
        status = Semaphore_pend(sem_srsp, SRSP_TIMEOUT_MS);
        if (!status)
        {
            //TODO LOG dbg_print(PRINT_LEVEL_WARNING,"rpcSendFrame: SRSP Error - CMD0: 0x%02X CMD1: 0x%02X\n",cmd0, cmd1);
            status = MT_RPC_ERR_SUBSYSTEM;
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcSendFrame: Receive SRSP\n");
            status = MT_RPC_SUCCESS;
        }

        //set expected SRSP to invalid
        expectedSrspCmdId = 0xFF;
    }

    //Unlock RPC sem
    Semaphore_post(sem_rpc);

    return status;
}

static uint8_t calcFcs(uint8_t *msg, uint8_t size)
{
    uint8_t result = 0;

    // calculate FCS by XORing all bytes
    while (size--)
    {
        result ^= *msg++;
    }

    return result;
}
uint64_t ip_to_uint32(char *mac)
{
    uint64_t result = 0;
    uint8_t i = 0;
    int values[4];

    if (sscanf(mac, "%x.%x.%x.%x", &values[0], &values[1], &values[2], &values[3]) == 4)
    {
        result = BUILD_UINT32(values[3], values[2], values[1], values[0]);
    }
    else
    {
        printf("Invalid MAC address format\n");
    }

    return result;
}

uint64_t ieee_to_uint64(char *mac)
{
    uint64_t result = 0;
    uint8_t i = 0;
    int values[8];

    if (sscanf(mac, "%x:%x:%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5], &values[6], &values[7]) == 8)
    {
        result = BUILD_UINT64(values[7], values[6], values[5], values[4], values[3], values[2], values[1], values[0]);
    }
    else
    {
        printf("Invalid MAC address format\n");
    }

    return result;
}
