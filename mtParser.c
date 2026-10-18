/*
 * mtParser.c
 *
 * This module contains the API for the ZigBee SoC Host Interface.
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

#include "common.h"
#include "mtParser.h"
#include "rpc.h"

#include "mtSys.h"
#include "mtZdo.h"
#include "mtAf.h"
#include "mtSapi.h"
#include "mtApp_config.h"

uint8_t srspRpcBuff[RPC_MAX_LEN];
uint8_t srspRpcLen;

Semaphore_Handle sem_mtprocess;

void mtProcess(uint8_t *rpcBuff, uint8_t rpcLen)
{
    Semaphore_pend(sem_mtprocess,BIOS_WAIT_FOREVER);
    //Read CMD0
    switch (rpcBuff[0] & MT_RPC_SUBSYSTEM_MASK)
    {
    case MT_RPC_SYS_ZDO:
        //process ZDO RPC's in the ZDO module
        zdoProcess(rpcBuff, rpcLen);
        break;

    case MT_RPC_SYS_SYS:
        //process SYS RPC's in the Sys module
        sysProcess(rpcBuff, rpcLen);
        break;

    case MT_RPC_SYS_AF:
        //process SYS RPC's in the Sys module
        afProcess(rpcBuff, rpcLen);
        break;

    case MT_RPC_SYS_SAPI:
        //process SYS RPC's in the Sys module
        sapiProcess(rpcBuff, rpcLen);
        break;

    case MT_RPC_SYS_APP_CONFIG:
        //process SYS RPC's in the Sys module
        appCfgProcess(rpcBuff, rpcLen);
        break;

    default:
        //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "mtProcess: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[0], rpcBuff[1]);
        break;
    }

    Semaphore_post(sem_mtprocess);
}

