/*
 * mtSapi.c
 *
 * This module contains the API for the MT ZDO Interface
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

#include "mtSapi.h"
#include "mtSys.h"
#include "mtParser.h"
#include "rpc.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
extern uint8_t srspRpcBuff[RPC_MAX_LEN];
extern uint8_t srspRpcLen;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void processStartCnf(uint8_t *rpcBuff, uint8_t rpcLen);
void processBindCnf(uint8_t *rpcBuff, uint8_t rpcLen);
void processAllowBindCnf(uint8_t *rpcBuff, uint8_t rpcLen);
void processSendDataCnf(uint8_t *rpcBuff, uint8_t rpcLen);
void processFindDeviceCnf(uint8_t *rpcBuff, uint8_t rpcLen);
void processReceiveDataInd(uint8_t *rpcBuff, uint8_t rpcLen);

/*********************************************************************
 * API FUNCTIONS
 */

uint8_t zbSystemReset(void)
{

	rpcSendFrame((MT_RPC_CMD_AREQ | MT_RPC_SYS_SAPI), MT_SAPI_SYS_RESET, NULL, 0);

	return SUCCESS;
}
uint8_t zbAppRegisterReq(AppRegisterReqFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 9 + (req->InputCommandsNum * 2)
	        + (req->OutputCommandsNum * 2);
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		int idx;

		cmd[cmInd++] = req->AppEndpoint;
		cmd[cmInd++] = (uint8_t)(req->AppProfileId & 0xFF);
		cmd[cmInd++] = (uint8_t)((req->AppProfileId >> 8) & 0xFF);
		cmd[cmInd++] = (uint8_t)(req->DeviceId & 0xFF);
		cmd[cmInd++] = (uint8_t)((req->DeviceId >> 8) & 0xFF);
		cmd[cmInd++] = req->DeviceVersion;
		cmd[cmInd++] = req->Unused;
		cmd[cmInd++] = req->InputCommandsNum;
		for (idx = 0; idx < req->InputCommandsNum; idx++)
		{
			cmd[cmInd++] = (uint8_t)(req->InputCommandsList[idx] & 0xFF);
			cmd[cmInd++] = (uint8_t)((req->InputCommandsList[idx] >> 8) & 0xFF);
		}
		cmd[cmInd++] = req->OutputCommandsNum;
		for (idx = 0; idx < req->OutputCommandsNum; idx++)
		{
			cmd[cmInd++] = (uint8_t)(req->OutputCommandsList[idx] & 0xFF);
			cmd[cmInd++] = (uint8_t)(
			        (req->OutputCommandsList[idx] >> 8) & 0xFF);
		}

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI), MT_SAPI_APP_REGISTER_REQ, cmd, cmdLen);

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
uint8_t zbStartReq()
{
	uint8_t status;

	status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI), MT_SAPI_START_REQ, NULL, 0);

	if (status == MT_RPC_SUCCESS)
	{
		rpcWaitMqClientMsg(50);
	}

	return status;
}
uint8_t zbPermitJoiningReq(PermitJoiningReqFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 3;
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		cmd[cmInd++] = (uint8_t)(req->Destination & 0xFF);
		cmd[cmInd++] = (uint8_t)((req->Destination >> 8) & 0xFF);
		cmd[cmInd++] = req->Timeout;

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI),MT_SAPI_PERMIT_JOINING_REQ, cmd, cmdLen);

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
uint8_t zbBindDevice(BindDeviceFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 11;
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		cmd[cmInd++] = req->Create;
		cmd[cmInd++] = (uint8_t)(req->CommandId & 0xFF);
		cmd[cmInd++] = (uint8_t)((req->CommandId >> 8) & 0xFF);
		memcpy((cmd + cmInd), req->DstIeee, 8);
		cmInd += 8;

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI), MT_SAPI_BIND_DEVICE, cmd, cmdLen);

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
uint8_t zbAllowBind(AllowBindFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 1;
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		cmd[cmInd++] = req->Timeout;

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI), MT_SAPI_ALLOW_BIND, cmd, cmdLen);

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
uint8_t zbSendDataReq(SendDataReqFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 8 + req->Len;
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		int idx;

		cmd[cmInd++] = (uint8_t)(req->Destination & 0xFF);
		cmd[cmInd++] = (uint8_t)((req->Destination >> 8) & 0xFF);
		cmd[cmInd++] = (uint8_t)(req->CommandId & 0xFF);
		cmd[cmInd++] = (uint8_t)((req->CommandId >> 8) & 0xFF);
		cmd[cmInd++] = req->Handle;
		cmd[cmInd++] = req->Ack;
		cmd[cmInd++] = req->Radius;
		cmd[cmInd++] = req->Len;
		for (idx = 0; idx < req->Len; idx++)
		{
			cmd[cmInd++] = req->Data[idx];
		}

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI),
		MT_SAPI_SEND_DATA_REQ, cmd, cmdLen);

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
uint8_t zbFindDeviceReq(FindDeviceReqFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 8;
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		memcpy((cmd + cmInd), req->SearchKey, 8);
		cmInd += 8;

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI), MT_SAPI_FIND_DEVICE_REQ, cmd, cmdLen);

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
uint8_t zbWriteConfiguration(WriteConfigurationFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 3 + req->Len;
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		int idx;

		cmd[cmInd++] = req->ConfigId;
		cmd[cmInd++] = req->Len;
		for (idx = 0; idx < req->Len; idx++)
		{
			cmd[cmInd++] = req->Value[idx];
		}

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI), MT_SAPI_WRITE_CONFIGURATION, cmd, cmdLen);

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
uint8_t zbGetDeviceInfo(GetDeviceInfoFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 1;
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		cmd[cmInd++] = req->Param;

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI), MT_SAPI_GET_DEVICE_INFO, cmd, cmdLen);

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
uint8_t zbReadConfiguration(ReadConfigurationFormat_t *req)
{
	uint8_t status;
	uint8_t cmInd = 0;
	uint32_t cmdLen = 1;
	uint8_t *cmd = malloc(cmdLen);

	if (cmd)
	{

		cmd[cmInd++] = req->ConfigId;

		status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SAPI), MT_SAPI_READ_CONFIGURATION, cmd, cmdLen);

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

void processReadConfigurationSrsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    ReadConfigurationSrspFormat_t rsp;
    if (rpcLen < 3)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.Status = rpcBuff[msgIdx++];
    rsp.ConfigId = rpcBuff[msgIdx++];
    rsp.Len = rpcBuff[msgIdx++];
    if (rpcLen > 3)
    {
        uint32_t i;
        for (i = 0; i < rsp.Len; i++)
        {
            rsp.Value[i] = rpcBuff[msgIdx++];
        }
    }
}
void processGetDeviceInfoSrsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    GetDeviceInfoSrspFormat_t rsp;
    if (rpcLen < 9)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.Param = rpcBuff[msgIdx++];
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        rsp.Value[i] = rpcBuff[msgIdx++];
    }
}
void processFindDeviceCnf(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    FindDeviceCnfFormat_t rsp;
    if (rpcLen < 11)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.SearchKey = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.Result = 0;
    uint8_t i;
    for (i = 0; i < 8; i++)
        rsp.Result |= ((uint64_t) rpcBuff[msgIdx++]) << (i * 8);
}
void processSendDataCnf(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    SendDataCnfFormat_t rsp;
    if (rpcLen < 2)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.Handle = rpcBuff[msgIdx++];
    rsp.Status = rpcBuff[msgIdx++];
}
void processReceiveDataInd(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    ReceiveDataIndFormat_t rsp;
    if (rpcLen < 6)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.Source = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.Command = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.Len = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    if (rpcLen > 6)
    {
        uint32_t i;
        for (i = 0; i < rsp.Len; i++)
        {
            rsp.Data[i] = rpcBuff[msgIdx++];
        }
    }
}
void processAllowBindCnf(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    AllowBindCnfFormat_t rsp;
    if (rpcLen < 2)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.Source = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
}
void processBindCnf(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    BindCnfFormat_t rsp;
    if (rpcLen < 3)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.CommandId = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.Status = rpcBuff[msgIdx++];
}
void processStartCnf(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    StartCnfFormat_t rsp;
    if (rpcLen < 1)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.Status = rpcBuff[msgIdx++];
}

void sapiProcess(uint8_t *rpcBuff, uint8_t rpcLen)
{
    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sapiProcess: processing CMD0:%x, CMD1:%x\n",rpcBuff[0], rpcBuff[1]);
	if ((rpcBuff[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_AREQ)
	{
		//Read CMD1 and processes the specific SREQ
		switch (rpcBuff[1])
		{
		case MT_SAPI_FIND_DEVICE_CNF:
		    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sapiProcess: MT_SAPI_FIND_DEVICE_CNF\n");
			processFindDeviceCnf(rpcBuff, rpcLen);
			break;
		case MT_SAPI_SEND_DATA_CNF:
		    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE,"sapiProcess: MT_SAPI_SEND_DATA_CNF\n");
			processSendDataCnf(rpcBuff, rpcLen);
			break;
		case MT_SAPI_RECEIVE_DATA_IND:
		    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sapiProcess: MT_SAPI_RECEIVE_DATA_IND\n");
			processReceiveDataInd(rpcBuff, rpcLen);
			break;
		case MT_SAPI_ALLOW_BIND_CNF:
		    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE,  "sapiProcess: MT_SAPI_ALLOW_BIND_CNF\n");
			processAllowBindCnf(rpcBuff, rpcLen);
			break;
		case MT_SAPI_BIND_CNF:
		    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sapiProcess: MT_SAPI_BIND_CNF\n");
			processBindCnf(rpcBuff, rpcLen);
			break;
		case MT_SAPI_START_CNF:
		    //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "sapiProcess: MT_SAPI_START_CNF\n");
			processStartCnf(rpcBuff, rpcLen);
			break;

		default:
		    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "sapiProcess: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[0],rpcBuff[1]);
			break;
		}
	}
}


