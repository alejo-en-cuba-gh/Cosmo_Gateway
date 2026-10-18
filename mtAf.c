/*
 * mtAf.c
 *
 * This module contains the API for the MT AF Interface.
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

#include "zap.h"
#include "mtAf.h"
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
extern uint8_t srspRpcBuff[RPC_MAX_LEN];
extern uint8_t srspRpcLen;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void model_id_parse(dbt_zdevice *zdevice, uint8_t *type, uint8_t data_type)
{
    char *value = mem_alloc(data_type);
    strncpy(value, (char*) type, data_type);
    value[data_type] = '\0';

    //1
    if ((strcmp(value, "902010/28") == 0) ||
    (strcmp(value, "AV2010/28") == 0) ||
    (strcmp(value, "PSB00003") == 0))
    {
        strcpy((char*) &zdevice->model_id, "PSB00003");
    }
    //2
    else if ((strcmp(value, "902010/18") == 0) ||
    (strcmp(value, "AV2010/18") == 0) ||
    (strcmp(value, "UPSB0001") == 0))
    {
        strcpy((char*) &zdevice->model_id, "UPSB0001");
    }
    //3
    else if ((strcmp(value, "902010/25") == 0) ||
    (strcmp(value, "AV2010/25") == 0) ||
    (strcmp(value, "PSMB0004") == 0))
    {
        strcpy((char*) &zdevice->model_id, "PSMB0004");
    }
    //4
    else if ((strcmp(value, "902010/26") == 0) ||
    (strcmp(value, "AV2010/26") == 0) ||
    (strcmp(value, "PDB00001") == 0))
    {
        strcpy((char*) &zdevice->model_id, "PDB00001");
    }
    //5
    else if ((strcmp(value, "902010/16") == 0) ||
    (strcmp(value, "AV2010/16") == 0) ||
    (strcmp(value, "UPDB0001") == 0))
    {
        strcpy((char*) &zdevice->model_id, "UPDB0001");
    }
    //6
    else if ((strcmp(value, "THERMB01") == 0) ||
    (strcmp(value, "AV2010/32") == 0) ||
    (strcmp(value, "902010/32") == 0))
    {
        strcpy((char*) &zdevice->model_id, "THERMB01");
    }
    //7
    else if (!strcmp(value, "HTHERMB1") ||
    !strcmp(value, "Thermostat"))
    {
        strcpy((char*) &zdevice->model_id, "HTHERMB1");
    }
    //8
    else if (strcmp(value, "HTHERMB2") == 0)
    {
        strcpy((char*) &zdevice->model_id, "HTHERMB2");
    }
    //9
    else if ((strcmp(value, "FKABB002") == 0) ||
    (strcmp(value, "AV2010/21A") == 0) ||
    (strcmp(value, "902010/21A") == 0))
    {
        strcpy((char*) &zdevice->model_id, "FKABB002");
    }
    //10
    else if ((strcmp(value, "FKABB003") == 0) ||
    (strcmp(value, "AV2010/21B") == 0) ||
    (strcmp(value, "902010/21B") == 0))
    {
        strcpy((char*) &zdevice->model_id, "FKABB003");
    }
    //11
    else if ((strcmp(value, "FKABB001") == 0) ||
    (strcmp(value, "AV2010/21C") == 0) ||
    (strcmp(value, "902010/21C") == 0))
    {
        strcpy((char*) &zdevice->model_id, "FKABB001");
    }
    //12
    else if ((strcmp(value, "BWMWSV01") == 0) ||
    (strcmp(value, "AV2010/14") == 0) ||
    (strcmp(value, "902010/14") == 0))
    {
        strcpy((char*) &zdevice->model_id, "BWMWSV01");
    }
    //13
    else if ((strcmp(value, "BWMWSB01") == 0) ||
    (strcmp(value, "AV2010/22") == 0) ||
    (strcmp(value, "902010/22") == 0))
    {
        strcpy((char*) &zdevice->model_id, "BWMWSB01");
    }
    //14
    else if ((strcmp(value, "BWMWSB02") == 0) ||
    (strcmp(value, "AV2010/22B") == 0) ||
    (strcmp(value, "902010/22B") == 0) ||
    (strcmp(value, "AV2010/22A") == 0) ||
    (strcmp(value, "902010/22A") == 0))
    {
        strcpy((char*) &zdevice->model_id, "BWMWSB02");
    }
    //15
    else if ((strcmp(value, "GLASB001") == 0) ||
    (strcmp(value, "AV2010/33") == 0) ||
    (strcmp(value, "902010/33") == 0))
    {
        strcpy((char*) &zdevice->model_id, "GLASB001");
    }
    //16
    else if ((strcmp(value, "WMWSB001") == 0) ||
    (strcmp(value, "AV2010/37") == 0) ||
    (strcmp(value, "902010/37") == 0))
    {
        strcpy((char*) &zdevice->model_id, "WMWSB001");
    }
    //17
    else if ((strcmp(value, "RWMSIB01") == 0) ||
    (strcmp(value, "AV2010/24A") == 0) ||
    (strcmp(value, "902010/24A") == 0) ||
    (strcmp(value, "902010/24") == 0) ||
    (strcmp(value, "AV2010/24") == 0))
    {
        strcpy((char*) &zdevice->model_id, "RWMSIB01");
    }
    //18
    else if ((strcmp(value, "SIRENB01") == 0) ||
    (strcmp(value, "AV2010/29A") == 0) ||
    (strcmp(value, "902010/29A") == 0) ||
    (strcmp(value, "AV2010/29") == 0) ||
    (strcmp(value, "902010/29") == 0))
    {
        strcpy((char*) &zdevice->model_id, "SIRENB01");
    }
    //19
    else if ((strcmp(value, "UPSPFMB1") == 0) ||
    (strcmp(value, "UPSPFMB1") == 0) ||
    (strcmp(value, "ZR-EKKO-1DRELAY-HA") == 0))
    {
        strcpy((char*) &zdevice->model_id, "UPSPFMB1");
    }
    //20
    else if ((strcmp(value, "UPSMB001") == 0) ||
    (strcmp(value, "ZR-EKKO-1RELAY-HA") == 0))
    {
        strcpy((char*) &zdevice->model_id, "UPSMB001");
    }
    //21
    else if ((strcmp(value, "UPSDMB01") == 0) ||
    (strcmp(value, "ZR-EKKO-DIMMER-HA") == 0))
    {
        strcpy((char*) &zdevice->model_id, "UPSDMB01");
    }
    //22
    else if ((strcmp(value, "LUXSENB1") == 0) ||
    (strcmp(value, "ZED-THL2D-HA") == 0) ||
    (strcmp(value, "ZED-TL-HA") == 0))
    {
        strcpy((char*) &zdevice->model_id, "LUXSENB1");
    }
    //23
    else if ((strcmp(value, "UPRSB001") == 0) ||
    (strcmp(value, "ZR-EKKO-SHUTTER-HA") == 0))
    {
        strcpy((char*) &zdevice->model_id, "UPRSB001");
    }
    //24
    else if ((strcmp(value, "UPS2MB01") == 0) ||
    (strcmp(value, "ZR-EKKO-2RELAYS-HA") == 0))
    {
        strcpy((char*) &zdevice->model_id, "UPS2MB01");
    }
    //25        FSREPB01
    else if (strcmp(value, "FSREPB01") == 0)
    {
        strcpy((char*) &zdevice->model_id, "FSREPB01");
    }
    //26        HYDRONIC
    else if (strcmp(value, "1211/01") == 0)
    {
        strcpy((char*) &zdevice->model_id, "1211/01");
    }
    else
    {
        strcpy((char*) &zdevice->model_id, value);
    }
}

void cluster_response(IncomingMsgFormat_t *rsp)
{
    msg_mqtt_t msg;

    msg.buf = NULL;
    msg.size_buf = 0;
    msg.content = NULL;
    msg.size_content = 0;
    msg.topic = NULL;
    msg.size_topic = 0;

    char mac[18];
    db_get_mac(mac);

    char ieee_address[24];

    msg_rpc_t msg_rpc;

    msg_rpc.data = NULL;
    msg_rpc.data_length = 0;
    msg_rpc.device_id = NULL;
    msg_rpc.id_length = 0;
    msg_rpc.item_type = NULL;
    msg_rpc.type_length = 0;
    msg_rpc.read_write = 0;

    dbt_zdevice *zdevice = NULL;

    uint8_t msgIdx = 0;
    uint16_t i = 0;

    uint8_t frame_control = 0;
    uint8_t transaction_secuensce = 0;
    uint8_t command_id = 0;
    uint8_t command_payload = 0;
    uint8_t status_code = 0;
    uint8_t data_type = 0;
    uint8_t *value;
    uint16_t attribute_id = 0;
    float temperature = 0;

    uint32_t value1;
    uint16_t attribute_id2;
    uint8_t data_type2;
    uint64_t value2;
    uint16_t attribute_id3;
    uint8_t data_type3;
    uint64_t value3;

    bool add_zdevice = false;

    char lm[70];

    zdevice = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->nwk_address, false, ZDEVICE, NUMBER, 0, rsp->SrcAddr, "", N_UINT16);

    sprintf(ieee_address, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                                BREAK_UINT64(zdevice->ieee_address, 7),
                                BREAK_UINT64(zdevice->ieee_address, 6),
                                BREAK_UINT64(zdevice->ieee_address, 5),
                                BREAK_UINT64(zdevice->ieee_address, 4),
                                BREAK_UINT64(zdevice->ieee_address, 3),
                                BREAK_UINT64(zdevice->ieee_address, 2),
                                BREAK_UINT64(zdevice->ieee_address, 1),
                                BREAK_UINT64(zdevice->ieee_address, 0));

    if(zdevice)
    {
        if((zdevice->status == 0) && strlen(zdevice->model_id) != 0)
        {
            zdevice->status = 1;
            send_112(ieee_address, 1);
        }
    }
    else
    {
        if(zdevice->status == 1 && strlen(zdevice->model_id) != 0)
        {
            zdevice->status = 0;
            send_112(ieee_address, 0);
        }
    }



    switch (rsp->ClusterId)
    {
    case BASIC:

        // Extract Zigbee ZCL frame components
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];

        printf("[DEBUG] BASIC: frame_control=0x%02X, transaction_seq=0x%02X, command_id=0x%02X\n",
                frame_control, transaction_secuensce, command_id);

        switch (command_id)
        {
        case ReadAttributesResponse:
            printf("[DEBUG] Processing ReadAttributesResponse...\n");

            // Iterate through received attributes
            while (rsp->Len > msgIdx)
            {
                attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                msgIdx += 2;

                printf("[DEBUG] Attribute ID: 0x%04X\n", attribute_id);

                switch (attribute_id)
                {
                case Firmware_Version:
                    // Parse firmware version attribute
                    status_code = rsp->Data[msgIdx++];
                    data_type = rsp->Data[msgIdx++];
                    zdevice->firmware_version = rsp->Data[msgIdx++];
                    add_zdevice = true;

                    printf("[DEBUG] Firmware Version: status=0x%02X, data_type=0x%02X, version=%d\n",
                            status_code, data_type, zdevice->firmware_version);
                    break;

                case Model_Id:
                    // Parse model ID string
                    status_code = rsp->Data[msgIdx++];
                    data_type = get_frame_payload(rsp->Data[msgIdx], &rsp->Data[msgIdx + 1]);
                    msgIdx += 2;
                    value = mem_alloc(data_type);

                    for (i = 0; i < data_type; i++)
                    {
                        value[i] = rsp->Data[msgIdx++];
                    }

                    printf("[DEBUG] Received Model ID (%d bytes): ", data_type);
                    for (i = 0; i < data_type; i++) printf("%02X ", value[i]);
                    printf("\n");

                    model_id_parse(zdevice, value, data_type);

                    msg_rpc.data_length = 10;
                    msg_rpc.data = mem_alloc(msg_rpc.data_length);
                    memcpy(msg_rpc.data, zdevice->model_id, msg_rpc.data_length);
                    add_zdevice = true;

                    printf("[DEBUG] Parsed Model ID: %s\n", zdevice->model_id);
                    break;

                case Manufacterer_Name:
                    // Read and ignore manufacturer name
                    status_code = rsp->Data[msgIdx++];
                    data_type = get_frame_payload(rsp->Data[msgIdx], &rsp->Data[msgIdx + 1]);
                    msgIdx += 2;
                    value = mem_alloc(data_type);

                    for (i = 0; i < data_type; i++)
                    {
                        value[i] = rsp->Data[msgIdx++];
                    }

                    printf("[DEBUG] Manufacturer Name received (%d bytes). Ignored after reading.\n", data_type);
                    mem_free(value, data_type);
                    add_zdevice = true;
                    break;

                case Power_Source:
                    // Parse and publish power source
                    status_code = rsp->Data[msgIdx++];
                    data_type = rsp->Data[msgIdx++];
                    value1 = rsp->Data[msgIdx++];

                    printf("[DEBUG] Power Source: status=0x%02X, data_type=0x%02X, value=0x%02X\n",
                            status_code, data_type, value1);

                    msg.size_content = 50;
                    msg.content = mem_alloc(msg.size_content);
                    switch (value1)
                    {
                    case 0x00:
                        sprintf(msg.content, "%s", "UNKNOWN");
                        break;
                    case 0x01:
                        sprintf(msg.content, "%s", "SINGLE-FASE");
                        break;
                    case 0x02:
                        sprintf(msg.content, "%s", "THREE-FASE");
                        break;
                    case 0x03:
                        sprintf(msg.content, "%s", "BATTERY");
                        break;
                    case 0x04:
                        sprintf(msg.content, "%s", "DC-SOURCE");
                        break;
                    case 0x05:
                        sprintf(msg.content, "%s", "EMERGENCY MAINS CONSTANTLY POWERED");
                        break;
                    case 0x06:
                        sprintf(msg.content, "%s", "EMERGENCY MAINS AND TRANSFER SWITCH");
                        break;
                    default:
                        sprintf(msg.content, "%s", "UNKNOWN SOURCE");
                    }

                    msg.size_topic = 100;
                    msg.topic = mem_alloc(msg.size_topic);
                    sprintf(msg.topic, "smaway/%s/toplatform/item/%s/power-source", mac, ieee_address);

                    printf("[DEBUG] Sending Power Source to MQTT: topic=%s, content=%s\n", msg.topic, msg.content);

                    sendToMqttCheckingMailboxNoWait(&msg);
                    //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                    break;
                }
            }

            // If any valid attribute was received, send message to add_zdevice
            if (add_zdevice)
            {
                printf("[DEBUG] Basic information received. Posting to add_zdevice.\n");
                Mailbox_post(m_add_device, &msg_rpc, BIOS_NO_WAIT);
            }
            else
            {
                printf("[DEBUG] No valid model or firmware information received.\n");
            }
            break;

        case WriteAttributesResponse:
            break;
        }
        break;

    case POWER_CONFIGURATION:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            switch (attribute_id)
            {
            case Battery_Alarm_State:
                status_code = rsp->Data[msgIdx++];
                data_type = get_frame_payload(rsp->Data[msgIdx++], NULL);
                value = mem_alloc(data_type);
                *value = rsp->Data[msgIdx++];

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%s", ((((uint32_t) *value & 0x0001) == 1) ? "true" : "false"));

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/battery-low\0", mac, ieee_address);

                mem_free(value, data_type);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case Battery_Percent:
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];
                value1 = rsp->Data[msgIdx];
                value1 *= 100;
                value1 /= 200;
                msgIdx++;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%d", value1);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/battery-percent\0", mac, ieee_address);

                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;
            }
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            switch (attribute_id)
            {
            case Battery_Alarm_State:
                data_type = get_frame_payload(rsp->Data[msgIdx++], NULL);
                value = mem_alloc(data_type);
                *value = rsp->Data[msgIdx++];

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%s", ((((uint32_t) *value & 0x0001) == 1) ? "true" : "false"));

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/battery-low\0", mac, ieee_address);

                mem_free(value, data_type);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;
            case Battery_Percent:
                data_type = rsp->Data[msgIdx++];
                value1 = rsp->Data[msgIdx];
                value1 *= 100;
                value1 /= 200;
                msgIdx++;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%d", value1);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/battery-percent\0", mac, ieee_address);

                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);

                break;
            }
            break;
        case DefaultResponse:
            break;
        }
        break;

    case ON_OFF:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];

        switch (command_id)
        {
        case ReadAttributesResponse:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            switch (attribute_id)
            {
            case On_Off:
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];
                value = mem_alloc(data_type);
                *value = rsp->Data[msgIdx++];

                msg.size_content = 5;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%s", (((int) *value == 1) ? "ON" : "OFF"));

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                if(strcmp(zdevice->model_id, "1211/01") == 0)
                {
                    sprintf(msg.topic, "smaway/%s/toplatform/item/%s-%d/onoff\0", mac, ieee_address, rsp->SrcEndpoint);
                }
                else
                {
                    sprintf(msg.topic, "smaway/%s/toplatform/item/%s/onoff\0", mac, ieee_address);
                }

                mem_free(value, data_type);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);

                break;
            }
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
                    attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                    msgIdx += 2;
                    switch(attribute_id)
                    {
                    case On_Off:
                        data_type = rsp->Data[msgIdx++];
                        value = mem_alloc(data_type);
                        *value = rsp->Data[msgIdx++];

                        msg.size_content = 5;
                        msg.content = mem_alloc(msg.size_content);
                        sprintf(msg.content, "%s", ((*value == 1) ? "ON" : "OFF"));

                        msg.size_topic = 100;
                        msg.topic = mem_alloc(msg.size_topic);
                        if(strcmp(zdevice->model_id, "1211/01") == 0)
                        {
                            sprintf(msg.topic, "smaway/%s/toplatform/item/%s-%d/onoff\0", mac, ieee_address, rsp->SrcEndpoint);
                        }
                        else
                        {
                            sprintf(msg.topic, "smaway/%s/toplatform/item/%s/onoff\0", mac, ieee_address);
                        }

                        mem_free(value, data_type);
                        sendToMqttCheckingMailboxNoWait(&msg);
                        //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                        break;
                    }
                    break;

        case DefaultResponse:
            command_payload = rsp->Data[msgIdx++];
            status_code = rsp->Data[msgIdx++];
            break;
        }
        break;

    case BINARY_INPUT_BASIC:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];

        switch (command_id)
        {
        case ReadAttributesResponse:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;

            if (attribute_id == PresentValue) {
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];

                value = mem_alloc(sizeof(uint8_t));
                *value = rsp->Data[msgIdx++];

                msg.size_content = 4;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%s", ((*value == 1) ? "ON" : "OFF"));

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                if (strcmp(zdevice->model_id, "1211/01") == 0) {
                    sprintf(msg.topic, "smaway/%s/toplatform/item/%s-%d/onoff\0", mac, ieee_address, (rsp->SrcEndpoint + 11));
                } else {
                    sprintf(msg.topic, "smaway/%s/toplatform/item/%s/binaryinput", mac, ieee_address);
                }

                mem_free(value, sizeof(uint8_t));
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
            }
            break;

        case ReportAttributes:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;

            if (attribute_id == PresentValue) {
                data_type = rsp->Data[msgIdx++];

                value = mem_alloc(sizeof(uint8_t));
                *value = rsp->Data[msgIdx++];

                msg.size_content = 4;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%s", ((*value == 1) ? "ON" : "OFF"));

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                if (strcmp(zdevice->model_id, "1211/01") == 0) {
                    sprintf(msg.topic, "smaway/%s/toplatform/item/%s-%d/onoff\0", mac, ieee_address, (rsp->SrcEndpoint + 11));
                } else {
                    sprintf(msg.topic, "smaway/%s/toplatform/item/%s/binaryinput", mac, ieee_address);
                }

                mem_free(value, sizeof(uint8_t));
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
            }
            break;

        case DefaultResponse:
            command_payload = rsp->Data[msgIdx++];
            status_code = rsp->Data[msgIdx++];

            if (status_code != SUCCESS) {
                printf("Error: DefaultResponse devuelve estado %d\n", status_code);
            }
            break;
        }
        break;


    case LEVEL_CONTROL:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];

        switch (command_id)
        {
        case ReportAttributes:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            data_type = rsp->Data[msgIdx++];
            value = mem_alloc(data_type);
            *value = rsp->Data[msgIdx++];

            msg.size_content = 5;
            msg.content = mem_alloc(msg.size_content);
            sprintf(msg.content, "%d", *value);

            msg.size_topic = 100;
            msg.topic = mem_alloc(msg.size_topic);
            sprintf(msg.topic, "smaway/%s/toplatform/item/%s/level-control\0", mac, ieee_address);

            mem_free(value, data_type);
            sendToMqttCheckingMailboxNoWait(&msg);
            //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);

            break;
        }
        break;

    case THERMOSTAT:

        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            switch (attribute_id)
            {
            case Local_Temperature:

                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];

                attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                temperature = attribute_id2;
                temperature /= 100;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%.02f", temperature);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/local-temperature\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case Outdoor_Temperature:
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];

                attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                temperature = attribute_id2;
                temperature /= 100;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%.02f", temperature);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/outdoor-temperature\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case Target_Temperature:
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];

                attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                temperature = attribute_id2;
                temperature /= 100;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%.02f", temperature);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/target-temperature\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case System_Mode:
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%s", (rsp->Data[msgIdx] == 0x00) ? "OFF" : (rsp->Data[msgIdx] == 0x01) ? "AUTO" : (rsp->Data[msgIdx] == 0x03) ? "COOL" : "HEAT");
                msgIdx++;

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/system-mode\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case Occupied_Heating_Set:
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];

                attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                temperature = attribute_id2;
                temperature /= 100;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%.02f", temperature);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/target-temperature\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case Control_Sequence_Operation:
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%d", rsp->Data[msgIdx++]);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/control-sequence-operation\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            }
            break;
        case WriteAttributesResponse:
            #ifdef _DEBUG_
            printf("processIncomingMsgExt: TransSeqNum=%d, DataLen=%d\n", transaction_secuensce, rsp->Len);
            #endif
            afPostResponseSemaphore();
            break;

        case ReportAttributes:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            switch (attribute_id)
            {
            case Local_Temperature:
                data_type = rsp->Data[msgIdx++];

                attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                temperature = attribute_id2;
                temperature /= 100;
                msgIdx += 2;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%.02f", temperature);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/local-temperature\0", mac, ieee_address);

                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);

                break;

            case Outdoor_Temperature:
                data_type = rsp->Data[msgIdx++];

                attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                temperature = attribute_id2;
                temperature /= 100;
                msgIdx += 2;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%.02f", temperature);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/outdoor-temperature\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case Target_Temperature:
                data_type = rsp->Data[msgIdx++];

                attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                temperature = attribute_id2;
                temperature /= 100;
                msgIdx += 2;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%.02f", temperature);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/target-temperature\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case System_Mode:
                data_type = rsp->Data[msgIdx++];

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%s", (rsp->Data[msgIdx] == 0x0000) ? "OFF" : (rsp->Data[msgIdx] == 0x0001) ? "AUTO" : (rsp->Data[msgIdx] == 0x0003) ? "COOL" : "HEAT");
                msgIdx++;

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/system-mode\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;

            case Occupied_Heating_Set:
                data_type = rsp->Data[msgIdx++];

                attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                temperature = attribute_id2;
                temperature /= 100;
                msgIdx += 2;

                msg.size_content = 10;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%.02f", temperature);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/target-temperature\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;
            }
            break;
        case ConfigureReportingResponse:
            #ifdef _DEBUG_
            printf("processIncomingMsgExt: TransSeqNum=%d, DataLen=%d\n", transaction_secuensce, rsp->Len);
            #endif
            afPostResponseSemaphore();
            break;

        case DefaultResponse:
            break;
        }
        //Mailbox_post(m_lappsend, &msg_app, BIOS_WAIT_FOREVER);
        break;

    case DIAGNOSTIC:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            switch (attribute_id)
            {
            case Last_LQI:
                status_code = rsp->Data[msgIdx++];
                data_type = rsp->Data[msgIdx++];
                value1 = rsp->Data[msgIdx++];
                value1 = (value1 * 100) / 255;
                msg.size_content = 10;

                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "%d", value1);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/item/%s/lqi-value\0", mac, ieee_address);
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
                break;
            }
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            switch (attribute_id)
            {
            }
            break;
        case DefaultResponse:
            break;
        }
        break;

    case IAS_ZONE:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            break;
        case DefaultResponse:
            break;
        }
        break;

    case IAS_WD:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            break;
        case DefaultResponse:
            break;
        }
        break;

    case METERING:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];

        switch (command_id)
        {
        case ReadAttributesResponse:
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            data_type = rsp->Data[msgIdx++];
            value1 = BUILD_UINT32(rsp->Data[msgIdx], rsp->Data[msgIdx + 1], rsp->Data[msgIdx + 2], 0);
            msgIdx += 3;

            attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            data_type2 = rsp->Data[msgIdx++];
            value2 = BUILD_UINT64(rsp->Data[msgIdx], rsp->Data[msgIdx + 1], rsp->Data[msgIdx + 2], rsp->Data[msgIdx + 3], rsp->Data[msgIdx + 4], rsp->Data[msgIdx + 5], 0, 0);
            msgIdx += 6;

            attribute_id3 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
            msgIdx += 2;
            data_type3 = rsp->Data[msgIdx++];
            value3 = BUILD_UINT64(rsp->Data[msgIdx], rsp->Data[msgIdx + 1], rsp->Data[msgIdx + 2], rsp->Data[msgIdx + 3], rsp->Data[msgIdx + 4], rsp->Data[msgIdx + 5], 0, 0);
            msgIdx += 6;

            break;
        case DefaultResponse:
            break;
        }
        break;

    case ASTREL_COMMON_FCC4:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            break;
        case DefaultResponse:
            break;
        }
        break;

    case ILLUMINANCE_MEASUREMENT:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:

            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            break;
        case DefaultResponse:
            break;
        }
        break;

    case WINDOW_COVERING:
        frame_control = rsp->Data[msgIdx++];
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            break;
        case DefaultResponse:
            break;
        }
        break;

    case HTHERMB1_CONFIG:
        frame_control = rsp->Data[msgIdx++];
        attribute_id2 = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
        msgIdx += 2;
        transaction_secuensce = rsp->Data[msgIdx++];
        command_id = rsp->Data[msgIdx++];
        switch (command_id)
        {
        case ReadAttributesResponse:

            while (rsp->Len > msgIdx)
            {
                attribute_id = BUILD_UINT16(rsp->Data[msgIdx], rsp->Data[msgIdx + 1]);
                msgIdx += 2;
                switch (attribute_id)
                {
                case 0x0000:
                    status_code = rsp->Data[msgIdx++];
                    data_type = rsp->Data[msgIdx++];
                    value1 = rsp->Data[msgIdx++];

                    msg.size_topic = 50;
                    msg.topic = mem_alloc(msg.size_topic);
                    sprintf(msg.topic, "smaway/%s/toplatform/config/113\0", mac);

                    msg.size_content = 100;
                    msg.content = mem_alloc(msg.size_content);
                    sprintf(msg.content, "{\"DeviceId\":\"%s\", \"PropertyName\" : \"water-scale-proof\",\"Value\": \"%d\"}", ieee_address, value1);
                    break;

                case 0x0001:
                    status_code = rsp->Data[msgIdx++];
                    data_type = rsp->Data[msgIdx++];
                    value1 = rsp->Data[msgIdx++];

                    msg.size_topic = 50;
                    msg.topic = mem_alloc(msg.size_topic);
                    sprintf(msg.topic, "smaway/%s/toplatform/config/113\0", mac);

                    msg.size_content = 100;
                    msg.content = mem_alloc(msg.size_content);
                    sprintf(msg.content, "{\"DeviceId\":\"%s\", \"PropertyName\" : \"frost\",\"Value\": \"%d\"}", ieee_address, value1);
                    break;

                case 0x0002:
                    status_code = rsp->Data[msgIdx++];
                    data_type = rsp->Data[msgIdx++];
                    value1 = rsp->Data[msgIdx++];

                    msg.size_topic = 50;
                    msg.topic = mem_alloc(msg.size_topic);
                    sprintf(msg.topic, "smaway/%s/toplatform/config/113\0", mac);

                    msg.size_content = 100;
                    msg.content = mem_alloc(msg.size_content);
                    sprintf(msg.content, "{\"DeviceId\":\"%s\", \"PropertyName\" : \"window-check\",\"Value\": \"%d\"}", ieee_address, value1);
                    break;

                case 0x0003:
                    status_code = rsp->Data[msgIdx++];
                    data_type = rsp->Data[msgIdx++];
                    value1 = rsp->Data[msgIdx++];

                    msg.size_topic = 50;
                    msg.topic = mem_alloc(msg.size_topic);
                    sprintf(msg.topic, "smaway/%s/toplatform/config/113\0", mac);

                    msg.size_content = 100;
                    msg.content = mem_alloc(msg.size_content);
                    sprintf(msg.content, "{\"DeviceId\":\"%s\", \"PropertyName\" : \"child-lock\",\"Value\": \"%d\"}", ieee_address, value1);
                    break;

                case 0x0004:
                    status_code = rsp->Data[msgIdx++];
                    data_type = rsp->Data[msgIdx++];
                    value1 = rsp->Data[msgIdx++];

                    msg.size_topic = 50;
                    msg.topic = mem_alloc(msg.size_topic);
                    sprintf(msg.topic, "smaway/%s/toplatform/config/113\0", mac);

                    msg.size_content = 100;
                    msg.content = mem_alloc(msg.size_content);
                    sprintf(msg.content, "{\"DeviceId\":\"%s\", \"PropertyName\" : \"key-away\",\"Value\": \"%d\"}", ieee_address, value1);
                    break;

                case 0x0005:
                    status_code = rsp->Data[msgIdx++];
                    data_type = rsp->Data[msgIdx++];
                    value1 = rsp->Data[msgIdx++];

                    msg.size_topic = 100;
                    msg.topic = mem_alloc(msg.size_topic);
                    sprintf(msg.topic, "smaway/%s/toplatform/item/%s/valve-state\0", mac, ieee_address);

                    msg.size_content = 10;
                    msg.content = mem_alloc(msg.size_content);
                    sprintf(msg.content, "%d", value1);
                    break;
                }
                sendToMqttCheckingMailboxNoWait(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
            }
            break;
        case WriteAttributesResponse:
            break;
        case ReportAttributes:
            break;
        case DefaultResponse:
            break;
        }
        break;
    }
    if (zdevice)
    {
        set_element((dbt_generic**) &zdevice, ZDEVICE, true);
    }

    sprintf(lm, "zigbee_recv - async cluster:0x%02X command: 0x%02X attribute:0x%02X",rsp->ClusterId,command_id, attribute_id);
    logevent(L_DEBUG, lm);
}

uint16_t get_frame_payload(uint8_t data_type, uint8_t *ptr)
{
    uint16_t value = 0;
    switch (data_type)
    {
    case unk:
        case nodata:
        return NULL;
    case boolean:
        case enum8:
        case int8:
        case uint8:
        case map8:
        case data8:
        return 1;
    case attribId:
        case clusterId:
        case semi_p:
        case enum16:
        case int16:
        case uint16:
        case map16:
        case data16:
        return 2;
    case int24:
        case uint24:
        case map24:
        case data24:
        return 3;
    case bacOID:
        case UTC:
        case date:
        case ToD:
        case simple_p:
        case int32:
        case uint32:
        case map32:
        case data32:
        return 4;
    case int40:
        case uint40:
        case map40:
        case data40:
        return 5;
    case int48:
        case uint48:
        case map48:
        case data48:
        return 6;
    case int56:
        case uint56:
        case map56:
        case data56:
        return 7;
    case EUI64:
        case double_p:
        case int64:
        case uint64:
        case map64:
        case data64:
        return 8;

    case octstr:
        case string:
        return ptr[0];

    case octstr16:
        case string16:
        return BUILD_UINT16(ptr[0], ptr[1]);

    case array:
        case structure:
        return 0;

    case set:
        case bag:
        return 0;

    case key128:
        return 16;

    }
    return 0;
}

uint8_t afRegister(RegisterFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 9 + (req->AppNumInClusters * 2) + (req->AppNumOutClusters * 2);
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = req->EndPoint;
        cmd[cmInd++] = (uint8_t) (req->AppProfId & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->AppProfId >> 8) & 0xFF);
        cmd[cmInd++] = (uint8_t) (req->AppDeviceId & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->AppDeviceId >> 8) & 0xFF);
        cmd[cmInd++] = req->AppDevVer;
        cmd[cmInd++] = req->LatencyReq;
        cmd[cmInd++] = req->AppNumInClusters;
        for (idx = 0; idx < req->AppNumInClusters; idx++)
        {
            cmd[cmInd++] = (uint8_t) (req->AppInClusterList[idx] & 0xFF);
            cmd[cmInd++] = (uint8_t) ((req->AppInClusterList[idx] >> 8) & 0xFF);
        }
        cmd[cmInd++] = req->AppNumOutClusters;
        for (idx = 0; idx < req->AppNumOutClusters; idx++)
        {
            cmd[cmInd++] = (uint8_t) (req->AppOutClusterList[idx] & 0xFF);
            cmd[cmInd++] = (uint8_t) ((req->AppOutClusterList[idx] >> 8) & 0xFF);
        }
        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_AF), MT_AF_REGISTER, cmd, cmdLen);

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
uint8_t afDataRequest(DataRequestFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 10 + req->Len;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);
        cmd[cmInd++] = req->DstEndpoint;
        cmd[cmInd++] = req->SrcEndpoint;
        cmd[cmInd++] = (uint8_t) (req->ClusterID & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->ClusterID >> 8) & 0xFF);
        cmd[cmInd++] = req->TransID;
        cmd[cmInd++] = req->Options;
        cmd[cmInd++] = req->Radius;
        cmd[cmInd++] = req->Len;
        for (idx = 0; idx < req->Len; idx++)
        {
            cmd[cmInd++] = req->Data[idx];

        }

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_AF),
        MT_AF_DATA_REQUEST,
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
uint8_t afDataRequestExt(DataRequestExtFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 20 + req->Len;
    char lm[70];
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = req->DstAddrMode;
        memcpy((cmd + cmInd), req->DstAddr, 8);
        cmInd += 8;
        cmd[cmInd++] = req->DstEndpoint;
        cmd[cmInd++] = (uint8_t) (req->DstPanID & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstPanID >> 8) & 0xFF);
        cmd[cmInd++] = req->SrcEndpoint;
        cmd[cmInd++] = (uint8_t) (req->ClusterId & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->ClusterId >> 8) & 0xFF);
        cmd[cmInd++] = req->TransId;
        cmd[cmInd++] = req->Options;
        cmd[cmInd++] = req->Radius;
        cmd[cmInd++] = (uint8_t) (req->Len & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Len >> 8) & 0xFF);
        for (idx = 0; idx < req->Len; idx++)
        {
            cmd[cmInd++] = req->Data[idx];
        }

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_AF), MT_AF_DATA_REQUEST_EXT, cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            rpcWaitMqClientMsg(50);
        }
        #ifdef _DEBUG_
        else
            printf("afDataRequestExt: No reply received for CLusterId: %d and AttrId: %d\n", req->ClusterId, req->Data[4]);
        #endif

        free(cmd);

        Task_sleep(1000);
        return status;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");
        sprintf(lm, "zigbee - data request: cluster: 0x%02X, memory was not allocated",req->ClusterId);
        logevent(L_ERROR, lm);
        return 1;
    }
}
uint8_t afDataRequestSrcRtg(DataRequestSrcRtgFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 11 + (req->RelayCount * 2) + req->Len;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = (uint8_t) (req->DstAddr & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->DstAddr >> 8) & 0xFF);
        cmd[cmInd++] = req->DstEndpoint;
        cmd[cmInd++] = req->SrcEndpoint;
        cmd[cmInd++] = (uint8_t) (req->ClusterID & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->ClusterID >> 8) & 0xFF);
        cmd[cmInd++] = req->TransID;
        cmd[cmInd++] = req->Options;
        cmd[cmInd++] = req->Radius;
        cmd[cmInd++] = req->RelayCount;
        for (idx = 0; idx < req->RelayCount; idx++)
        {
            cmd[cmInd++] = (uint8_t) (req->RelayList[idx] & 0xFF);
            cmd[cmInd++] = (uint8_t) ((req->RelayList[idx] >> 8) & 0xFF);
        }
        cmd[cmInd++] = req->Len;
        for (idx = 0; idx < req->Len; idx++)
        {
            cmd[cmInd++] = req->Data[idx];
        }

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_AF),
        MT_AF_DATA_REQUEST_SRC_RTG,
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
uint8_t afInterPanCtl(InterPanCtlFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 1 + req->Command;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = req->Command;
        for (idx = 0; idx < req->Command; idx++)
        {
            cmd[cmInd++] = req->Data[idx];
        }

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_AF),
        MT_AF_INTER_PAN_CTL,
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
uint8_t afDataStore(DataStoreFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 3 + req->Length;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        int idx;

        cmd[cmInd++] = (uint8_t) (req->Index & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Index >> 8) & 0xFF);
        cmd[cmInd++] = req->Length;
        for (idx = 0; idx < req->Length; idx++)
        {
            cmd[cmInd++] = req->Data[idx];
        }

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_AF), MT_AF_DATA_STORE, cmd, cmdLen);

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
uint8_t afDataRetrieve(DataRetrieveFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 7;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        memcpy((cmd + cmInd), req->TimeStamp, 4);
        cmInd += 4;
        cmd[cmInd++] = (uint8_t) (req->Index & 0xFF);
        cmd[cmInd++] = (uint8_t) ((req->Index >> 8) & 0xFF);
        cmd[cmInd++] = req->Length;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_AF), MT_AF_DATA_RETRIEVE, cmd, cmdLen);

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
uint8_t afApsfConfigSet(ApsfConfigSetFormat_t *req)
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 3;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = req->Endpoint;
        cmd[cmInd++] = req->FrameDelay;
        cmd[cmInd++] = req->WindowSize;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_AF), MT_AF_APSF_CONFIG_SET, cmd, cmdLen);

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

void processDataConfirm(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    DataConfirmFormat_t rsp;
    if (rpcLen < 3)
    {
        //printf("MT_RPC_ERR_LENGTH\n");
    }

    rsp.Status = rpcBuff[msgIdx++];
    rsp.Endpoint = rpcBuff[msgIdx++];
    rsp.TransId = rpcBuff[msgIdx++];
}
void processIncomingMsg(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    IncomingMsgFormat_t rsp;
    if (rpcLen < 17)
    {
        //printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.GroupId = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.ClusterId = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.SrcAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.SrcEndpoint = rpcBuff[msgIdx++];
    rsp.DstEndpoint = rpcBuff[msgIdx++];
    rsp.WasVroadcast = rpcBuff[msgIdx++];
    rsp.LinkQuality = rpcBuff[msgIdx++];
    rsp.SecurityUse = rpcBuff[msgIdx++];
    rsp.TimeStamp = 0;
    uint8_t i;
    for (i = 0; i < 4; i++)
        rsp.TimeStamp |= ((uint32_t) rpcBuff[msgIdx++]) << (i * 8);
    rsp.TransSeqNum = rpcBuff[msgIdx++];
    rsp.Len = rpcBuff[msgIdx++];
    if (rpcLen > 17)
    {
        uint32_t i;
        for (i = 0; i < rsp.Len; i++)
        {
            rsp.Data[i] = rpcBuff[msgIdx++];
        }
    }
    cluster_response(&rsp);

}
void processIncomingMsgExt(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    IncomingMsgExtFormat_t rsp;
    zcl_frame_payload_t frameFormat;
    if (rpcLen < 27)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }
    //printf("rpcLen = %d\n", rpcLen);

    rsp.GroupId = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.ClusterId = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.SrcAddrMode = rpcBuff[msgIdx++];
    rsp.SrcAddr = 0;
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if ((rsp.SrcAddrMode == 2 && i < 2) || rsp.SrcAddrMode == 3)
        {
            rsp.SrcAddr |= ((uint64_t) rpcBuff[msgIdx]) << (i * 8);
        }
        msgIdx++;
    }

    rsp.SrcEndpoint = rpcBuff[msgIdx++];
    rsp.SrcPanId = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
    rsp.DstEndpoint = rpcBuff[msgIdx++];
    rsp.WasVroadcast = rpcBuff[msgIdx++];
    rsp.LinkQuality = rpcBuff[msgIdx++];
    rsp.SecurityUse = rpcBuff[msgIdx++];
    rsp.TimeStamp = 0;
    for (i = 0; i < 4; i++)
        rsp.TimeStamp |= ((uint32_t) rpcBuff[msgIdx++]) << (i * 8);
    rsp.TransSeqNum = rpcBuff[msgIdx++];
    rsp.Len = rpcBuff[msgIdx++];
    uint32_t ind;
    for (ind = 0; ind < rsp.Len; ind++)
    {
        rsp.Data[ind] = rpcBuff[msgIdx++];
    }
}
void processReflectError(uint8_t *rpcBuff, uint8_t rpcLen)
{
    uint8_t msgIdx = 2;
    ReflectErrorFormat_t rsp;
    if (rpcLen < 6)
    {
        printf("MT_RPC_ERR_LENGTH\n");
    }

    rsp.Status = rpcBuff[msgIdx++];
    rsp.Endpoint = rpcBuff[msgIdx++];
    rsp.TransId = rpcBuff[msgIdx++];
    rsp.DstAddrMode = rpcBuff[msgIdx++];
    rsp.DstAddr = BUILD_UINT16(rpcBuff[msgIdx], rpcBuff[msgIdx + 1]);
    msgIdx += 2;
}

int16_t af_cmd_send(uint8_t cmd1, uint8_t *rpcFrame, uint8_t rpcLen)
{
    switch (cmd1)
    {
    case MT_AF_REGISTER:
        afRegister((RegisterFormat_t*) rpcFrame);
        break;
    case MT_AF_DATA_REQUEST:
        break;
    case MT_AF_DATA_REQUEST_EXT:
        afDataRequestExt((DataRequestExtFormat_t*) rpcFrame);
        break;
    case MT_AF_DATA_REQUEST_SRC_RTG:
        break;
    case MT_AF_INTER_PAN_CTL:
        break;
    case MT_AF_DATA_STORE:
        break;
    case MT_AF_DATA_RETRIEVE:
        break;
    case MT_AF_APSF_CONFIG_SET:
        break;
    }
    return 0;
}

void afProcess(uint8_t *rpcBuff, uint8_t rpcLen)
{
    if ((rpcBuff[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_AREQ)
    {
        //Read CMD1 and processes the specific SREQ
        switch (rpcBuff[1])
        {
        case MT_AF_DATA_CONFIRM:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "afProcess: MT_AF_DATA_CONFIRM\n");
            processDataConfirm(rpcBuff, rpcLen);
            break;
        case MT_AF_INCOMING_MSG:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "afProcess: MT_AF_INCOMING_MSG\n");
            processIncomingMsg(rpcBuff, rpcLen);
            break;
        case MT_AF_INCOMING_MSG_EXT:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE,"afProcess: MT_AF_INCOMING_MSG_EXT\n");
            processIncomingMsgExt(rpcBuff, rpcLen);
            break;
        case MT_AF_REFLECT_ERROR:
            //TODO LOG dbg_print(PRINT_LEVEL_VERBOSE, "afProcess: MT_AF_REFLECT_ERROR\n");
            processReflectError(rpcBuff, rpcLen);
            break;
        default:
            //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "processRpcAf: CMD0:%x, CMD1:%x, not handled\n", rpcBuff[0], rpcBuff[1]);
            break;
        }
    }
}

bool afPendResponseSemaphore(void)
{
    if(!Semaphore_pend(sem_mtaf_response,AF_RESPONSE_DEFAULT_TIMEOUT))
    {
        afResetResponseSemaphore();
        return false;
    }
    else
        return true;
}

void afPostResponseSemaphore(void)
{
    Semaphore_post(sem_mtaf_response);
}

void afResetResponseSemaphore(void)
{
    Semaphore_reset(sem_mtaf_response, 0);
}

void send_112(char ieee_address[24], bool status)
{
    msg_mqtt_t msg;
    char mac[18];
    db_get_mac(mac);

    msg.size_topic = 100;
    msg.topic = mem_alloc(msg.size_topic);
    sprintf(msg.topic, "smaway/%s/toplatform/config/112", mac);

    // Determine the device status as a string
    const char *device_status = status ? "ONLINE" : "OFFLINE";

    // Create the payload in JSON format
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"DeviceId\":\"%s\",\"Status\":\"%s\"}", ieee_address, device_status);

    msg.size_content = strlen(payload) + 1;
    msg.content = mem_alloc(msg.size_content);
    strcpy(msg.content, payload);

    // Send message to the MQTT queue
    sendToMqttCheckingMailboxNoWait(&msg);
    //Mailbox_post(m_mqtt_send, &msg, BIOS_NO_WAIT);
}

