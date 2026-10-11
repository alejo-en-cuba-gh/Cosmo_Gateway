/*
 * mtZdo.c
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

#include "mtZdo.h"

/*********************************************************************
 * MACROS
 */
#define STARTDELAY 0

/*********************************************************************
 * LOCAL VARIABLES
 */
extern Mailbox_Handle m_add_device;
extern uint8_t srspRpcBuff[RPC_MAX_LEN];
extern uint8_t srspRpcLen;

uint8_t open_network = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
//Temporary task for state machine to add device
uint8_t remove_device(MgmtLeaveReqFormat_t *req)
{
    msg_rpc_t msg_send;
    msg_send.device_id = NULL;
    msg_send.id_length = 0;
    msg_send.item_type = NULL;
    msg_send.type_length = 0;
    msg_send.read_write = 0;

    msg_send.data_length = sizeof(MgmtLeaveReqFormat_t) + 2;
    msg_send.data = mem_alloc(msg_send.data_length);

    msg_send.data[0] = (MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO);
    msg_send.data[1] = MT_ZDO_MGMT_LEAVE_REQ;
    memcpy((void *)msg_send.data[2],req,sizeof(MgmtLeaveReqFormat_t));

    Mailbox_post(m_rpc_send, &msg_send, BIOS_NO_WAIT);
}

//REQ
uint8_t zdoMgmtPermitJoinReq(MgmtPermitJoinReqFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 5;
    uint8_t *cmd = mem_alloc(cmdLen);

    if(req->Duration != 0)
    {
        open_network = 1;
    }
    else
    {
        open_network = 0;
    }

    if (cmd)
    {

        cmd[cmInd++] = req->AddrMode;
        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);
        cmd[cmInd++] = req->Duration;
        cmd[cmInd++] = req->TCSignificance;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO), MT_ZDO_MGMT_PERMIT_JOIN_REQ, cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(1000);
            status = srspRpcBuff[2];
        }

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
uint8_t zdo_ieee_addr_req(IeeeAddrReqFormat_t *req)
{
    uint8_t status1;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 4;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->ShortAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->ShortAddr >> 8) & 0xFF);
        cmd[cmInd++] = req->ReqType;
        cmd[cmInd++] = req->StartIndex;

        status1 = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO), MT_ZDO_IEEE_ADDR_REQ, cmd, cmdLen);

        if (status1 == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(200);
        }

        mem_free(cmd, cmdLen);
        return status1;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
uint8_t zdo_node_desc_req(NodeDescReqFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 4;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);
        cmd[cmInd++] = (uint8_t) (req->NwkAddrOfInterest & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->NwkAddrOfInterest >> 8) & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO),
        MT_ZDO_NODE_DESC_REQ,
                              cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(200);
        }

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
uint8_t zdo_simple_desc_req(SimpleDescReqFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 5;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);
        cmd[cmInd++] = (uint8_t) (req->NwkAddrOfInterest & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->NwkAddrOfInterest >> 8) & 0xFF);
        cmd[cmInd++] = req->Endpoint;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO),
        MT_ZDO_SIMPLE_DESC_REQ,
                              cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(200);
        }

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
uint8_t zdo_active_ep_req(ActiveEpReqFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 4;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);
        cmd[cmInd++] = (uint8_t) (req->NwkAddrOfInterest & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->NwkAddrOfInterest >> 8) & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO),
        MT_ZDO_ACTIVE_EP_REQ,
                              cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(200);
        }

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
uint8_t zdo_bind_req(BindReqFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = sizeof(BindReqFormat_t);
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);

        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 0);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 1);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 2);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 3);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 4);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 5);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 6);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 7);

        cmd[cmInd++] = (uint8_t) (req->SrcEndpoint & 0xFF);

        cmd[cmInd++] = (uint8_t) (req->ClusterID & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->ClusterID >> 8) & 0xFF);

        cmd[cmInd++] = (uint8_t) (req->DstAddrMode & 0xFF);

        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 0);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 1);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 2);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 3);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 4);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 5);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 6);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 7);
        cmd[cmInd++] = (uint8_t) (req->DstEndpoint & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO), MT_ZDO_BIND_REQ, cmd, cmdLen);

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
        return 1;
    }
}
uint8_t zdo_unbind_req(UnbindReqFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = sizeof(UnbindReqFormat_t);
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);

        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 7);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 6);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 5);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 4);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 3);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 2);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 1);
        cmd[cmInd++] = BREAK_UINT64(req->SrcAddress, 0);

        cmd[cmInd++] = (uint8_t) (req->SrcEndpoint & 0xFF);

        cmd[cmInd++] = (uint8_t) (req->ClusterID & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->ClusterID >> 8) & 0xFF);

        cmd[cmInd++] = (uint8_t) (req->DstAddrMode & 0xFF);

        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 7);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 6);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 5);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 4);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 3);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 2);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 1);
        cmd[cmInd++] = BREAK_UINT64(req->DstAddress, 0);

        cmd[cmInd++] = (uint8_t) (req->DstEndpoint & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO), MT_ZDO_UNBIND_REQ, cmd, cmdLen);

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
        return 1;
    }
}
uint8_t zdo_mgmt_leave_req(MgmtLeaveReqFormat_t *req)
{

    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 11;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);

        cmd[cmInd++] = req->DeviceAddr[0];
        cmd[cmInd++] = req->DeviceAddr[1];
        cmd[cmInd++] = req->DeviceAddr[2];
        cmd[cmInd++] = req->DeviceAddr[3];
        cmd[cmInd++] = req->DeviceAddr[4];
        cmd[cmInd++] = req->DeviceAddr[5];
        cmd[cmInd++] = req->DeviceAddr[6];
        cmd[cmInd++] = req->DeviceAddr[7];

        cmd[cmInd++] = req->RemoveChildre_Rejoin;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO), MT_ZDO_MGMT_LEAVE_REQ, cmd, cmdLen);
        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(500);
        }

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
uint8_t zdo_ext_sec_aps_remove_req(ExtSecApsRemoveReqFormat_t *req)
{

    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 12;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = (uint8_t) (req->NWKAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->NWKAddr >> 8) & 0xFF);

        cmd[cmInd++] = req->IEEEaddr[0];
        cmd[cmInd++] = req->IEEEaddr[1];
        cmd[cmInd++] = req->IEEEaddr[2];
        cmd[cmInd++] = req->IEEEaddr[3];
        cmd[cmInd++] = req->IEEEaddr[4];
        cmd[cmInd++] = req->IEEEaddr[5];
        cmd[cmInd++] = req->IEEEaddr[6];
        cmd[cmInd++] = req->IEEEaddr[7];

        cmd[cmInd++] = (uint8_t) (req->parent_NWKAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->parent_NWKAddr >> 8) & 0xFF);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO), MT_ZDO_EXT_SEC_APS_REMOVE_REQ, cmd, cmdLen);
        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(500);
        }

        char lm[70];
        sprintf(lm, "zigbee_send - leave: device %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",req->IEEEaddr[7],req->IEEEaddr[6],req->IEEEaddr[5],req->IEEEaddr[4],req->IEEEaddr[3],req->IEEEaddr[2],req->IEEEaddr[1],req->IEEEaddr[0]);
        logevent(L_DEBUG, lm);

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
//RSP
void process_ieee_addr_rsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t i;
    uint8_t msgIdx = 2;
    msg_rpc_t msg_app;
    IeeeAddrRspFormat_t IeeeAddrRsp = { 0 };

    IeeeAddrRsp.Status = rpcBuff[msgIdx++];
    IeeeAddrRsp.IEEEAddr = BUILD_UINT64(rpcBuff[msgIdx], rpcBuff[msgIdx + 1],
                                        rpcBuff[msgIdx + 2],
                                        rpcBuff[msgIdx + 3], rpcBuff[msgIdx + 4],
                                        rpcBuff[msgIdx + 5],
                                        rpcBuff[msgIdx + 6], rpcBuff[msgIdx + 7]);
    msgIdx += 8;

    IeeeAddrRsp.NwkAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;

    IeeeAddrRsp.StartIndex = rpcBuff[msgIdx++];
    IeeeAddrRsp.NumAssocDev = rpcBuff[msgIdx++];

    for (i = 0; i < IeeeAddrRsp.NumAssocDev; i++, msgIdx += 2)
    {
        IeeeAddrRsp.AssocDevList[i] = BUILD_UINT16(rpcBuff[msgIdx],
                                                   rpcBuff[msgIdx + 1]);
    }

    msg_app.data_length = sizeof(IeeeAddrRspFormat_t);
    msg_app.data = mem_alloc(msg_app.data_length);

    msg_app.device_id = NULL;
    msg_app.item_type = NULL;
    msg_app.id_length = 0;
    msg_app.type_length = 0;
    memcpy((void*) msg_app.data, &IeeeAddrRsp, sizeof(IeeeAddrRspFormat_t));

    Mailbox_post(m_add_device, &msg_app, BIOS_NO_WAIT);
}
void process_node_desc_rsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    msg_rpc_t msg_app;
    uint8_t msgIdx = 2;

    NodeDescRspFormat_t NodeDescRsp = { 0 };

    NodeDescRsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    NodeDescRsp.Status = rpcBuff[msgIdx++];
    NodeDescRsp.NwkAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    NodeDescRsp.LoTy_ComDescAv_UsrDesAv = rpcBuff[msgIdx++];
    NodeDescRsp.APSFlg_FrqBnd = rpcBuff[msgIdx++];
    NodeDescRsp.MACCapFlg = rpcBuff[msgIdx++];
    NodeDescRsp.ManufacturerCode = rpcBuff[msgIdx++];
    NodeDescRsp.MaxBufferSize = rpcBuff[msgIdx++];
    NodeDescRsp.MaxTransferSize = BUILD_UINT16(rpcBuff[msgIdx],
                                               rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    NodeDescRsp.ServerMask = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    NodeDescRsp.MaxOutTransferSize = BUILD_UINT16(rpcBuff[msgIdx],
                                                  rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    NodeDescRsp.DescriptorCapabilities = rpcBuff[msgIdx++];

    msg_app.device_id = NULL;
    msg_app.item_type = NULL;
    msg_app.id_length = 0;
    msg_app.type_length = 0;

    msg_app.data_length = sizeof(NodeDescRspFormat_t);
    msg_app.data = mem_alloc(msg_app.data_length);
    memcpy(msg_app.data, &NodeDescRsp, msg_app.data_length);

    Mailbox_post(m_add_device, &msg_app, BIOS_NO_WAIT);
}
void process_active_endpoint_rsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    msg_rpc_t msg_app;
    uint8_t i;
    uint8_t msgIdx = 2;
    ActiveEpRspFormat_t ActiveEpRsp = { 0 };

    ActiveEpRsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    ActiveEpRsp.Status = rpcBuff[msgIdx++];
    ActiveEpRsp.NwkAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    ActiveEpRsp.ActiveEPCount = rpcBuff[msgIdx++];

    for (i = 0; i < ActiveEpRsp.ActiveEPCount; i++)
    {
        ActiveEpRsp.ActiveEPList[i] = rpcBuff[msgIdx++];
    }

    msg_app.device_id = NULL;
    msg_app.item_type = NULL;
    msg_app.id_length = 0;
    msg_app.type_length = 0;

    msg_app.data_length = sizeof(ActiveEpRspFormat_t);
    msg_app.data = mem_alloc(msg_app.data_length);
    memcpy(msg_app.data, &ActiveEpRsp, msg_app.data_length);

    Mailbox_post(m_add_device, &msg_app, BIOS_NO_WAIT);
}
void process_simple_desc_rsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t i;
    msg_rpc_t msg_app;
    uint8_t msgIdx = 2;

//Filling Simple Descriptor Struct
    SimpleDescRspFormat_t SimpleDescRsp = { 0 };

    SimpleDescRsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    SimpleDescRsp.Status = rpcBuff[msgIdx++];
    SimpleDescRsp.NwkAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    SimpleDescRsp.Len = rpcBuff[msgIdx++];
    SimpleDescRsp.Endpoint = rpcBuff[msgIdx++];
    SimpleDescRsp.ProfileID = BUILD_UINT16(rpcBuff[msgIdx],
                                           rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    SimpleDescRsp.DeviceID = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    SimpleDescRsp.DeviceVersion = rpcBuff[msgIdx++];
    SimpleDescRsp.NumInClusters = rpcBuff[msgIdx++];

    for (i = 0; i < SimpleDescRsp.NumInClusters; i++, msgIdx += 2)
    {
        SimpleDescRsp.InClusterList[i] = BUILD_UINT16(rpcBuff[msgIdx],
                                                      rpcBuff[msgIdx + 1]);
    }

    SimpleDescRsp.NumOutClusters = rpcBuff[msgIdx++];

    for (i = 0; i < SimpleDescRsp.NumOutClusters; i++, msgIdx += 2)
    {
        SimpleDescRsp.OutClusterList[i] = BUILD_UINT16(rpcBuff[msgIdx],
                                                       rpcBuff[msgIdx + 1]);
    }

    msg_app.device_id = NULL;
    msg_app.item_type = NULL;
    msg_app.id_length = 0;
    msg_app.type_length = 0;

    msg_app.data_length = sizeof(SimpleDescRspFormat_t);
    msg_app.data = mem_alloc(msg_app.data_length);
    memcpy(msg_app.data, &SimpleDescRsp, msg_app.data_length);

    Mailbox_post(m_add_device, &msg_app, BIOS_NO_WAIT);
}
void process_state_change(uint8_t *rpcBuff, uint8_t rpcLen)
{
    //uint8_t zdoState = rpcBuff[2];
    //uint8_t status;
}
void process_startup_from_app(uint8_t *rpcBuff, uint8_t rpcLen)
{
    //uint8_t msgIdx = 2;
    //StartUpFromAppSrspFormat_t rsp;

    //rsp.Status = rpcBuff[msgIdx++];
}
void processMgmtPermitJoinRsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    //uint8_t msgIdx = 2;
    //MgmtPermitJoinRspFormat_t rsp;
    //rsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    //msgIdx += 2;
    //rsp.Status = rpcBuff[msgIdx++];
}
void process_tc_dev_ind(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    TcDevIndFormat_t rsp;

    msg_rpc_t msg_app;

    if (rpcLen < 3)
    {
        printf("MT_RPC_ERR_LENGTH\n");

    }

    rsp.SrcNwkAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.IEEEAddr = BUILD_UINT64(rpcBuff[msgIdx], rpcBuff[msgIdx + 1],
                                rpcBuff[msgIdx + 2],
                                rpcBuff[msgIdx + 3],
                                rpcBuff[msgIdx + 4],
                                rpcBuff[msgIdx + 5],
                                rpcBuff[msgIdx + 6],
                                rpcBuff[msgIdx + 7]);
    msgIdx += 8;
    rsp.ParentNwkAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);

    msg_app.data_length = sizeof(TcDevIndFormat_t);
    msg_app.data = mem_alloc(msg_app.data_length);

    msg_app.device_id = NULL;
    msg_app.item_type = NULL;
    msg_app.id_length = 0;
    msg_app.type_length = 0;

    memcpy((void*) msg_app.data, &rsp, sizeof(TcDevIndFormat_t));

    Mailbox_post(m_add_device, &msg_app, BIOS_NO_WAIT);
}
void process_end_dev_annce_ind(uint8_t *rpcBuff, uint8_t rpcLen)
{
    dbt_zdevice *zdev;
    dbt_zendpoint *zenpoint;

    uint8_t msgIdx = 2;
    EndDeviceAnnceIndFormat_t rsp;

    rsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.NwkAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.IEEEAddr = BUILD_UINT64(rpcBuff[msgIdx], rpcBuff[msgIdx + 1],
                                rpcBuff[msgIdx + 2],
                                rpcBuff[msgIdx + 3], rpcBuff[msgIdx + 4],
                                rpcBuff[msgIdx + 5],
                                rpcBuff[msgIdx + 6], rpcBuff[msgIdx + 7]);
    msgIdx += 8;
    rsp.Capabilities = rpcBuff[msgIdx];

    zdev = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->ieee_address, false, ZDEVICE, NUMBER, 0, rsp.IEEEAddr, "", N_UINT64);

    if((open_network == 0) && zdev == NULL)
    {

    }

}
void process_bind_rsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    //uint8_t msgIdx = 2;
    //BindRspFormat_t rsp;

    //rsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    //msgIdx += 2;
    //rsp.Status = rpcBuff[msgIdx++];
}
void process_unbind_rsp(uint8_t *rpcBuff, uint8_t rpcLen)
{
    //uint8_t msgIdx = 2;
    //BindRspFormat_t rsp;

    //rsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    //msgIdx += 2;
    //rsp.Status = rpcBuff[msgIdx++];
}
void process_leave_ind(uint8_t *rpcBuff, uint8_t rpcLen)
{
    dbt_zdevice *zdev;
    dbt_zendpoint *zenpoint;
    msg_mqtt_t msg_mqtt;
    msg_rpc_t msg_app;

    msg_mqtt.buf = NULL;
    msg_mqtt.topic = NULL;
    msg_mqtt.content = NULL;
    msg_mqtt.size_buf = 0;
    msg_mqtt.size_content = 0;
    msg_mqtt.size_topic = 0;

    char ieee[24];


    char mac[18];
    db_get_mac(mac);

    uint8_t msgIdx = 2;
    LeaveIndFormat_t rsp;

    rsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.ExtAddr = BUILD_UINT64(rpcBuff[msgIdx], rpcBuff[msgIdx + 1],
                               rpcBuff[msgIdx + 2],
                               rpcBuff[msgIdx + 3],
                               rpcBuff[msgIdx + 4],
                               rpcBuff[msgIdx + 5],
                               rpcBuff[msgIdx + 6],
                               rpcBuff[msgIdx + 7]);
    msgIdx += 8;
    rsp.Request = rpcBuff[msgIdx++];
    rsp.Remove = rpcBuff[msgIdx++];
    rsp.Rejoin = rpcBuff[msgIdx++];

    sprintf(ieee, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            rpcBuff[11],
            rpcBuff[10],
            rpcBuff[9],
            rpcBuff[8],
            rpcBuff[7],
            rpcBuff[6],
            rpcBuff[5],
            rpcBuff[4]);

    char lm[70];
    sprintf(lm, "zigbee_recv - leave: device %s", ieee);
    logevent(L_DEBUG, lm);

    while(Mailbox_pend(m_add_device, &msg_app, 1000))
    {
        if (msg_app.data)
        {
            mem_free(msg_app.data, msg_app.data_length);
        }
        if (msg_app.device_id)
        {
            mem_free(msg_app.device_id, msg_app.id_length);
        }
        if(msg_app.item_type)
        {
            mem_free(msg_app.item_type, msg_app.type_length);
        }
    }

    zdev = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->ieee_address, false, ZDEVICE, NUMBER, 0, rsp.ExtAddr, "", N_UINT64);

    if (zdev)
    {
        zenpoint = (dbt_zendpoint*) find_element((dbt_generic**) &config_db.db_zendpoint, (char**) &config_db.db_zendpoint->zdevice_id, false, ZDEVICE, NUMBER, 0, zdev->id, "", N_UINT16);

        while (zenpoint)
        {
            db_delete_item((dbt_generic**) &zenpoint, (dbt_generic**) &config_db.db_zendpoint, (dbt_generic**) &config_db.db_zendpoint_last, ZENDPOINT);
            zenpoint = (dbt_zendpoint*) find_element((dbt_generic**) &config_db.db_zendpoint, (char**) &config_db.db_zendpoint->zdevice_id, false, ZDEVICE, NUMBER, 0, zdev->id, "", N_UINT16);
        }

        db_delete_item((dbt_generic**) &zdev, (dbt_generic**) &config_db.db_zdevice, (dbt_generic**) &config_db.db_zdevice_last, ZDEVICE);

    msg_mqtt.size_topic = 70;
    msg_mqtt.topic = mem_alloc(msg_mqtt.size_topic);
    sprintf(msg_mqtt.topic, "smaway/%s/toplatform/config/112", mac);

    msg_mqtt.size_content = 100;
    msg_mqtt.content = mem_alloc(msg_mqtt.size_content);
    sprintf(msg_mqtt.content, "{\"DeviceId\" : \"%s\", \"Status\": \"REMOVED\"}", ieee);

    msg_mqtt.size_buf = 0;
    msg_mqtt.buf = NULL;

    sendToMqttCheckingMailbox(&msg_mqtt);
    //Mailbox_post(m_mqtt_send, &msg_mqtt, BIOS_WAIT_FOREVER);
    }
}
void process_msg_cb_incoming(uint8_t *rpcBuff, uint8_t rpcLen)
{
    /*
    uint8_t msgIdx = 2;
    MsgCbIncomingFormat_t rsp;

    rsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.WasBroadcast = rpcBuff[msgIdx++];
    rsp.ClusterID = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.SecurityUse = rpcBuff[msgIdx++];
    rsp.SeqNum = rpcBuff[msgIdx++];
    rsp.MacDstAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.Status = rpcBuff[msgIdx++];
*/
}

uint8_t zdoInit(void)
{
    uint8_t status;
// build the bufferss
    uint32_t cmdLen = 2;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        cmd[0] = LO_UINT16(STARTDELAY);
        cmd[1] = HI_UINT16(STARTDELAY);

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO), MT_ZDO_STARTUP_FROM_APP, cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(1000);
        }

        mem_free(cmd, cmdLen);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        return 1;
    }
}
void zdoProcess(uint8_t *rpcBuff, uint8_t rpcLen)
{
//TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: processing CMD0:%x, CMD1:%x\n", rpcBuff[0], rpcBuff[1]);
    if ((rpcBuff[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_AREQ)
    {
        //Read CMD1 and processes the specific SREQ
        switch (rpcBuff[1])
        {
        case MT_ZDO_IEEE_ADDR_RSP:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_IEEE_ADDR_RSP\n");
            process_ieee_addr_rsp(rpcBuff, rpcLen);
            break;
        case MT_ZDO_NODE_DESC_RSP:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_NODE_DESC_RSP\n");
            process_node_desc_rsp(rpcBuff, rpcLen);
            break;
        case MT_ZDO_ACTIVE_EP_RSP:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_ACTIVE_EP_RSP\n");
            process_active_endpoint_rsp(rpcBuff, rpcLen);
            break;
        case MT_ZDO_SIMPLE_DESC_RSP:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_SIMPLE_DESC_RSP\n");
            process_simple_desc_rsp(rpcBuff, rpcLen);
            break;

        case MT_ZDO_BIND_RSP:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_BIND_RSP\n");
            process_bind_rsp(rpcBuff, rpcLen);
            break;
        case MT_ZDO_UNBIND_RSP:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_UNBIND_RSP\n");
            process_unbind_rsp(rpcBuff, rpcLen);
            break;

        case MT_ZDO_MGMT_PERMIT_JOIN_RSP:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_MGMT_PERMIT_JOIN_RSP\n");
            processMgmtPermitJoinRsp(rpcBuff, rpcLen);
            break;

        case MT_ZDO_STATE_CHANGE_IND:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_STATE_CHANGE_IND\n");
            process_state_change(rpcBuff, rpcLen);
            break;
        case MT_ZDO_END_DEVICE_ANNCE_IND:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_END_DEVICE_ANNCE_IND\n");
            process_end_dev_annce_ind(rpcBuff, rpcLen);
            break;
        case MT_ZDO_LEAVE_IND:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_LEAVE_IND\n");
            process_leave_ind(rpcBuff, rpcLen);
            break;
        case MT_ZDO_TC_DEV_IND:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_TC_DEV_IND\n");
            process_tc_dev_ind(rpcBuff, rpcLen);
            break;

        case MT_ZDO_MSG_CB_INCOMING:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "zdoProcess: MT_ZDO_MSG_CB_INCOMING\n");
            process_msg_cb_incoming(rpcBuff, rpcLen);
            break;

        default:
            //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "zdoProcess: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[0], rpcBuff[1]);
            break;
        }
    }
}

int16_t zdo_cmd_send(uint8_t cmd1, uint8_t *rpcFrame, uint8_t rpcLen)
{
    switch (cmd1)
    {
    case MT_ZDO_NWK_ADDR_REQ:
        break;
    case MT_ZDO_IEEE_ADDR_REQ:
        zdo_ieee_addr_req((IeeeAddrReqFormat_t*) rpcFrame);
        break;
    case MT_ZDO_NODE_DESC_REQ:
        zdo_node_desc_req((NodeDescReqFormat_t*) rpcFrame);
        break;
    case MT_ZDO_POWER_DESC_REQ:
        break;
    case MT_ZDO_SIMPLE_DESC_REQ:
        zdo_simple_desc_req((SimpleDescReqFormat_t*) rpcFrame);
        break;
    case MT_ZDO_ACTIVE_EP_REQ:
        zdo_active_ep_req((ActiveEpReqFormat_t*) rpcFrame);
        break;
    case MT_ZDO_MATCH_DESC_REQ:
        break;
    case MT_ZDO_COMPLEX_DESC_REQ:
        break;
    case MT_ZDO_USER_DESC_REQ:
        break;
    case MT_ZDO_DEVICE_ANNCE:
        break;
    case MT_ZDO_USER_DESC_SET:
        break;
    case MT_ZDO_SERVER_DISC_REQ:
        break;
    case MT_ZDO_END_DEVICE_BIND_REQ:
        break;
    case MT_ZDO_BIND_REQ:
        break;
    case MT_ZDO_UNBIND_REQ:
        break;
    case MT_ZDO_SET_LINK_KEY:
        break;
    case MT_ZDO_REMOVE_LINK_KEY:
        break;
    case MT_ZDO_GET_LINK_KEY:
        break;
    case MT_ZDO_NWK_DISCOVERY_REQ:
        break;
    case MT_ZDO_JOIN_REQ:
        break;
    case MT_ZDO_MGMT_NWK_DISC_REQ:
        break;
    case MT_ZDO_MGMT_LQI_REQ:
        break;
    case MT_ZDO_MGMT_RTG_REQ:
        break;
    case MT_ZDO_MGMT_BIND_REQ:
        break;
    case MT_ZDO_MGMT_LEAVE_REQ:
        zdo_mgmt_leave_req((MgmtPermitJoinReqFormat_t*) rpcFrame);
        break;
    case MT_ZDO_MGMT_DIRECT_JOIN_REQ:
        break;
    case MT_ZDO_MGMT_PERMIT_JOIN_REQ:
        zdoMgmtPermitJoinReq((MgmtPermitJoinReqFormat_t*) rpcFrame);
        break;
    case MT_ZDO_MGMT_NWK_UPDATE_REQ:
        break;
    case MT_ZDO_EXT_NWK_INFO:
        break;
    case MT_ZDO_EXT_SEC_APS_REMOVE_REQ:
        zdo_ext_sec_aps_remove_req((ExtSecApsRemoveReqFormat_t*) rpcFrame);
        break;
    }
    return 0;

}

