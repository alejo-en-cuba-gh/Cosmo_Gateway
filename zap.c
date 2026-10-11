/*
 * zap.c
 *
 *  Created on: Aug 16, 2024
 *      Author: HelierDavidCamejoAra
 */
#include "zap.h"
#include "errno.h"
#include "jsonc.h"
#include <ti/sysbios/knl/Clock.h>

#define STARTDELAY 0


Task_Handle t_rpc_recv = NULL;
Semaphore_Handle sem_uart = NULL;
extern Semaphore_Handle sem_systemprintf;

extern dbt_db config_db;

uint16_t coordinatorNwkAddr = 0xFFFF;
bool runConfigSteps_THERMB01(dbt_zdevice *zdev, uint8_t step);  //TODO: Make single function for any configuration receiving the step number
bool runConfigSteps_HTHERMB1(dbt_zdevice *zdev, uint8_t step);

//init ZDO device state
devStates_t devState = DEV_HOLD;

bool value_on_array(uint16_t *array, uint8_t length, uint16_t value)
{
    uint8_t i;
    for (i = 0; i < length; i++)
    {
        if (array[i] == value)
        {
            return true;
        }
    }
    return false;
}
uint8_t array_without_duplicate(uint16_t *array1, uint8_t length1, uint16_t *array2, uint8_t length2, uint16_t *result)
{
    uint8_t Indice = 0;
    uint8_t i;
    // A�adir elementos del primer array
    for (i = 0; i < length1; i++)
    {
        if (!value_on_array(result, Indice, array1[i]))
        {
            result[Indice++] = array1[i];
        }
    }

    // A�adir elementos del segundo array
    for (i = 0; i < length2; i++)
    {
        if (!value_on_array(result, Indice, array2[i]))
        {
            result[Indice++] = array2[i];
        }
    }

    return Indice;
}

void zap_add_zdevice_coord()
{
    bool status = 0;
    dbt_zdevice *zdevice = NULL;
    dbt_zendpoint *zendpoint = NULL;
    IeeeAddrReqFormat_t IeeeAddrReq;
    IeeeAddrRspFormat_t *IeeeAddrRsp = NULL;
    RegisterFormat_t Register;
    msg_rpc_t msg_app;
    uint16_t j = 0;
    void *r;

    msg_app.data = NULL;
    msg_app.device_id = NULL;
    msg_app.item_type = NULL;
    msg_app.data_length = 0;
    msg_app.id_length = 0;
    msg_app.read_write = 0;
    msg_app.type_length = 0;

    if (config_db.db_zdevice == NULL)
    {

        while (status == 0)
        {
            IeeeAddrReq.ShortAddr = 0;
            IeeeAddrReq.ReqType = 0;
            IeeeAddrReq.StartIndex = 0;

            zdo_ieee_addr_req(&IeeeAddrReq);

            status = Mailbox_pend(m_add_device, &msg_app, 5000);
            if (status)
            {
                zdevice = (dbt_zdevice*) db_add_item(ZDEVICE, (dbt_generic**) &config_db.db_zdevice, (dbt_generic**) &config_db.db_zdevice_last);

                IeeeAddrRsp = (IeeeAddrRspFormat_t*) msg_app.data;

                zdevice->nwk_address = 0;
                zdevice->mfg_id = 0;
                zdevice->ep_count = 0;
                zdevice->parent_address = 0;
                zdevice->model_id[0] = 0;
                zdevice->cap_info = 0;
                zdevice->ieee_address = IeeeAddrRsp->IEEEAddr;
                zdevice->firmware_version = 0;
                zdevice->status = 0;

                set_element((dbt_generic**) &zdevice, ZDEVICE, true);

                mem_free(msg_app.device_id, msg_app.id_length);
                mem_free(msg_app.item_type, msg_app.type_length);
                mem_free(msg_app.data, msg_app.data_length);
            }
            Task_sleep(300);
        }
    }
    else
    {
        zendpoint = (dbt_zendpoint*) get_element((dbt_generic**) &config_db.db_zendpoint, ZENDPOINT);

        while (zendpoint)
        {
            Register.EndPoint = zendpoint->id;
            Register.AppProfId = zendpoint->profile_id;
            Register.AppDeviceId = zendpoint->device_id;
            Register.AppDevVer = zendpoint->device_version;
            Register.LatencyReq = 0;
            Register.AppNumInClusters = zendpoint->num_in_cluster;
            Register.AppNumOutClusters = zendpoint->num_in_cluster;
            for (j = 0; j < zendpoint->num_in_cluster; j++)
            {
                Register.AppInClusterList[j] = zendpoint->clusters[j];
                Register.AppOutClusterList[j] = zendpoint->clusters[j];
            }
            afRegister(&Register);

            Task_sleep(200);

            r = zendpoint->element->next;
            mem_free(zendpoint, sizeof(dbt_zendpoint));
            zendpoint = (dbt_zendpoint*) get_element((dbt_generic**) &r, ZENDPOINT);
        }
    }
}

void ieee_addr_req(uint16_t NwkAddr)
{
    IeeeAddrReqFormat_t IeeeAddrReq = { 0 };

    IeeeAddrReq.ShortAddr = NwkAddr;
    IeeeAddrReq.ReqType = 0x00;
    IeeeAddrReq.StartIndex = 0x00;

    zdo_ieee_addr_req(&IeeeAddrReq);
}
void node_desc_req(uint16_t NwkAddr)
{
    NodeDescReqFormat_t NodeDescReq = { 0 };

    NodeDescReq.DstAddr = NwkAddr;
    NodeDescReq.NwkAddrOfInterest = NwkAddr;

    zdo_node_desc_req(&NodeDescReq);
}
void active_ep_req(uint16_t NwkAddr)
{
    ActiveEpReqFormat_t ActiveEpReq = { 0 };

    ActiveEpReq.DstAddr = NwkAddr;
    ActiveEpReq.NwkAddrOfInterest = NwkAddr;

    zdo_active_ep_req(&ActiveEpReq);
}
void simple_desc_req(uint16_t NwkAddr, uint8_t endpoint)
{
    SimpleDescReqFormat_t SimpleDescReq = { 0 };

    SimpleDescReq.DstAddr = NwkAddr;
    SimpleDescReq.NwkAddrOfInterest = NwkAddr;
    SimpleDescReq.Endpoint = endpoint;

    zdo_simple_desc_req(&SimpleDescReq);
}

int8_t set_thermostat_Control_SeqOfOper(dbt_zdevice *zdev_1, int16_t endpoint)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 7;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x01; //TransactionSeqNumber
    req.Data[2] = 0x02; //Command_ID
    req.Data[3] = 0x1B; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH
    req.Data[5] = 0x30; //Data Type
    req.Data[6] = 0x04; //Value

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afResetResponseSemaphore();

    afDataRequestExt(&req);

    return afPendResponseSemaphore();
}

int8_t identify(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t time_s)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0003;  //General: Identify
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x01; //Frame Control - Frame Type Command is specific to a Cluster
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = LO_UINT16(time_s); //Identify_Time_Sec LOW
    req.Data[4] = HI_UINT16(time_s); //Identify_Time_Sec HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}


int8_t configure_report_child_lock_enabled(dbt_zdevice *zdev_1, int16_t endpoint,
                                           uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    memset(&req.DstAddr[2], 0, 6);

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 13;

    req.Data[0] = 0x04; // Frame Control: Manufacturer specific
    req.Data[1] = 0x02; // Manufacturer Code LOW
    req.Data[2] = 0x10; // Manufacturer Code HIGH
    req.Data[3] = 0x00; // Transaction Seq Number
    req.Data[4] = 0x06; // Command ID: Configure Reporting

    req.Data[5] = 0x00; // Direction: Reported
    req.Data[6] = 0x03; // Attribute ID LOW
    req.Data[7] = 0x00; // Attribute ID HIGH
    req.Data[8] = 0x10; // Data Type: Boolean

    req.Data[9]  = LO_UINT16(min_rep_time);
    req.Data[10] = HI_UINT16(min_rep_time);
    req.Data[11] = LO_UINT16(max_rep_time);
    req.Data[12] = HI_UINT16(max_rep_time);

    req.Data[13] = 0x00; // Reportable change = 0

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);
    return 0;
}


int8_t configure_Rep_target_temp(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    if (zendpoint == NULL) return -1;

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    memset(&req.DstAddr[2], 0, 6);

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201; // Cl�ster Thermostat
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 13;

    req.Data[0] = 0x00; // Frame Control (ZCL Frame sin manufacturer specific)
    req.Data[1] = 0x00; // Transaction Sequence Number
    req.Data[2] = 0x06; // Configure Reporting command

    req.Data[3] = 0x00;       // Direcci�n: hacia servidor
    req.Data[4] = 0x12;       // Atributo LOW byte (0x0012 - Occupied Heating Setpoint)
    req.Data[5] = 0x00;       // Atributo HIGH byte
    req.Data[6] = 0x29;       // Data Type (int16)
    req.Data[7] = LO_UINT16(min_rep_time);
    req.Data[8] = HI_UINT16(min_rep_time);
    req.Data[9] = LO_UINT16(max_rep_time);
    req.Data[10] = HI_UINT16(max_rep_time);
    req.Data[11] = 0x00; // Reportable Change LOW
    req.Data[12] = 0x00; // Reportable Change HIGH

    // No liberar memoria si zendpoint viene de BD
    afDataRequestExt(&req);
    return 0;
}

int8_t configure_Rep_local_temp(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 13;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x03; //TransactionSeqNumber
    req.Data[2] = 0x06; //Command_ID - Configure Reporting

    req.Data[3] = 0x00; //Direction - Reported

    //Local-temperature
    req.Data[4] = 0x00; //Attribute_ID LOW
    req.Data[5] = 0x00; //Attribute_ID HIGH

    //Signed 16-bit interger
    req.Data[6] = 0x29; //Data Type

    //Minimun Reporting Interval
    req.Data[7] = LO_UINT16(min_rep_time); //Value LOW
    req.Data[8] = HI_UINT16(min_rep_time); //Value HIGH

    //Maximun Reporting Interval
    req.Data[9] = LO_UINT16(max_rep_time); //Value LOW
    req.Data[10] = HI_UINT16(max_rep_time); //Value HIGH

    //Reportable Change
    req.Data[11] = 0x00;
    req.Data[12] = 0x00;

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afResetResponseSemaphore();

    afDataRequestExt(&req);

    return afPendResponseSemaphore();
}

int8_t configure_rep_system_mode(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 11;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x04; //TransactionSeqNumber
    req.Data[2] = 0x06; //Command_ID - Configure Reporting

    req.Data[3] = 0x00; //Direction - Reported

    //Local-temperature
    req.Data[4] = 0x1C; //Attribute_ID LOW
    req.Data[5] = 0x00; //Attribute_ID HIGH

    //Signed 16-bit interger
    req.Data[6] = 0x30; //Data Type

    //Minimun Reporting Interval
    req.Data[7] = LO_UINT16(min_rep_time); //Value LOW
    req.Data[8] = HI_UINT16(min_rep_time); //Value HIGH

    //Maximun Reporting Interval
    req.Data[9] = LO_UINT16(max_rep_time); //Value LOW
    req.Data[10] = HI_UINT16(max_rep_time); //Value HIGH

    /*//Reportable Change
    req.Data[11] = 0; //Value LOW
    req.Data[12] = 0; //Value HIGH*/

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afResetResponseSemaphore();

    afDataRequestExt(&req);

    return afPendResponseSemaphore();
}

int8_t configure_rep_SM_OHeatSetp(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 13;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x02; //TransactionSeqNumber
    req.Data[2] = 0x06; //Command_ID - Configure Reporting

    //Attribute 0
    req.Data[3] = 0x00; //Direction: Reported

    //Local-temperature
    req.Data[4] = 0x12; //Attribute_ID LOW
    req.Data[5] = 0x00; //Attribute_ID HIGH

    //Signed 16-bit interger
    req.Data[6] = 0x29; //Data Type

    //Minimun Reporting Interval
    req.Data[7] = LO_UINT16(min_rep_time); //Value LOW
    req.Data[8] = HI_UINT16(min_rep_time); //Value HIGH

    //Maximun Reporting Interval
    req.Data[9] = LO_UINT16(max_rep_time); //Value LOW
    req.Data[10] = HI_UINT16(max_rep_time); //Value HIGH

    //Reportable Change
    req.Data[11] = 0x00; //Value LOW
    req.Data[12] = 0x00; //Value HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afResetResponseSemaphore();

    afDataRequestExt(&req);

    return afPendResponseSemaphore();

}
int8_t configure_powerconf(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0001;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 11;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x05; //TransactionSeqNumber
    req.Data[2] = 0x06; //Command_ID - Configure Reporting

    req.Data[3] = 0x00; //Direction - Reported

    //Battery Alarm State
    req.Data[4] = 0x3E; //Attribute_ID LOW
    req.Data[5] = 0x00; //Attribute_ID HIGH

    //Signed 32-Bit map
    req.Data[6] = 0x1B; //Data Type

    //Minimun Reporting Interval
    req.Data[7] = LO_UINT16(min_rep_time); //Value LOW
    req.Data[8] = HI_UINT16(min_rep_time); //Value HIGH

    //Maximun Reporting Interval
    req.Data[9] = LO_UINT16(max_rep_time); //Value LOW
    req.Data[10] = HI_UINT16(max_rep_time); //Value HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t configure_battery_percentage(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0001;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 12;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x06; //Command_ID - Configure Reporting

    req.Data[3] = 0x00; //Direction - Reported

    //Battery Alarm State
    req.Data[4] = 0x21; //Attribute_ID LOW
    req.Data[5] = 0x00; //Attribute_ID HIGH

    //Signed Unsigned 8-bit Interger
    req.Data[6] = 0x20; //Data Type

    //Minimun Reporting Interval
    req.Data[7] = LO_UINT16(min_rep_time); //Value LOW
    req.Data[8] = HI_UINT16(min_rep_time); //Value HIGH

    //Maximun Reporting Interval
    req.Data[9] = LO_UINT16(max_rep_time); //Value LOW
    req.Data[10] = HI_UINT16(max_rep_time); //Value HIGH

    //Reportable Change
    req.Data[11] = 0x00;

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t configure_diagnostic_lqi(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0B05; //Diagnostics
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 12;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x06; //TransactionSeqNumber
    req.Data[2] = 0x06; //Command_ID - Configure Reporting

    req.Data[3] = 0x00; //Direction - Reported

    //Battery Alarm State
    req.Data[4] = 0x1C; //Attribute_ID LOW
    req.Data[5] = 0x01; //Attribute_ID HIGH

    //Signed Unsigned 8-bit interger
    req.Data[6] = 0x20; //Data Type

    //Minimun Reporting Interval
    req.Data[7] = LO_UINT16(min_rep_time); //Value LOW
    req.Data[8] = HI_UINT16(min_rep_time); //Value HIGH

    //Maximun Reporting Interval
    req.Data[9] = LO_UINT16(max_rep_time); //Value LOW
    req.Data[10] = HI_UINT16(max_rep_time); //Value HIGH

    req.Data[11] = 0x00; //Reportable Change

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t configure_valve_state(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 14;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x06; //Command_ID - Configure Reporting

    req.Data[5] = 0x00; //Direction - Reported

    //Valve State
    req.Data[6] = 0x05; //Attribute_ID LOW
    req.Data[7] = 0x00; //Attribute_ID HIGH

    //Signed Unsigned 8-bit Interger
    req.Data[8] = 0x20; //Data Type

    //Minimun Reporting Interval
    req.Data[9] = LO_UINT16(min_rep_time); //Value LOW
    req.Data[10] = HI_UINT16(min_rep_time); //Value HIGH

    //Maximun Reporting Interval
    req.Data[11] = LO_UINT16(max_rep_time); //Value LOW
    req.Data[12] = HI_UINT16(max_rep_time); //Value HIGH

    //Reportable Change
    req.Data[13] = 0x00;

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}

// items_types
int8_t get_fv_MN_model(uint16_t nwk_address)
{
    DataRequestExtFormat_t req;
    dbt_zdevice *zdevice = NULL;
    dbt_zendpoint *zendpoint = NULL;

    zdevice = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->nwk_address, false, ZDEVICE, NUMBER, 0, nwk_address, "", N_UINT16);
    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdevice->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");
    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(nwk_address);
    req.DstAddr[1] = HI_UINT16(nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0000;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 9;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID - read Attributes
    req.Data[3] = 0x01; //AttributeID
    req.Data[4] = 0x00; //AttributeID
    req.Data[5] = 0x04; //AttributeID
    req.Data[6] = 0x00; //AttributeID
    req.Data[7] = 0x05; //AttributeID
    req.Data[8] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    mem_free(zdevice, sizeof(dbt_zdevice));

    afDataRequestExt(&req);

    return 0;
}

int8_t configurar_Reporte_OnOff(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time) {
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;


    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02; // Direcci�n de 16 bits
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0006; // Cluster ID para On/Off
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 11;
    req.Data[0] = 0x00; // Control de Frame
    req.Data[1] = 0x00; // N�mero de Secuencia de Transacci�n
    req.Data[2] = 0x06; // Command_ID - Configurar Reporte

    req.Data[3] = 0x00; // Direcci�n - Reportado

    // Atributo On/Off
    req.Data[4] = 0x00; // Attribute_ID LOW
    req.Data[5] = 0x00; // Attribute_ID HIGH

    // Tipo de dato: Booleano
    req.Data[6] = 0x10; // Data Type

    // Intervalo de Reporte M�nimo
    req.Data[7] = LO_UINT16(min_rep_time); // Valor LOW
    req.Data[8] = HI_UINT16(min_rep_time); // Valor HIGH

    // Intervalo de Reporte M�ximo
    req.Data[9] = LO_UINT16(max_rep_time); // Valor LOW
    req.Data[10] = HI_UINT16(max_rep_time); // Valor HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);
    return 0;
}

int8_t configurar_Reporte_BinaryInput(dbt_zdevice *zdev_1, int16_t endpoint, uint16_t min_rep_time, uint16_t max_rep_time) {
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02; // Direcci�n de 16 bits
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x000F; // Cluster ID para Binary Input
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 11;
    req.Data[0] = 0x00; // Control de Frame
    req.Data[1] = 0x00; // N�mero de Secuencia de Transacci�n
    req.Data[2] = 0x06; // Command_ID - Configurar Reporte

    req.Data[3] = 0x00; // Direcci�n - Reportado

    // Atributo Binary Input - Present Value
    req.Data[4] = LO_UINT16(0x0055); // Attribute_ID LOW
    req.Data[5] = HI_UINT16(0x0055); // Attribute_ID HIGH

    // Tipo de dato: Booleano
    req.Data[6] = 0x10; // Data Type

    // Intervalo de Reporte M�nimo
    req.Data[7] = LO_UINT16(min_rep_time); // Valor LOW
    req.Data[8] = HI_UINT16(min_rep_time); // Valor HIGH

    // Intervalo de Reporte M�ximo
    req.Data[9] = LO_UINT16(max_rep_time); // Valor LOW
    req.Data[10] = HI_UINT16(max_rep_time); // Valor HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);
    return 0;
}


/*int8_t read_onoff(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");
    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0006;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID
    req.Data[3] = 0x00; //AttributeID
    req.Data[4] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}

int8_t read_onoff2(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0002, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0006;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID
    req.Data[3] = 0x00; //AttributeID
    req.Data[4] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}*/

int8_t read_level_control(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0002, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0008;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID
    req.Data[3] = 0x00; //AttributeID
    req.Data[4] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_local_temperature(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x09; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x00; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_target_temperature(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x0A; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x12; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_system_mode(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x0B; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x1C; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_lqi_value(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0B05;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x07; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x1C; //Attribute_ID LOW
    req.Data[4] = 0x01; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_battery_low(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0001;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x08; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x3E; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_battery_percent(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0001;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x21; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_power_source(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0000;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x07; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}

int8_t read_valve_property(dbt_zdevice *zdev_1, int16_t endpoint)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 13;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x00; //Command_ID

    req.Data[5] = 0x00; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH

    req.Data[7] = 0x01; //Attribute_ID LOW
    req.Data[8] = 0x00; //Attribute_ID HIGH

    req.Data[9] = 0x02; //Attribute_ID LOW
    req.Data[10] = 0x00; //Attribute_ID HIGH

    req.Data[11] = 0x03; //Attribute_ID LOW
    req.Data[12] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_water_scale_proof(dbt_zdevice *zdev_1, int16_t endpoint)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 7;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x00; //Command_ID
    req.Data[5] = 0x00; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_frost_enabled(dbt_zdevice *zdev_1, int16_t endpoint)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 7;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x00; //Command_ID
    req.Data[5] = 0x01; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_window_check_enabled(dbt_zdevice *zdev_1, int16_t endpoint)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 7;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x00; //Command_ID
    req.Data[5] = 0x02; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_child_lock_enabled(dbt_zdevice *zdev_1, int16_t endpoint)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 7;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x00; //Command_ID
    req.Data[5] = 0x03; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_key_away_enabled(dbt_zdevice *zdev_1, int16_t endpoint)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 7;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x00; //Command_ID
    req.Data[5] = 0x04; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t read_motor_opening_size(dbt_zdevice *zdev_1, int16_t endpoint)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 7;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x00; //Command_ID
    req.Data[5] = 0x05; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}

int8_t sys_osal_wite(uint8_t id, uint8_t offset, uint8_t len, uint8_t *value)
{
    uint8_t status;
    uint8_t i = 0;
    OsalNvWriteFormat_t nvWrite;
    msg_rpc_t msg_srsp;

    msg_srsp.data = NULL;
    msg_srsp.device_id = NULL;
    msg_srsp.item_type = NULL;

    msg_srsp.id_length = 0;
    msg_srsp.type_length = 0;
    msg_srsp.read_write = 0;
    msg_srsp.data_length = 0;

    nvWrite.Id = id;
    nvWrite.Offset = offset;
    nvWrite.Len = len;
    for (i = 0; i < len; i++)
    {
        nvWrite.Value[i] = value[i];
    }
    status = sysOsalNvWrite(&nvWrite);

    Mailbox_pend(m_rpc_srsp, &msg_srsp, 5000);

    if (((msg_srsp.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
    && ((msg_srsp.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP)
    && (msg_srsp.data[1] == MT_SYS_OSAL_NV_WRITE))
    {
        mem_free(msg_srsp.data, msg_srsp.data_length);
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
        return -1;
    }
    return status;
}
int8_t set_NV_startup(uint8_t startupOption)
{
    msg_rpc_t msg_srsp;
    msg_srsp.data = NULL;
    msg_srsp.device_id = NULL;
    msg_srsp.item_type = NULL;

    msg_srsp.id_length = 0;
    msg_srsp.type_length = 0;
    msg_srsp.read_write = 0;
    msg_srsp.data_length = 0;

    uint8_t status;
    OsalNvWriteFormat_t nvWrite;

    // sending startup option
    nvWrite.Id = ZCD_NV_STARTUP_OPTION;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = startupOption;
    status = sysOsalNvWrite(&nvWrite);

    Mailbox_pend(m_rpc_srsp, &msg_srsp, 5000);

    if (((msg_srsp.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
    && ((msg_srsp.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP)
    && (msg_srsp.data[1] == MT_SYS_OSAL_NV_WRITE))
    {
        mem_free(msg_srsp.data, msg_srsp.data_length);
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
        return -1;
    }
    return status;
}
int8_t sys_reset(uint8_t Type)
{
    uint8_t status;
    msg_mqtt_t msg_app;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 1;
    uint8_t *cmd = mem_alloc(cmdLen);

    if (cmd)
    {
        cmd[cmInd++] = Type;

        status = rpcSendFrame((MT_RPC_CMD_AREQ | MT_RPC_SYS_SYS), MT_SYS_RESET_REQ, cmd, cmdLen);

        if (status == MT_RPC_SUCCESS)
        {
            status = Mailbox_pend(m_mtSys, &msg_app, 5000);

            //Mailbox_post(m_mqtt_send, &msg_app, BIOS_WAIT_FOREVER);

            cmdLen = 1;
            if (!status)
            {
                mem_free(cmd, cmdLen);
                return -1;
            }
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            mem_free(cmd, cmdLen);
            return -1;
        }
        mem_free(cmd, cmdLen);
        return 0;
    }
    else
    {
        //TODO LOG dbg_print(PRINT_LEVEL_WARNING, "Memory for cmd was not allocated\n");;
        return -1;
    }
}
int8_t set_dev_type(uint8_t devType)
{
    uint8_t status;
    OsalNvWriteFormat_t nvWrite;
    msg_rpc_t msg = { 0 };

    nvWrite.Id = ZCD_NV_LOGICAL_TYPE;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = devType;
    status = sysOsalNvWrite(&nvWrite);

    if (status == MT_RPC_SUCCESS)
    {
        Mailbox_pend(m_rpc_srsp, &msg, 5000);
        if (((msg.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
        && ((msg.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP))
        {
            mem_free(msg.data, msg.data_length);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }

    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "\n");
    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n",status);

    return status;
}
int8_t set_panid(uint32_t panId)
{
    uint8_t status;
    OsalNvWriteFormat_t nvWrite;
    msg_rpc_t msg = { 0 };

    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "\n");
    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "NV Write PAN ID cmd sending...\n");

    nvWrite.Id = ZCD_NV_PANID;
    nvWrite.Offset = 0;
    nvWrite.Len = 2;
    nvWrite.Value[0] = LO_UINT16(panId);
    nvWrite.Value[1] = HI_UINT16(panId);
    status = sysOsalNvWrite(&nvWrite);

    if (status == MT_RPC_SUCCESS)
    {
        Mailbox_pend(m_rpc_srsp, &msg, 2500);

        if (((msg.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
        && ((msg.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP))
        {
            //mem_free(msg.data, msg.length);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }

    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "\n");
    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n",status);

    return status;
}
int8_t set_channel_NV(uint8_t channel)
{
    uint8_t status;
    OsalNvWriteFormat_t nvWrite;
    msg_rpc_t msg = { 0 };
    uint32_t channel_array = 0;
    channel_array = 1 << channel;

    nvWrite.Id = ZCD_NV_CHANLIST;
    nvWrite.Offset = 0;
    nvWrite.Len = 4;
    nvWrite.Value[0] = BREAK_UINT32(channel_array, 0);
    nvWrite.Value[1] = BREAK_UINT32(channel_array, 1);
    nvWrite.Value[2] = BREAK_UINT32(channel_array, 2);
    nvWrite.Value[3] = BREAK_UINT32(channel_array, 3);

    status = sysOsalNvWrite(&nvWrite);

    if (status == MT_RPC_SUCCESS)
    {
        Mailbox_pend(m_rpc_srsp, &msg, 5000);
        if (((msg.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
        && ((msg.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP))
        {
            mem_free(msg.data, msg.data_length);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }

    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "\n");
    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n",status);

    return status;
}
int8_t set_channel(uint8_t channel)
{
    int8_t status;
    status = appCnfBDBsetChannel(1, channel);
    if (status)
    {
        return status;
    }

    status = appCnfBDBsetChannel(0, 0);
    if (status)
    {
        return status;
    }
    return status;
}
int8_t set_zdo_direct_cb()
{
    OsalNvWriteFormat_t nvWrite;
    int8_t status;
    msg_rpc_t msg = { 0 };

    // setting chanList
    nvWrite.Id = ZCD_NV_ZDO_DIRECT_CB;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = 1;
    status = sysOsalNvWrite(&nvWrite);

    if (status == MT_RPC_SUCCESS)
    {
        Mailbox_pend(m_rpc_srsp, &msg, 2500);

        if (((msg.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
        && ((msg.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP))
        {
            mem_free(msg.data, msg.data_length);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }

    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "\n");
    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n",status);

    return status;
}
int8_t set_startup_optionclean()
{
    OsalNvWriteFormat_t nvWrite;
    int8_t status;
    msg_rpc_t msg = { 0 };

    // setting chanList
    nvWrite.Id = ZCD_NV_STARTUP_OPTION;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = 3;
    status = sysOsalNvWrite(&nvWrite);

    if (status == MT_RPC_SUCCESS)
    {
        Mailbox_pend(m_rpc_srsp, &msg, 5000);

        if (((msg.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
        && ((msg.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP))
        {
            mem_free(msg.data, msg.data_length);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }

    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "\n");
    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n",status);

    return status;
}
int8_t set_startup_option()
{
    OsalNvWriteFormat_t nvWrite;
    int8_t status;
    msg_rpc_t msg = { 0 };

    // setting chanList
    nvWrite.Id = ZCD_NV_STARTUP_OPTION;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = 0;
    status = sysOsalNvWrite(&nvWrite);

    if (status == MT_RPC_SUCCESS)
    {
        Mailbox_pend(m_rpc_srsp, &msg, 2500);

        if (((msg.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
        && ((msg.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP))
        {
            mem_free(msg.data, msg.data_length);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }

    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "\n");
    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n",status);

    return status;
}
int32_t registerAf(void)
{
    int32_t status = 0;
    msg_rpc_t msg;
    RegisterFormat_t reg;

    reg.EndPoint = 1;
    reg.AppProfId = 0x0104;
    reg.AppDeviceId = 0x0100;
    reg.AppDevVer = 1;
    reg.LatencyReq = 0;
    reg.AppNumInClusters = 1;
    reg.AppInClusterList[0] = 0x0006;
    reg.AppNumOutClusters = 0;

    status = afRegister(&reg);

    Mailbox_pend(m_rpc_srsp, &msg, 5000);

    if (msg.data)
    {
        //mem_free(msg.data, msg.length);
    }

    return status;
}
int32_t start_zbnetwork()
{
    int8_t status;

    //registerAf();
    //TODO LOG consolePrint("EndPoint: 1\n");

    status = zdoInit();

    //TODO LOG dbg_print(PRINT_LEVEL_INFO, "process zdoStatechange callbacks\n");

    //set startup option back to keep configuration in case of reset
    //status = setNVStartup(0);

    return status;
}
int32_t zdo_permit_join_req(uint8_t duration)
{
    int8_t status;

    MgmtPermitJoinReqFormat_t join;

    join.AddrMode = 0x0F;
    join.DstAddr = 0xFFFC;
    join.Duration = duration;
    join.TCSignificance = 1;

    status = zdoMgmtPermitJoinReq(&join);

    return status;
}
int8_t get_nwk_key()
{
    msg_rpc_t data_resp;
    msg_mqtt_t msg;
    char mac[18];

    db_get_mac(mac);

    int8_t status;

    OsalNvReadFormat_t nvRead;

    //Send command
    nvRead.Id = ZCD_NV_NWK_ACTIVE_KEY_INFO;
    nvRead.Offset = 0;
    status = sysOsalNvRead(&nvRead);

    if (status == MT_RPC_SUCCESS)
    {
        //Wait for Sincronic response
        Mailbox_pend(m_rpc_srsp, &data_resp, 5000);

        if (((data_resp.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
        && ((data_resp.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP)
        && (data_resp.data[1] == MT_SYS_OSAL_NV_READ))
        {
            //to send to app response
            msg.size_content = 60;
            msg.content = mem_alloc(msg.size_content);
            sprintf(msg.content,
                    "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                    data_resp.data[5],
                    data_resp.data[6],
                    data_resp.data[7],
                    data_resp.data[8],
                    data_resp.data[9],
                    data_resp.data[10],
                    data_resp.data[11],
                    data_resp.data[12],
                    data_resp.data[13],
                    data_resp.data[14],
                    data_resp.data[15],
                    data_resp.data[16],
                    data_resp.data[17],
                    data_resp.data[18],
                    data_resp.data[19],
                    data_resp.data[20]);

            mem_free(data_resp.data, data_resp.data_length);

            msg.size_topic = 50;
            msg.topic = malloc(msg.size_topic);
            sprintf(msg.topic, "smaway/%s/tocloud/config/GET_NWK_KEY\0", mac);

            sendToMqttCheckingMailbox(&msg);
            //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
        }
        else
        {
            return -1;
        }
    }

    return status;
}
int8_t get_NV_info()
{
    msg_mqtt_t msg;
    char mac[18];
    db_get_mac(mac);

    msg_rpc_t data_resp;
    uint32_t cmdLen = 0;
    uint8_t *cmd = NULL;
    int8_t status;

    GetNVInfoFormat_t NVInfo;

    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_UTIL), MT_UTIL_GET_NV_INFO, cmd, cmdLen);

    if (status == MT_RPC_SUCCESS)
    {
        //Wait for Sincronic response
        Mailbox_pend(m_rpc_srsp, &data_resp, 5000);

        if (((data_resp.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_UTIL)
        && ((data_resp.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP)
        && (data_resp.data[1] == MT_UTIL_GET_NV_INFO))
        {
            NVInfo.Status = data_resp.data[2];

            NVInfo.IEEEAddr = BUILD_UINT64(data_resp.data[3], data_resp.data[4], data_resp.data[5], data_resp.data[6], data_resp.data[7], data_resp.data[8], data_resp.data[9], data_resp.data[10]);
            NVInfo.ScanChannels = BUILD_UINT32(data_resp.data[11], data_resp.data[12], data_resp.data[13], data_resp.data[14]);
            NVInfo.PanId = BUILD_UINT16(data_resp.data[15], data_resp.data[16]);
            NVInfo.SecurityLevel = data_resp.data[17];
            NVInfo.PreConfigKey[0] = data_resp.data[18];
            NVInfo.PreConfigKey[1] = data_resp.data[19];
            NVInfo.PreConfigKey[2] = data_resp.data[20];
            NVInfo.PreConfigKey[3] = data_resp.data[21];
            NVInfo.PreConfigKey[4] = data_resp.data[22];
            NVInfo.PreConfigKey[5] = data_resp.data[23];
            NVInfo.PreConfigKey[6] = data_resp.data[24];
            NVInfo.PreConfigKey[7] = data_resp.data[25];
            NVInfo.PreConfigKey[8] = data_resp.data[26];
            NVInfo.PreConfigKey[9] = data_resp.data[27];
            NVInfo.PreConfigKey[10] = data_resp.data[28];
            NVInfo.PreConfigKey[11] = data_resp.data[29];
            NVInfo.PreConfigKey[12] = data_resp.data[30];
            NVInfo.PreConfigKey[13] = data_resp.data[31];
            NVInfo.PreConfigKey[14] = data_resp.data[32];
            NVInfo.PreConfigKey[15] = data_resp.data[33];

            //to send to app response
            msg.size_content = 60;
            msg.content = mem_alloc(msg.size_content);
            sprintf(msg.content,
                    "%02X,%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                    NVInfo.SecurityLevel,
                    NVInfo.PreConfigKey[0],
                    NVInfo.PreConfigKey[1],
                    NVInfo.PreConfigKey[2],
                    NVInfo.PreConfigKey[3],
                    NVInfo.PreConfigKey[4],
                    NVInfo.PreConfigKey[5],
                    NVInfo.PreConfigKey[6],
                    NVInfo.PreConfigKey[7],
                    NVInfo.PreConfigKey[8],
                    NVInfo.PreConfigKey[9],
                    NVInfo.PreConfigKey[10],
                    NVInfo.PreConfigKey[11],
                    NVInfo.PreConfigKey[12],
                    NVInfo.PreConfigKey[13],
                    NVInfo.PreConfigKey[14],
                    NVInfo.PreConfigKey[15]
                    );

            mem_free(data_resp.data, data_resp.data_length);

            msg.size_topic = 50;
            msg.topic = malloc(msg.size_topic);
            sprintf(msg.topic, "smaway/%s/tocloud/config/UTIL_GET_NV_INFO\0", mac);

            sendToMqttCheckingMailbox(&msg);
            //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }

    return status;
}


void get_Coordinator_NwkAddr()
{
    msg_rpc_t data_resp;
    uint32_t cmdLen = 0;
    uint8_t *cmd = NULL;
    int8_t status;

    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_UTIL), MT_UTIL_GET_NV_INFO, cmd, cmdLen);

    if (status == MT_RPC_SUCCESS)
    {
        // Esperar respuesta sincr�nica
        if (Mailbox_pend(m_rpc_srsp, &data_resp, 5000))
        {
            if (((data_resp.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_UTIL) &&
                ((data_resp.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP) &&
                (data_resp.data[1] == MT_UTIL_GET_NV_INFO))
            {
                coordinatorNwkAddr = 0x0000; // La direcci�n de red del coordinador siempre es 0x0000 en Zigbee
            }
            mem_free(data_resp.data, data_resp.data_length);
        }
    }
}

int8_t get_nwk_info()
{
    msg_mqtt_t msg;
    char mac[18];
    db_get_mac(mac);

    msg_rpc_t data_resp;
    uint32_t cmdLen = 0;
    uint8_t *cmd = NULL;
    int8_t status;

    ExtNwkInfoSrspFormat_t nwk_info;

    //Send command
    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_ZDO), MT_ZDO_EXT_NWK_INFO, cmd, cmdLen);

    if (status == MT_RPC_SUCCESS)
    {
        //Wait for Sincronic response
        Mailbox_pend(m_rpc_srsp, &data_resp, 5000);

        if (((data_resp.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_ZDO)
        && ((data_resp.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP)
        && (data_resp.data[1] == MT_ZDO_EXT_NWK_INFO))
        {
            nwk_info.ShortAddr = BUILD_UINT16(data_resp.data[2],
                                              data_resp.data[3]);
            nwk_info.DevType = data_resp.data[4];
            nwk_info.PanId = BUILD_UINT16(data_resp.data[5], data_resp.data[6]);
            nwk_info.ParentAddr = BUILD_UINT16(data_resp.data[7],
                                               data_resp.data[8]);

            nwk_info.ExtPanId[7] = data_resp.data[9];
            nwk_info.ExtPanId[6] = data_resp.data[10];
            nwk_info.ExtPanId[5] = data_resp.data[11];
            nwk_info.ExtPanId[4] = data_resp.data[12];
            nwk_info.ExtPanId[3] = data_resp.data[13];
            nwk_info.ExtPanId[2] = data_resp.data[14];
            nwk_info.ExtPanId[1] = data_resp.data[15];
            nwk_info.ExtPanId[0] = data_resp.data[16];

            nwk_info.ExtParentAddr[7] = data_resp.data[17];
            nwk_info.ExtParentAddr[6] = data_resp.data[18];
            nwk_info.ExtParentAddr[5] = data_resp.data[19];
            nwk_info.ExtParentAddr[4] = data_resp.data[20];
            nwk_info.ExtParentAddr[3] = data_resp.data[21];
            nwk_info.ExtParentAddr[2] = data_resp.data[22];
            nwk_info.ExtParentAddr[1] = data_resp.data[23];
            nwk_info.ExtParentAddr[0] = data_resp.data[24];

            nwk_info.CurChannel = data_resp.data[25];

            //to send to app response
            msg.size_content = 160;
            msg.content = mem_alloc(msg.size_content);
            sprintf(msg.content,
                    "%04X,%02X,%04X,%04X,%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X,%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X,%02X",
                    nwk_info.ShortAddr,
                    nwk_info.DevType,
                    nwk_info.PanId,
                    nwk_info.ParentAddr,
                    nwk_info.ExtPanId[0],
                    nwk_info.ExtPanId[1],
                    nwk_info.ExtPanId[2],
                    nwk_info.ExtPanId[3],
                    nwk_info.ExtPanId[4],
                    nwk_info.ExtPanId[5],
                    nwk_info.ExtPanId[6],
                    nwk_info.ExtPanId[7],
                    nwk_info.ExtParentAddr[0],
                    nwk_info.ExtParentAddr[1],
                    nwk_info.ExtParentAddr[2],
                    nwk_info.ExtParentAddr[3],
                    nwk_info.ExtParentAddr[4],
                    nwk_info.ExtParentAddr[5],
                    nwk_info.ExtParentAddr[6],
                    nwk_info.ExtParentAddr[7],
                    nwk_info.CurChannel);

            mem_free(data_resp.data, data_resp.data_length);

            msg.size_topic = 50;
            msg.topic = malloc(msg.size_topic);
            sprintf(msg.topic, "smaway/%s/tocloud/config/NetworkInfoReq\0", mac);

            sendToMqttCheckingMailbox(&msg);
            //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }

    return status;
}
int8_t start_commisioning()
{
    uint8_t status;
    uint8_t cmInd = 0;
    uint32_t cmdLen = 1;
    uint8_t *cmd = malloc(cmdLen);

    if (cmd)
    {

        cmd[cmInd++] = 0x02;

        status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_APP), 0x05, cmd,
                              cmdLen);

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
int8_t sys_ping()
{
    uint8_t status;
    msg_rpc_t msg = { 0 };
    msg_mqtt_t msg_mqtt;
    char mac[18];
    db_get_mac(mac);

    status = rpcSendFrame((MT_RPC_CMD_SREQ | MT_RPC_SYS_SYS), MT_SYS_PING, NULL, 0);

    if (status == MT_RPC_SUCCESS)
    {
        Mailbox_pend(m_rpc_srsp, &msg, 5000);

        if (((msg.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
        && ((msg.data[0] & MT_RPC_CMD_TYPE_MASK) == MT_RPC_CMD_SRSP)
        && (msg.data[1] == MT_SYS_PING))
        {
            msg_mqtt.size_topic = 45;
            msg_mqtt.topic = mem_alloc(msg_mqtt.size_topic);
            sprintf(msg_mqtt.topic, "smaway/%s/tocloud/config/SYS_PING", mac);

            msg_mqtt.size_content = 13;
            msg_mqtt.content = mem_alloc(msg_mqtt.size_content);
            sprintf(msg_mqtt.content, "0x%02X,0x%02X", msg.data[2], msg.data[2 + 1]);

            msg_mqtt.size_buf = 0;
            msg_mqtt.buf = NULL;

            mem_free(msg.data, msg.data_length);

            sendToMqttCheckingMailbox(&msg_mqtt);
            //Mailbox_post(m_mqtt_send, &msg_mqtt, BIOS_WAIT_FOREVER);
        }
        else
        {
            //TODO LOG dbg_print(PRINT_LEVEL_INFO, "rpcWaitMqClientMsg: Timed out [%d] - %s\n",rpcLen, strerror(errno));
            return -1;
        }
    }
    return status;
}
int8_t nwk_remove_device_req(uint64_t ieee)
{
    ExtSecApsRemoveReqFormat_t req;
    dbt_zdevice *zdev = NULL, *zdev_el = NULL;

    zdev = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->ieee_address, false, ZDEVICE, NUMBER, 0, ieee, "", N_UINT64);
    zdev_el = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->ieee_address, false, ZDEVICE, NUMBER, 0, zdev->parent_address, "", N_UINT64);
    if (zdev && zdev_el)
    {
        req.NWKAddr = zdev->nwk_address;

        req.IEEEaddr[0] = BREAK_UINT64(zdev->ieee_address, 0);
        req.IEEEaddr[1] = BREAK_UINT64(zdev->ieee_address, 1);
        req.IEEEaddr[2] = BREAK_UINT64(zdev->ieee_address, 2);
        req.IEEEaddr[3] = BREAK_UINT64(zdev->ieee_address, 3);
        req.IEEEaddr[4] = BREAK_UINT64(zdev->ieee_address, 4);
        req.IEEEaddr[5] = BREAK_UINT64(zdev->ieee_address, 5);
        req.IEEEaddr[6] = BREAK_UINT64(zdev->ieee_address, 6);
        req.IEEEaddr[7] = BREAK_UINT64(zdev->ieee_address, 7);

        req.parent_NWKAddr = zdev_el->nwk_address;

        zdo_ext_sec_aps_remove_req(&req);

        mem_free(zdev_el, sizeof(dbt_zdevice));
        mem_free(zdev, sizeof(dbt_zdevice));

        return 0;
    }
    else
    {
        return -1;
    }

}
int8_t nwk_set_binding_entry_req(dbt_zdevice *zdev_1, int16_t endpoint, int16_t cluster)
{
    int8_t status;

    BindReqFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "",
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "");

    req.DstAddr = zdev_1->nwk_address;
    req.SrcAddress = zdev_1->ieee_address;
    req.SrcEndpoint = zendpoint->zendpoint_id;
    req.DstAddrMode = 0x03;
    req.ClusterID = cluster;
    req.DstAddress = zdev_1->parent_address;
    printf("binding : 0x%08X", zdev_1->parent_address);
    req.DstEndpoint = zendpoint->id;

    mem_free(zendpoint, sizeof(dbt_zendpoint));

    status = zdo_bind_req(&req);
    Task_sleep(200);
    return status;
}
int8_t nwk_set_unbinding_entry_req(dbt_zdevice *zdev_1, int16_t endpoint,
                                   int16_t cluster)
{
    int8_t status;

    UnbindReqFormat_t req;

    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "",
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "");

    req.DstAddr = zdev_1->nwk_address;
    req.SrcAddress = zdev_1->ieee_address;
    req.SrcEndpoint = zendpoint->zendpoint_id;
    req.DstAddrMode = 0x03;
    req.ClusterID = cluster;
    req.DstAddress = zdev_1->parent_address;
    req.DstEndpoint = zendpoint->id;

    mem_free(zendpoint, sizeof(dbt_zendpoint));

    status = zdo_unbind_req(&req);

    return status;
}
int8_t factory_reset(uint8_t channel)
{
    int8_t status;
    status = set_NV_startup(3);
    if (status)
    {
        return status;
    }
    Task_sleep(200);

    status = sys_reset(0);
    if (status)
    {
        return status;
    }
    Task_sleep(200);

    status = appCnfBDBsetChannel(1, channel);
    if (status)
    {
        return status;
    }
    Task_sleep(200);

    status = appCnfBDBsetChannel(0, 0);
    if (status)
    {
        return status;
    }
    Task_sleep(200);

    status = appCnfBDBStartCommisioning(0x04);
    if (status)
    {
        return status;
    }
    Task_sleep(200);

    status = set_zdo_direct_cb();
    if (status)
    {
        return status;
    }
    Task_sleep(200);

    return 0;
}

int8_t get_zdevice_firmware_version(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");
    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0000;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID - read Attributes
    req.Data[3] = 0x01; //AttributeID
    req.Data[4] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t get_zdevice_model_id(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");
    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0000;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID
    req.Data[3] = 0x05; //AttributeID
    req.Data[4] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t get_zdevice_power_source(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");
    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0000;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 6;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x01; //ComandID
    req.Data[3] = 0x07; //AttributeID
    req.Data[4] = 0x00; //AttributeID
    req.Data[5] = 0x00; //DataType

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}

int8_t get_zdevice_onoff(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");
    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0006;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID
    req.Data[3] = 0x00; //AttributeID
    req.Data[4] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_zdevice_onoff(dbt_zdevice *zdev_1, uint8_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0006;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 3;
    req.Data[0] = 0x01; //Frame Control.. Comand is specific to a Cluster
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = value; //Value

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t get_zdevice_onoff2(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0002, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0006;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID
    req.Data[3] = 0x00; //AttributeID
    req.Data[4] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_zdevice_onoff2(dbt_zdevice *zdev_1, uint8_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0002, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0006;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 3;
    req.Data[0] = 0x01; //Frame Control.. Comand is specific to a Cluster
    req.Data[0] = 0x00; //TransactionSeqNumber
    req.Data[0] = value; //Value

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t get_zdevice_level_control(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0002, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0008;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //ComandID
    req.Data[3] = 0x00; //AttributeID
    req.Data[4] = 0x00; //AttributeID

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_zdevice_level_control(dbt_zdevice *zdev_1, uint16_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0002, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0008;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 6;
    req.Data[0] = 0x01; //Frame Control.. Comand is specific to a Cluster
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //CommandID Specific
    req.Data[3] = 0x01; //Level
    req.Data[4] = LO_UINT16(value); //TrnasitionTime
    req.Data[5] = HI_UINT16(value);

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t get_local_temperature(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x00; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH
    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t get_target_temperature(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x12; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_target_temperature(dbt_zdevice *zdev_1, int16_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 8;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x02; //Command_ID
    req.Data[3] = 0x12; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH
    req.Data[5] = 0x29; //Data Type
    req.Data[6] = LO_UINT16(value); //Value LOW
    req.Data[7] = HI_UINT16(value); //Value HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afResetResponseSemaphore();

    afDataRequestExt(&req);

    return afPendResponseSemaphore();
}
int8_t get_lqi_value(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0B05;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x1C; //Attribute_ID LOW
    req.Data[4] = 0x01; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t get_battery_percent(dbt_zdevice *zdev_1)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0001;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 5;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x00; //Command_ID
    req.Data[3] = 0x21; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_system_mode(dbt_zdevice *zdev_1, uint8_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, 0x0001, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0x0201;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 7;
    req.Data[0] = 0x00; //Frame Control
    req.Data[1] = 0x00; //TransactionSeqNumber
    req.Data[2] = 0x02; //Command_ID
    req.Data[3] = 0x1C; //Attribute_ID LOW
    req.Data[4] = 0x00; //Attribute_ID HIGH
    req.Data[5] = 0x30; //Data Type
    req.Data[6] = value; //Value LOW

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afResetResponseSemaphore();

    afDataRequestExt(&req);

    return afPendResponseSemaphore();
}

int8_t set_water_scale_proof(dbt_zdevice *zdev_1, int16_t endpoint, uint8_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 9;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber

    req.Data[4] = 0x02; //Command_ID
    req.Data[5] = 0x00; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH
    req.Data[7] = 0x10; //Data Type
    req.Data[8] = value; //Value LOW

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_frost_enabled(dbt_zdevice *zdev_1, int16_t endpoint, uint8_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 9;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x02; //Command_ID
    req.Data[5] = 0x01; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH
    req.Data[7] = 0x10; //Data Type
    req.Data[8] = value; //Value LOW

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_window_check_enabled(dbt_zdevice *zdev_1, int16_t endpoint, uint8_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 9;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x02; //Command_ID
    req.Data[5] = 0x02; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH
    req.Data[7] = 0x10; //Data Type
    req.Data[8] = value; //Value LOW

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_child_lock_enabled(dbt_zdevice *zdev_1, int16_t endpoint, uint8_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 9;
    req.Data[0] = 0x04; //Frame Control

    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH

    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x02; //Command_ID
    req.Data[5] = 0x03; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH
    req.Data[7] = 0x10; //Data Type
    req.Data[8] = value; //Value LOW

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}
int8_t set_key_away_enabled(dbt_zdevice *zdev_1, int16_t endpoint, uint8_t value)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    zendpoint = (dbt_zendpoint*) find_element_and((dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
                                                  (char**) &config_db.db_zendpoint->zdevice_id,
                                                  NUMBER, 0, zdev_1->id, "",
                                                  (char**) &config_db.db_zendpoint->zendpoint_id,
                                                  NUMBER, 0, endpoint, "");

    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    req.DstAddr[2] = 0;
    req.DstAddr[3] = 0;
    req.DstAddr[4] = 0;
    req.DstAddr[5] = 0;
    req.DstAddr[6] = 0;
    req.DstAddr[7] = 0;

    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;
    req.ClusterId = 0xFC06;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;
    req.Len = 9;
    req.Data[0] = 0x04; //Frame Control
    req.Data[1] = 0x02; //Manufacterer Code LOW
    req.Data[2] = 0x10; //Manufacterer Code HIGH
    req.Data[3] = 0x00; //TransactionSeqNumber
    req.Data[4] = 0x02; //Command_ID
    req.Data[5] = 0x04; //Attribute_ID LOW
    req.Data[6] = 0x00; //Attribute_ID HIGH
    req.Data[7] = 0x10; //Data Type
    req.Data[8] = value; //Value LOW

    mem_free(zendpoint, sizeof(dbt_zendpoint));
    afDataRequestExt(&req);

    return 0;
}

int8_t read_onoff(dbt_zdevice *zdev_1, uint16_t endpoint_id)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    // Buscar el endpoint en la base de datos
    zendpoint = (dbt_zendpoint*) find_element_and(
        (dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
        (char**) &config_db.db_zendpoint->zdevice_id, NUMBER, 0, zdev_1->id, "",
        (char**) &config_db.db_zendpoint->zendpoint_id, NUMBER, 0, endpoint_id, ""
    );

    if (zendpoint == NULL) {
        return -1; // No se encontr� el endpoint
    }

    // Configurar la direcci�n de destino
    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    memset(&req.DstAddr[2], 0, 6);

    // Configurar los endpoints
    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;

    // Configurar los par�metros Zigbee
    req.ClusterId = 0x0006;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;

    // Cargar datos del comando Read Attributes
    req.Len = 5;
    req.Data[0] = 0x00;  // Frame Control
    req.Data[1] = 0x00;  // Transaction Sequence Number
    req.Data[2] = 0x00;  // Command ID (Read Attribute)
    req.Data[3] = 0x00;  // Attribute ID LSB (OnOff)
    req.Data[4] = 0x00;  // Attribute ID MSB (OnOff)

    // Enviar el comando Zigbee
    afDataRequestExt(&req);

    // Liberar memoria despu�s de usar el endpoint
    mem_free(zendpoint, sizeof(dbt_zendpoint));

    return 0;
}

int8_t read_binary_input(dbt_zdevice *zdev_1, uint16_t endpoint_id) {
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    // Buscar el endpoint en la base de datos
    zendpoint = (dbt_zendpoint*) find_element_and(
        (dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
        (char**) &config_db.db_zendpoint->zdevice_id, NUMBER, 0, zdev_1->id, "",
        (char**) &config_db.db_zendpoint->zendpoint_id, NUMBER, 0, endpoint_id, ""
    );

    if (zendpoint == NULL) {
        return -1; // No se encontr� el endpoint
    }

    // Configurar la direcci�n de destino
    req.DstAddrMode = 0x02; // Direcci�n de 16 bits
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    memset(&req.DstAddr[2], 0, 6); // Inicializar el resto de la direcci�n

    // Configurar los endpoints
    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0; // PAN ID del destino
    req.SrcEndpoint = zendpoint->id;

    // Configurar los par�metros Zigbee
    req.ClusterId = 0x000F; // Cluster ID para Binary Input
    req.TransId = 0x00; // ID de transacci�n
    req.Options = 0x00; // Opciones
    req.Radius = 0x1E; // Radio de transmisi�n

    // Cargar datos del comando Read Attributes
    req.Len = 5;
    req.Data[0] = 0x00;  // Control de Frame
    req.Data[1] = 0x00;  // Secuencia de Transacci�n
    req.Data[2] = 0x00;  // Command ID (Read Attribute)
    req.Data[3] = LO_UINT16(0x0055);  // Attribute ID LSB (PresentValue)
    req.Data[4] = HI_UINT16(0x0055);  // Attribute ID MSB (PresentValue)

    // Enviar el comando Zigbee
    afDataRequestExt(&req);

    // Liberar memoria despu�s de usar el endpoint
    mem_free(zendpoint, sizeof(dbt_zendpoint));

    return 0;
}


int8_t write_onoff(dbt_zdevice *zdev_1, uint16_t endpoint_id, uint8_t state)
{
    DataRequestExtFormat_t req;
    dbt_zendpoint *zendpoint = NULL;

    // Buscar el endpoint en la base de datos
    zendpoint = (dbt_zendpoint*) find_element_and(
        (dbt_generic**) &config_db.db_zendpoint, false, ZENDPOINT,
        (char**) &config_db.db_zendpoint->zdevice_id, NUMBER, 0, zdev_1->id, "",
        (char**) &config_db.db_zendpoint->zendpoint_id, NUMBER, 0, endpoint_id, ""
    );

    if (zendpoint == NULL) {
        return -1; // No se encontr� el endpoint
    }

    // Configurar la direcci�n de destino
    req.DstAddrMode = 0x02;
    req.DstAddr[0] = LO_UINT16(zdev_1->nwk_address);
    req.DstAddr[1] = HI_UINT16(zdev_1->nwk_address);
    memset(&req.DstAddr[2], 0, 6);

    // Configurar los endpoints
    req.DstEndpoint = zendpoint->zendpoint_id;
    req.DstPanID = 0;
    req.SrcEndpoint = zendpoint->id;

    // Configurar los par�metros Zigbee
    req.ClusterId = 0x0006;
    req.TransId = 0x00;
    req.Options = 0x00;
    req.Radius = 0x1E;

    // Cargar datos del comando Zigbee
    req.Len = 3;
    req.Data[0] = 0x01;  // Frame Control
    req.Data[1] = 0x00;  // Transaction Sequence Number
    req.Data[2] = state; // Command ID: 0x01 (Off), 0x02 (On), 0x03 (Toggle)

    // Enviar el comando Zigbee
    afDataRequestExt(&req);

    // Liberar memoria despu�s de usar el endpoint
    mem_free(zendpoint, sizeof(dbt_zendpoint));

    return 0;
}


int8_t binding_clusters(uint16_t NwkAddr)
{
    dbt_zdevice *zdev = NULL;

    zdev = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->nwk_address, false, ZDEVICE, NUMBER, 0, NwkAddr, "", N_UINT16);

    //1
    if (strcmp((char*) zdev->model_id, "PSB00003") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
    }
    //2
    else if (strcmp((char*) zdev->model_id, "UPSB0001") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
    }
    //3
    else if (strcmp((char*) zdev->model_id, "PSMB0004") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0702);
    }
    //4
    else if (strcmp((char*) zdev->model_id, "PDB00001") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0008);
    }
    //5
    else if (strcmp((char*) zdev->model_id, "UPDB0001") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0008);
    }
    //6
    else if (strcmp((char*) zdev->model_id, "THERMB01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0201);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
    }
    //7
    else if (strcmp((char*) zdev->model_id, "HTHERMB1") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0000);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0201);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
        nwk_set_binding_entry_req(zdev, 0x01, 0xFC06);
    }
    //8
    else if (strcmp((char*) zdev->model_id, "HTHERMB2") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0000);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0201);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
        nwk_set_binding_entry_req(zdev, 0x01, 0xFC06);
    }
    //9
    else if (strcmp((char*) zdev->model_id, "FKABB002") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
    }
    //10
    else if (strcmp((char*) zdev->model_id, "FKABB003") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
    }
    //11
    else if (strcmp((char*) zdev->model_id, "FKABB001") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
    }
    //12
    else if (strcmp((char*) zdev->model_id, "BWMWSV01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
    }
    //13
    else if (strcmp((char*) zdev->model_id, "BWMWSB01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
    }
    //14
    else if (strcmp((char*) zdev->model_id, "BWMWSB02") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
    }
    //15
    else if (strcmp((char*) zdev->model_id, "GLASB001") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
    }
    //16
    else if (strcmp((char*) zdev->model_id, "WMWSB001") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
    }
    //17
    else if (strcmp((char*) zdev->model_id, "RWMSIB01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0502);
    }
    //18
    else if (strcmp((char*) zdev->model_id, "SIRENB01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0500);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0502);
    }
    //19
    else if (strcmp((char*) zdev->model_id, "UPSPFMB1") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
    }
    //20
    else if (strcmp((char*) zdev->model_id, "UPSMB001") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0702);
        nwk_set_binding_entry_req(zdev, 0x01, 0xFCC4);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
    }
    //21
    else if (strcmp((char*) zdev->model_id, "UPSDMB01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0702);
        nwk_set_binding_entry_req(zdev, 0x01, 0xFCC4);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
    }
    //22
    else if (strcmp((char*) zdev->model_id, "LUXSENB1") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0400);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0402);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0001);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
    }
    //23
    else if (strcmp((char*) zdev->model_id, "UPRSB001") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0102);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
    }
    //24
    else if (strcmp((char*) zdev->model_id, "UPS2MB01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x02, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x02, 0x0702);
        nwk_set_binding_entry_req(zdev, 0x02, 0xFCC4);
        nwk_set_binding_entry_req(zdev, 0x02, 0x0B05);

        nwk_set_binding_entry_req(zdev, 0x01, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0702);
        nwk_set_binding_entry_req(zdev, 0x01, 0xFCC4);
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
    }
    //25
    else if (strcmp((char*) zdev->model_id, "FSREPB01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x01, 0x0B05);
    }
    //26        HYDRONIC
    else if (strcmp((char*) zdev->model_id, "1211/01") == 0)
    {
        nwk_set_binding_entry_req(zdev, 0x02, 0x000F);
        nwk_set_binding_entry_req(zdev, 0x03, 0x000F);
        nwk_set_binding_entry_req(zdev, 0x04, 0x000F);
        nwk_set_binding_entry_req(zdev, 0x05, 0x000F);
        /*nwk_set_binding_entry_req(zdev, 0x03, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x04, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x05, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x06, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x07, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x08, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x09, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x0a, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x0b, 0x0006);
        nwk_set_binding_entry_req(zdev, 0x0c, 0x0006);*/
    }
    else
    {
        mem_free(zdev, sizeof(dbt_zdevice));
        return -1;
    }
    mem_free(zdev, sizeof(dbt_zdevice));
    return 0;
}

int8_t configure_device(uint16_t NwkAddr)
{
    uint8_t nSteps = 0;
    dbt_zdevice *zdev = NULL;
    uint32_t pendingSteps = 0;

    zdev = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->nwk_address, false, ZDEVICE, NUMBER, 0, NwkAddr, "", N_UINT16);

    //1
    if (strcmp((char*) zdev->model_id, "PSB00003") == 0)
    {
    }
    //2
    else if (strcmp((char*) zdev->model_id, "UPSB0001") == 0)
    {
    }
    //3
    else if (strcmp((char*) zdev->model_id, "PSMB0004") == 0)
    {
    }
    //4
    else if (strcmp((char*) zdev->model_id, "PDB00001") == 0)
    {
    }
    //5
    else if (strcmp((char*) zdev->model_id, "UPDB0001") == 0)
    {
    }
    //6
    else if (strcmp((char*) zdev->model_id, "THERMB01") == 0)
    {
        uint8_t iStep;
        nSteps          = THERMB01_NSTEPS;    //TODO: Remove hard-coded number
        pendingSteps    = (((uint32_t)1)<<nSteps) - 1;  //To get nSteps bits in 1
        uint8_t iRetry  = 0;
        Task_sleep(5000);

        while(pendingSteps && iRetry<CONFIG_DEVICE_NRETRY)
        {
            for (iStep = 0; iStep < nSteps; iStep++) {
                if((pendingSteps>>iStep) & 0x001)
                {
                    if(runConfigSteps_THERMB01(zdev, iStep+1))    //Added +1 to match index with step number
                        pendingSteps &= ~(0x001 << iStep);
                    else
                        printf("configure_device: Missed configuring step %d, retry attempt %d", iStep, iRetry);
                }
            }
            iRetry++;
        }

        if(pendingSteps)
        {
            mem_free(zdev, sizeof(dbt_zdevice));
            return -1;
        }
    }
//7
    else if (strcmp((char*) zdev->model_id, "HTHERMB1") == 0)
    {
        uint8_t iStep;
        nSteps          = HTHERMB1_NSTEPS;    //TODO: Remove hard-coded number
        pendingSteps    = (((uint32_t)1)<<nSteps) - 1;  //To get nSteps bits in 1
        uint8_t iRetry  = 0;
        //Task_sleep(5000);

        while(pendingSteps && iRetry<CONFIG_DEVICE_NRETRY)
        {
            for (iStep = 0; iStep < nSteps; iStep++) {
                if((pendingSteps>>iStep) & 0x001)
                {
                    if(runConfigSteps_HTHERMB1(zdev, iStep+1))    //Added +1 to match index with step number
                        pendingSteps &= ~(0x001 << iStep);
                    else
                        printf("configure_device: Missed configuring step %d, retry attempt %d", iStep, iRetry);
                }
            }
            iRetry++;
        }

        if(pendingSteps)
        {
            mem_free(zdev, sizeof(dbt_zdevice));
            return -1;
        }
    }
//8
    else if (strcmp((char*) zdev->model_id, "HTHERMB2") == 0)
    {
    }
//9
    else if (strcmp((char*) zdev->model_id, "FKABB002") == 0)
    {
    }
//10
    else if (strcmp((char*) zdev->model_id, "FKABB003") == 0)
    {
    }
//11
    else if (strcmp((char*) zdev->model_id, "FKABB001") == 0)
    {
    }
//12
    else if (strcmp((char*) zdev->model_id, "BWMWSV01") == 0)
    {
    }
//13
    else if (strcmp((char*) zdev->model_id, "BWMWSB01") == 0)
    {
    }
//14
    else if (strcmp((char*) zdev->model_id, "BWMWSB02") == 0)
    {
    }
//15
    else if (strcmp((char*) zdev->model_id, "GLASB001") == 0)
    {
    }
//16
    else if (strcmp((char*) zdev->model_id, "WMWSB001") == 0)
    {
    }
//17
    else if (strcmp((char*) zdev->model_id, "RWMSIB01") == 0)
    {
    }
//18
    else if (strcmp((char*) zdev->model_id, "SIRENB01") == 0)
    {
    }
//19
    else if (strcmp((char*) zdev->model_id, "UPSPFMB1") == 0)
    {
    }
//20
    else if (strcmp((char*) zdev->model_id, "UPSMB001") == 0)
    {
    }
//21
    else if (strcmp((char*) zdev->model_id, "UPSDMB01") == 0)
    {
    }
//22
    else if (strcmp((char*) zdev->model_id, "LUXSENB1") == 0)
    {
    }
//23
    else if (strcmp((char*) zdev->model_id, "UPRSB001") == 0)
    {
    }
//24
    else if (strcmp((char*) zdev->model_id, "UPS2MB01") == 0)
    {
    }
//25
    else if (strcmp((char*) zdev->model_id, "FSREPB01") == 0)
    {
    }
    //26        HYDRONIC
    else if (strcmp((char*) zdev->model_id, "1211/01") == 0)
    {
        configurar_Reporte_BinaryInput(zdev, 0x0002, 0, 600);
        configurar_Reporte_BinaryInput(zdev, 0x0003, 0, 600);
        configurar_Reporte_BinaryInput(zdev, 0x0004, 0, 600);
        configurar_Reporte_BinaryInput(zdev, 0x0005, 0, 600);
        /*configurar_Reporte_OnOff(zdev, 0x0003, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x0004, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x0005, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x0006, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x0007, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x0008, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x0009, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x000a, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x000b, 0, 600);
        configurar_Reporte_OnOff(zdev, 0x000c, 0, 600);*/
        read_onoff(zdev, 0x0002);
        read_onoff(zdev, 0x0003);
        read_onoff(zdev, 0x0004);
        read_onoff(zdev, 0x0005);
        read_onoff(zdev, 0x0006);
        read_onoff(zdev, 0x0007);
        read_onoff(zdev, 0x0008);
        read_onoff(zdev, 0x0009);
        read_onoff(zdev, 0x000a);
        read_onoff(zdev, 0x000b);
        read_onoff(zdev, 0x000c);
        read_binary_input(zdev, 0x0002);
        read_binary_input(zdev, 0x0003);
        read_binary_input(zdev, 0x0004);
        read_binary_input(zdev, 0x0005);
        /*read_onoff(zdev, 0x000d);
        read_onoff(zdev, 0x000e);
        read_onoff(zdev, 0x000f);
        read_onoff(zdev, 0x0010);*/
    }
    else
    {
        mem_free(zdev, sizeof(dbt_zdevice));
        return -1;
    }
    mem_free(zdev, sizeof(dbt_zdevice));
    return 0;
}

int8_t request_status()
{
    void *r = NULL;
    dbt_zdevice *zdev = NULL;
    zdev = (dbt_zdevice*) get_element((dbt_generic**) &config_db.db_zdevice, ZDEVICE);

    while(zdev)
    {
        //1
        if (strcmp((char*) zdev->model_id, "PSB00003") == 0)
        {
        }
        //2
        else if (strcmp((char*) zdev->model_id, "UPSB0001") == 0)
        {
        }
        //3
        else if (strcmp((char*) zdev->model_id, "PSMB0004") == 0)
        {
        }
        //4
        else if (strcmp((char*) zdev->model_id, "PDB00001") == 0)
        {
        }
        //5
        else if (strcmp((char*) zdev->model_id, "UPDB0001") == 0)
        {
        }
        //6
        else if (strcmp((char*) zdev->model_id, "THERMB01") == 0)
        {
            read_lqi_value(zdev);
            read_battery_low(zdev);
            read_local_temperature(zdev);
            read_target_temperature(zdev);
            read_system_mode(zdev);
        }
        //7
        else if (strcmp((char*) zdev->model_id, "HTHERMB1") == 0)
        {
            read_power_source(zdev);
            read_lqi_value(zdev);
            read_battery_low(zdev);
            read_battery_percent(zdev);
            read_local_temperature(zdev);
            read_target_temperature(zdev);
            read_system_mode(zdev);
            read_water_scale_proof(zdev, 0x0001);
            read_frost_enabled(zdev, 0x0001);
            read_window_check_enabled(zdev, 0x0001);
            read_child_lock_enabled(zdev, 0x0001);
            read_key_away_enabled(zdev, 0x0001);
            read_motor_opening_size(zdev, 0x0001);
        }
        //8
        else if (strcmp((char*) zdev->model_id, "HTHERMB2") == 0)
        {
        }
        //9
        else if (strcmp((char*) zdev->model_id, "FKABB002") == 0)
        {
        }
        //10
        else if (strcmp((char*) zdev->model_id, "FKABB003") == 0)
        {
        }
        //11
        else if (strcmp((char*) zdev->model_id, "FKABB001") == 0)
        {
        }
        //12
        else if (strcmp((char*) zdev->model_id, "BWMWSV01") == 0)
        {
        }
        //13
        else if (strcmp((char*) zdev->model_id, "BWMWSB01") == 0)
        {
        }
        //14
        else if (strcmp((char*) zdev->model_id, "BWMWSB02") == 0)
        {
        }
        //15
        else if (strcmp((char*) zdev->model_id, "GLASB001") == 0)
        {
        }
        //16
        else if (strcmp((char*) zdev->model_id, "WMWSB001") == 0)
        {
        }
        //17
        else if (strcmp((char*) zdev->model_id, "RWMSIB01") == 0)
        {
        }
        //18
        else if (strcmp((char*) zdev->model_id, "SIRENB01") == 0)
        {
        }
        //19
        else if (strcmp((char*) zdev->model_id, "UPSPFMB1") == 0)
        {
        }
        //20
        else if (strcmp((char*) zdev->model_id, "UPSMB001") == 0)
        {
        }
        //21
        else if (strcmp((char*) zdev->model_id, "UPSDMB01") == 0)
        {
        }
        //22
        else if (strcmp((char*) zdev->model_id, "LUXSENB1") == 0)
        {
        }
        //23
        else if (strcmp((char*) zdev->model_id, "UPRSB001") == 0)
        {
        }
        //24
        else if (strcmp((char*) zdev->model_id, "UPS2MB01") == 0)
        {
        }
        //25
        else if (strcmp((char*) zdev->model_id, "FSREPB01") == 0)
        {
        }
        //26        HYDRONIC
        else if (strcmp((char*) zdev->model_id, "1211/01") == 0)
        {
            read_onoff(zdev, 0x0002);
            read_onoff(zdev, 0x0003);
            read_onoff(zdev, 0x0004);
            read_onoff(zdev, 0x0005);
            read_onoff(zdev, 0x0006);
            read_onoff(zdev, 0x0007);
            read_onoff(zdev, 0x0008);
            read_onoff(zdev, 0x0009);
            read_onoff(zdev, 0x000a);
            read_onoff(zdev, 0x000b);
            read_onoff(zdev, 0x000c);
            read_binary_input(zdev, 0x0002);
            read_binary_input(zdev, 0x0003);
            read_binary_input(zdev, 0x0004);
            read_binary_input(zdev, 0x0005);
            /*read_onoff(zdev, 0x000d);
            read_onoff(zdev, 0x000e);
            read_onoff(zdev, 0x000f);
            read_onoff(zdev, 0x0010);*/
        }
        r = zdev->element->next;
        mem_free(zdev, sizeof(dbt_zdevice));
        zdev = (dbt_zdevice*) get_element((dbt_generic**) &r, ZENDPOINT);
    }
    return 0;
}

void add_zdevice(char *model, uint16_t model_size)
{
    msg_rpc_t msg_app;
    msg_mqtt_t msg_mqtt;
    char ieee_address[24];
    char lm[70];

    uint8_t i = 0, j = 0;
    int8_t status = 0;
    dbt_zdevice *zdevice = NULL;
    dbt_zdevice *zdev_parent = NULL;
    dbt_zendpoint *zendpoint = NULL;
    TcDevIndFormat_t *TcDevIndRsp = NULL;
    NodeDescRspFormat_t *NodeDescRsp = NULL;
    ActiveEpRspFormat_t *ActiveEpRsp = NULL;
    SimpleDescRspFormat_t *SimpleDescRsp = NULL;
    RegisterFormat_t Register;

    uint16_t nwk_address = 0;
    uint16_t parent_address = 0;

    char mac[18];
    db_get_mac(mac);

    // Wait for pairing message
    printf("Waiting for pairing message...\n");
    status = Mailbox_pend(m_add_device, &msg_app, 60000);

    if (!status)
    {
        printf("Error: Timeout waiting for pairing\n");
        logevent(L_ERROR, "zigbee - pairing mode timeout");
        zdo_permit_join_req(0);
        return;
    }

    printf("Pairing message received\n");
    Task_sleep(2000);

    TcDevIndRsp = (TcDevIndFormat_t*) msg_app.data;

    // Log device detection
    printf("Device detected: IEEEAddr = 0x%llX, SrcNwkAddr = 0x%04X, ParentNwkAddr = 0x%04X\n",
           TcDevIndRsp->IEEEAddr, TcDevIndRsp->SrcNwkAddr, TcDevIndRsp->ParentNwkAddr);

    // Check if device already exists in DB
    zdevice = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice,
                                          (char**) &config_db.db_zdevice->ieee_address,
                                          false, ZDEVICE, NUMBER, 0, TcDevIndRsp->IEEEAddr, "", N_UINT64);

    if (zdevice == NULL)
    {
        // Add new device to DB
        printf("New device. Adding to database...\n");
        zdevice = (dbt_zdevice*) db_add_item(ZDEVICE,
                                             (dbt_generic**) &config_db.db_zdevice,
                                             (dbt_generic**) &config_db.db_zdevice_last);
    }
    else
    {
        printf("Device already exists in database\n");
    }

    // Find parent device in DB
    zdev_parent = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice,
                                              (char**) &config_db.db_zdevice->nwk_address,
                                              false, ZDEVICE, NUMBER, 0, TcDevIndRsp->ParentNwkAddr, "", N_UINT16);

    // Update device info
    zdevice->ieee_address = TcDevIndRsp->IEEEAddr;
    zdevice->nwk_address = TcDevIndRsp->SrcNwkAddr;
    nwk_address = TcDevIndRsp->SrcNwkAddr;
    parent_address = TcDevIndRsp->ParentNwkAddr;

    if (zdev_parent != NULL)
    {
        zdevice->parent_address = zdev_parent->ieee_address;
        mem_free(zdev_parent, sizeof(dbt_zdevice));
    }

    mem_free(TcDevIndRsp, sizeof(TcDevIndFormat_t));

    // Request node descriptor
    printf("Requesting node descriptor from device 0x%04X...\n", zdevice->nwk_address);
    node_desc_req(zdevice->nwk_address);
    status = Mailbox_pend(m_add_device, &msg_app, 5000);
    if (!status)
    {
        printf("Timeout waiting for node descriptor\n");
        zdo_permit_join_req(0);
        return;
    }

    NodeDescRsp = (NodeDescRspFormat_t*) msg_app.data;
    printf("Descriptor received: capabilities = 0x%02X\n", NodeDescRsp->DescriptorCapabilities);
    zdevice->cap_info = NodeDescRsp->DescriptorCapabilities;
    mem_free(NodeDescRsp, msg_app.data_length);

    // Request list of active endpoints
    printf("Requesting active endpoint list from device 0x%04X...\n", zdevice->nwk_address);
    active_ep_req(zdevice->nwk_address);
    status = Mailbox_pend(m_add_device, &msg_app, 5000);
    if (!status)
    {
        printf("Timeout waiting for active endpoints\n");
        zdo_permit_join_req(0);
        return;
    }

    ActiveEpRsp = (ActiveEpRspFormat_t*) msg_app.data;
    zdevice->ep_count = ActiveEpRsp->ActiveEPCount;
    printf("%d active endpoints found on the device\n", zdevice->ep_count);

    // Iterate over endpoints and request simple descriptors
    for (i = 0; i < zdevice->ep_count; i++)
    {
        Task_sleep(500);
        uint8_t ep = ActiveEpRsp->ActiveEPList[i];
        printf("Requesting simple descriptor for endpoint %d (0x%02X)...\n", ep, ep);
        simple_desc_req(zdevice->nwk_address, ep);

        status = Mailbox_pend(m_add_device, &msg_app, 5000);
        if (!status)
        {
            printf("Timeout waiting for simple descriptor for endpoint %d\n", ep);
            continue;
        }

        SimpleDescRsp = (SimpleDescRspFormat_t*) msg_app.data;
        printf("Simple descriptor received - ProfileID: 0x%04X, DeviceID: 0x%04X, Version: %d\n",
               SimpleDescRsp->ProfileID, SimpleDescRsp->DeviceID, SimpleDescRsp->DeviceVersion);

        // Add endpoint to database
        zendpoint = (dbt_zendpoint*) db_add_item(ZENDPOINT,
                                                 (dbt_generic**) &config_db.db_zendpoint,
                                                 (dbt_generic**) &config_db.db_zendpoint_last);

        zendpoint->zdevice_id = zdevice->id;
        zendpoint->zendpoint_id = ep;
        zendpoint->profile_id = SimpleDescRsp->ProfileID;
        zendpoint->device_id = SimpleDescRsp->DeviceID;
        zendpoint->device_version = SimpleDescRsp->DeviceVersion;

        // Get unique cluster list (IN+OUT combined)
        zendpoint->num_in_cluster = array_without_duplicate((uint16_t*) &SimpleDescRsp->InClusterList,
                                                            SimpleDescRsp->NumInClusters,
                                                            (uint16_t*) &SimpleDescRsp->OutClusterList,
                                                            SimpleDescRsp->NumOutClusters,
                                                            (uint16_t*) &zendpoint->clusters);

        printf("Endpoint %d has %d clusters (combined IN+OUT):\n", ep, zendpoint->num_in_cluster);
        for (j = 0; j < zendpoint->num_in_cluster; j++)
        {
            printf("  - Cluster ID: 0x%04X\n", zendpoint->clusters[j]);
        }

        // Register the endpoint in the application framework
        Register.EndPoint = zendpoint->id;
        Register.AppProfId = zendpoint->profile_id;
        Register.AppDeviceId = zendpoint->device_id;
        Register.AppDevVer = zendpoint->device_version;
        Register.LatencyReq = 0;
        Register.AppNumInClusters = zendpoint->num_in_cluster;
        Register.AppNumOutClusters = zendpoint->num_in_cluster;

        for (j = 0; j < zendpoint->num_in_cluster; j++)
        {
            Register.AppInClusterList[j] = zendpoint->clusters[j];
            Register.AppOutClusterList[j] = zendpoint->clusters[j];
        }

        afRegister(&Register);
        mem_free(SimpleDescRsp, msg_app.data_length);
        set_element((dbt_generic**) &zendpoint, ZENDPOINT, true);
        Task_sleep(500);
    }
    mem_free(ActiveEpRsp, sizeof(ActiveEpRspFormat_t));

    set_element((dbt_generic**) &zdevice, ZDEVICE, true);
    Task_sleep(500);

    // Request model and firmware version
    printf("Requesting model and firmware version from the device...\n");
    get_fv_MN_model(nwk_address);
    status = Mailbox_pend(m_add_device, &msg_app, 5000);

    if (!status || msg_app.data == NULL)
    {
        printf("Error getting device model\n");
        zdo_permit_join_req(0);
        return;
    }

    // Compare received model
    if (strncmp((char*)msg_app.data, model, model_size) == 0)
    {
        printf("Correct model received: %s\n", model);
        mem_free(msg_app.data, msg_app.data_length);

        // Retrieve device again to populate MQTT message
        zdevice = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice,
                                              (char**) &config_db.db_zdevice->nwk_address,
                                              false, ZDEVICE, NUMBER, 0, nwk_address, "", N_UINT16);

        // Prepare MQTT message
        char mac[18];
        db_get_mac(mac);
        msg_mqtt.size_topic = 70;
        msg_mqtt.topic = mem_alloc(msg_mqtt.size_topic);
        sprintf(msg_mqtt.topic, "smaway/%s/tocloud/config/111", mac);

        sprintf(ieee_address, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                BREAK_UINT64(zdevice->ieee_address, 7),
                BREAK_UINT64(zdevice->ieee_address, 6),
                BREAK_UINT64(zdevice->ieee_address, 5),
                BREAK_UINT64(zdevice->ieee_address, 4),
                BREAK_UINT64(zdevice->ieee_address, 3),
                BREAK_UINT64(zdevice->ieee_address, 2),
                BREAK_UINT64(zdevice->ieee_address, 1),
                BREAK_UINT64(zdevice->ieee_address, 0));

        msg_mqtt.size_content = 150;
        msg_mqtt.content = mem_alloc(msg_mqtt.size_content);
        sprintf(msg_mqtt.content, "{\"DeviceId\" : \"%s\",\"FirmwareVersion\" : \"%hhu\",\"Model\" : \"%s\"}",
                ieee_address, zdevice->firmware_version, zdevice->model_id);

        msg_mqtt.size_buf = 0;
        msg_mqtt.buf = NULL;

        mem_free(zdevice, sizeof(dbt_zdevice));

        // Send MQTT message
        sendToMqttCheckingMailbox(&msg_mqtt);
        //Mailbox_post(m_mqtt_send, &msg_mqtt, BIOS_WAIT_FOREVER);
        Task_sleep(2000);

        sprintf(lm, "zigbee - new device: ieee: %s model: %s", ieee_address, model);
        logevent(L_DEBUG, lm);

        Task_sleep(1000);

        // Perform binding and cluster configuration
        printf("Performing binding and cluster configuration for device 0x%04X...\n", nwk_address);
        binding_clusters(nwk_address);
        configure_device(nwk_address);  //TODO: Add handling of problems when configuring and binding
    }
    else
    {
        // If model does not match, remove device
        printf("Error: received model does not match (%s vs %s)\n", (char*)msg_app.data, model);
        mem_free(msg_app.data, msg_app.data_length);

        zdevice = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice,
                                              (char**) &config_db.db_zdevice->nwk_address,
                                              false, ZDEVICE, NUMBER, 0, nwk_address, "", N_UINT16);

        if (zdevice && strlen(zdevice->model_id) == 0)
        {
            ExtSecApsRemoveReqFormat_t RemoveReq;
            RemoveReq.NWKAddr = nwk_address;

            for (i = 0; i < 8; i++)
                RemoveReq.IEEEaddr[i] = BREAK_UINT64(zdevice->ieee_address, i);

            RemoveReq.parent_NWKAddr = parent_address;

            printf("Removing device with NWK address 0x%04X and IEEE 0x%llX\n",
                   nwk_address, zdevice->ieee_address);

            zdo_ext_sec_aps_remove_req(&RemoveReq);
            mem_free(zdevice, sizeof(dbt_zdevice));
        }
    }

    // Finish pairing and close network
    printf("Finishing pairing. Closing network join permissions.\n");
    zdo_permit_join_req(0);
}

void processZigbeeIEEE(const char *ieee_input, char *ieee, uint8_t *num) {
    // Search for the '-' character in the input string.
    const char *dash = strchr(ieee_input, '-');

    if (dash != NULL) {
        // Dash found. Copy the IEEE address portion until the dash.
        size_t len = dash - ieee_input;
        strncpy(ieee, ieee_input, len);
        ieee[len] = '\0';  // Ensure the string is null-terminated.
        // Convert the number following the dash to an integer.
        *num = atoi(dash + 1);
    } else {
        // No dash found; copy the entire string and assign 0 to the number.
        strcpy(ieee, ieee_input);
        *num = 0;
    }
}

void rpc_send(UArg a0, UArg a1)
{
    msg_rpc_t msg_send = { 0 };
    msg_mqtt_t msg;

    char *s;
    int16_t r, ret = -1;
    int64_t i;
    uint8_t value;
    uint64_t ieee;

    char *state = NULL;
    char *model = NULL;

    uint16_t state_size;
    uint16_t model_size;

    msg_dbev_t msg_app;
    db_event_t e;

    dbt_zdevice *zdevice = NULL;
    dbt_zendpoint *zendpoint = NULL;
    dbt_device *device = NULL;

    Json_Handle json = NULL;
    Json_Handle hTemplate = NULL;

    char ieee_[50];  // Buffer to store the IEEE address (adjust size as needed)
    uint8_t endpoint;

    char lm[70];

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);
    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    // Wait for NW ready
    Semaphore_pend(sem_online, BIOS_WAIT_FOREVER);
    // Release the read semaphore
    Semaphore_post(sem_online);

    Semaphore_pend(sem_uart, BIOS_WAIT_FOREVER);

    bool zigbee_initialized = false;
    int8_t zigbee_status = 0;


    while (1)
    {

        Mailbox_pend(m_rpc_send, &msg_send, BIOS_WAIT_FOREVER);

        // Init Zigbee only if necesary
        if (!zigbee_initialized)
        {
            bool needs_zigbee = true;
            if (msg_send.item_type)
            {
                const char* no_zigbee_cmds[] = {"116", "117", "118", "119", "120", "123", "125", "126", NULL};
                int i;
                for (i = 0; no_zigbee_cmds[i]; i++)
                {
                    if (strcmp(msg_send.item_type, no_zigbee_cmds[i]) == 0)
                    {
                        needs_zigbee = false;
                        break;
                    }
                }
            }

            if (needs_zigbee)
            {
                flash_read_val(FL_STATUS_1, &zigbee_status, 1);

                if(zigbee_status != FACTORY_DEFAULT)
                {
                    ret = sys_reset(0);
                    Task_sleep(500);

                    //Init zigbee network
                    ret = zdoInit();
                    Task_sleep(1000);
                }
                else
                {
                    factory_reset(12);
                }

                zap_add_zdevice_coord();
                request_status();
                get_Coordinator_NwkAddr();
                command_101(zigbee_status);
                zigbee_initialized = true;
                printf("Zigbee initialized\n");
            }
            else
            {
                flash_read_val(FL_STATUS_1, &zigbee_status, 1);
                zigbee_initialized = true;
            }
        }

        Task_sleep(1000);

        s = msg_send.data;

        if (msg_send.device_id != NULL && msg_send.id_length > 20)
        {
            // Process the first IEEE address.
            processZigbeeIEEE(msg_send.device_id, ieee_, &endpoint);
            ieee = ieee_to_uint64(ieee_);
            zdevice = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->ieee_address, false, ZDEVICE, NUMBER, 0, ieee, "", N_UINT64);
        }

        if (msg_send.device_id == NULL && msg_send.item_type == NULL)
        {
            if ((msg_send.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_ZDO)
            {
                ret = zdo_cmd_send(msg_send.data[1], (uint8_t*) (msg_send.data + 2), msg_send.data_length);
            }
            else if ((msg_send.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SYS)
            {
            }
            else if ((msg_send.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_AF)
            {
                ret = af_cmd_send(msg_send.data[1], (uint8_t*) (msg_send.data + 2), msg_send.data_length);
            }
            else if ((msg_send.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_SAPI)
            {
            }
            else if ((msg_send.data[0] & MT_RPC_SUBSYSTEM_MASK) == MT_RPC_SYS_APP_CONFIG)
            {
            }
        }
        else
        {
            if (strcmp(msg_send.item_type, "110") == 0)
            {
                ret = create_json(&json, &hTemplate, msg_send.item_type);
                ret = parse_json(json, msg_send.data, msg_send.data_length);

                state = get_jsonstring(json, "\"State\"", &state_size);
                model = get_jsonstring(json, "\"Model\"", &model_size);

                ret = Json_destroyObject(json);
                ret = Json_destroyTemplate(hTemplate);

                if (strcmp(state, "OPEN") == 0)
                {
                    ret = zdo_permit_join_req(60);

                    add_zdevice(model,model_size);

                    free(state);
                    free(model);

                }
                else
                {
                    ret = zdo_permit_join_req(0);

                    free(state);
                    free(model);

                }
            }
            else if (strcmp(msg_send.item_type, "114") == 0)
            {
                ret = create_json(&json, &hTemplate, msg_send.item_type);
                ret = parse_json(json, msg_send.data, msg_send.data_length);

                state = get_jsonstring(json, "\"DeviceId\"", &state_size);

                ieee = ieee_to_uint64(state);
                ret = nwk_remove_device_req(ieee);

                zdevice = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->ieee_address, false, ZDEVICE, NUMBER, 0, ieee, "", N_UINT64);

                if (zdevice)
                {
                    zendpoint = (dbt_zendpoint*) find_element((dbt_generic**) &config_db.db_zendpoint, (char**) &config_db.db_zendpoint->zdevice_id, false, ZDEVICE, NUMBER, 0, zdevice->id, "", N_UINT16);

                    while (zendpoint)
                    {
                        db_delete_item((dbt_generic**) &zendpoint, (dbt_generic**) &config_db.db_zendpoint, (dbt_generic**) &config_db.db_zendpoint_last, ZENDPOINT);
                        zendpoint = (dbt_zendpoint*) find_element((dbt_generic**) &config_db.db_zendpoint, (char**) &config_db.db_zendpoint->zdevice_id, false, ZDEVICE, NUMBER, 0, zdevice->id, "", N_UINT16);
                    }

                    db_delete_item((dbt_generic**) &zdevice, (dbt_generic**) &config_db.db_zdevice, (dbt_generic**) &config_db.db_zdevice_last, ZDEVICE);
                }
                sprintf(lm, "zigbee_send: zdevice_delete %s", state);
                logevent(L_INFO, lm);

                char mac[18];
                db_get_mac(mac);

                msg.size_topic = 70;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/config/112", mac);

                msg.size_content = 100;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "{\"DeviceId\" : \"%s\", \"Status\": \"REMOVED\"}", state);

                msg.size_buf = 0;
                msg.buf = NULL;

                sendToMqttCheckingMailbox(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);

                ret = Json_destroyObject(json);

                ret = Json_destroyTemplate(hTemplate);

                free(state);
            }
            else if (strcmp(msg_send.item_type, "115") == 0)
            {
                uint32_t properties_count = 0;
                uint32_t j = 0;
                char keyBuf[37];

                ret = create_json(&json, &hTemplate, msg_send.item_type);
                ret = parse_json(json, msg_send.data, msg_send.data_length);

                state = get_jsonstring(json, "\"DeviceId\"", &state_size);
                ieee = ieee_to_uint64(state);
                zdevice = (dbt_zdevice*) find_element((dbt_generic**) &config_db.db_zdevice, (char**) &config_db.db_zdevice->ieee_address, false, ZDEVICE, NUMBER, 0, ieee, "", N_UINT64);
                free(state);

                properties_count = Json_getArrayMembersCount(json, "\"Properties\"");

                for (j = 0; j < properties_count && ret; j++)
                {
                    sprintf(keyBuf, "\"Properties\".[%d].\"PropertyName\"", j);
                    state = get_jsonstring(json, keyBuf, &state_size);
                    sprintf(keyBuf, "\"Properties\".[%d].\"Value\"", j);
                    model = get_jsonstring(json, keyBuf, &model_size);

                    if (strcmp(state, "child-lock") == 0)
                    {
                        set_child_lock_enabled(zdevice, 0x0001, atoi(model));
                    }
                    else if(strcmp(state, "frost") == 0)
                    {
                        set_frost_enabled(zdevice, 0x0001, atoi(model));
                    }
                    else if(strcmp(state, "water-scale-proof") == 0)
                    {
                        set_water_scale_proof(zdevice, 0x0001, atoi(model));
                    }
                    else if(strcmp(state, "window-check") == 0)
                    {
                        set_window_check_enabled(zdevice, 0x0001, atoi(model));
                    }
                    else if(strcmp(state, "firmware-version") == 0)
                    {
                    }

                    free(state);
                    free(model);
                }

                ret = Json_destroyObject(json);
                ret = Json_destroyTemplate(hTemplate);

                mem_free(zdevice,sizeof(dbt_zdevice));

            }
            else if (strcmp(msg_send.item_type, "116") == 0)
            {
                int32_t log_level = 0;
                uint16_t ll_size = sizeof(log_level);

                device = (dbt_device*) get_element((dbt_generic**) &config_db.db_device, DEVICE);

                ret = create_json(&json, &hTemplate, msg_send.item_type);
                if (ret != 0)
                {
                    printf("Error creating the new json\n");
                }
                else
                {
                    printf("New Json created succesfully\n");
                }
                ret = parse_json(json, msg_send.data, msg_send.data_length);
                if (ret != 0)
                {
                    printf("Error parsing\n");
                }
                else
                {
                    printf("parsign succesfull\n");
                }
                ret = Json_getValue(json, "\"LogLevel\"", &log_level, &ll_size);
                if (ret != 0)
                {
                    printf("Error getting the new log value: %d\n",log_level);
                }
                else
                {
                    printf("New log value: %d\n", log_level);
                }

                device->loglev = (int8_t)log_level;

                ret = Json_destroyObject(json);

                ret = Json_destroyTemplate(hTemplate);

                mem_free(state,state_size);

                char mac[18];
                db_get_mac(mac);

                msg.size_topic = 100;
                msg.topic = mem_alloc(msg.size_topic);
                sprintf(msg.topic, "smaway/%s/toplatform/config/117\0", mac);

                msg.size_content = 20;
                msg.content = mem_alloc(msg.size_content);
                sprintf(msg.content, "{\"LogLevel\":%d}", (int8_t)log_level);

                set_element((dbt_generic**) &device, DEVICE, true);

                sendToMqttCheckingMailbox(&msg);
                //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
            }
            else if (strcmp(msg_send.item_type, "118") == 0)
            {
                ret = create_json(&json, &hTemplate, msg_send.item_type);
                ret = parse_json(json, msg_send.data, msg_send.data_length);

                state = get_jsonstring(json, "\"FirmwareVersion\"", &state_size);

                ret = firmware_update(atoi(state));

                ret = Json_destroyObject(json);
                ret = Json_destroyTemplate(hTemplate);

                mem_free(state, state_size);
            }
            else if (strcmp(msg_send.item_type, "119") == 0)
            {
                SPIFFS_remove(&fs_spiffs, TEMPLATE_PATH);
                uint16_t counter = 0;
                counter = match_counter(s, '#');
                int i;

                    uint16_t *indexes = calloc(counter, sizeof(uint16_t));
                    get_index_match(s, '#', counter, indexes);

                char name[MAX_STR_LEN];
                char template[MAX_STR_LEN];
                for(i = 0; i < counter; i += 2){
                    get_element_string(s, indexes, counter, name, i);
                    get_element_string(s, indexes, counter, template, i + 1);
                    ret = create_template(name, template, strlen(template) + 1);
                    if(ret){
                        if(save_item_spiffs(name, TEMPLATE_PATH) == 0){
                            printf("Error saving template to list");
                        }
                    }
                }

                free(indexes);
                i = CONFIGURED;
                flash_write_val(FL_STATUS_1, &i, 1);
            }
            //Send logs
            else if (strcmp(msg_send.item_type, "120") == 0)
            {
                ret = send_logs();
            }
            //Send files
            else if (strcmp(msg_send.item_type, "123") == 0)
            {
                ret = send_files();
            }
            //Erase flash
            else if (strcmp(msg_send.item_type, "125") == 0)
            {
                send_file_operation_response("", true);
                Task_sleep(1000);
                erase_flash();
            }
            //Erase file
            else if (strcmp(msg_send.item_type, "126") == 0)
            {
                SPIFFS_remove(&fs_spiffs, s);
                send_file_operation_response(s, false);
            }
            //Command for zigbee status
            else if (strcmp(msg_send.item_type, "SYS_PING") == 0)
            {
                ret = sys_ping();
            }
            else if (strcmp(msg_send.item_type, "FACTORY_RESET") == 0)
            {
                ret = factory_reset(atoi(msg_send.data));
            }
            else if (strcmp(msg_send.item_type, "DELETE_TEMPLATE") == 0)
            {
                char name[10];
                r = get_next_dbset(s, &i,(char *) &name);
                if (r)
                {
                    s += r;
                }
                ret = delete_template((char *)&name);

            }
            else if (strcmp(msg_send.item_type, "DELETE_DB") == 0)
            {
                SPIFFS_remove(&fs_spiffs, "sw.cfg");
                SPIFFS_remove(&fs_spiffs, "sw.cfg.bu");
                ret = 0;
            }
            else if (strcmp(msg_send.item_type, "COSMO_RESET") == 0)
            {
                //Update database
                e = DBE_EXPORT;
                if (Mailbox_post(m_db, &e, BIOS_NO_WAIT) == false)
                {
                    err_mailbox_full(__func__, m_db);
                }

                Task_sleep(3000);
                Semaphore_post(sem_reboot);
            }
            else if (strcmp(msg_send.item_type, "GET_NWK_KEY") == 0)
            {
                ret = get_nwk_key();
            }
            else if (strcmp(msg_send.item_type, "TC_REQUIRE_KEY_EXANCHE") == 0)
            {
                ret = appCnfBDBSetTCRequireKeyExchange(atoi(msg_send.data));
            }
            else if (strcmp(msg_send.item_type, "STARTNETWORK") == 0)
            {
                ret = appCnfBDBStartCommisioning(0x04);
            }
            else if (strcmp(msg_send.item_type, "UTIL_GET_NV_INFO") == 0)
            {
                ret = get_NV_info();
            }
            else if (strcmp(msg_send.item_type, "SETCHANNEL") == 0)
            {
                ret = set_channel(atoi(msg_send.data));
            }
            else if (strcmp(msg_send.item_type, "SETZDODIRECTCB") == 0)
            {
                ret = set_zdo_direct_cb();
            }
            else if (strcmp(msg_send.item_type, "SETPANID") == 0)
            {
                ret = set_panid(atoi(msg_send.data));
            }
            else if (strcmp(msg_send.item_type, "SETDEVTYPE") == 0)
            {
                ret = set_dev_type(atoi(msg_send.data));
            }
            else if (strcmp(msg_send.item_type, "GetDeviceListReq") == 0)
            {
                msg_app.tab_id = ZDEVICE;
                msg_app.rec_id = 65535;
                parse_dbget(&msg_app);
                ret = 0;
            }
            else if (strcmp(msg_send.item_type, "NetworkInfoReq") == 0)
            {
                ret = get_nwk_info();
            }
            else if (strcmp(msg_send.item_type, "ChangeNetworkKeyReq") == 0)
            {
            }
            else if (strcmp(msg_send.item_type, "SystemReset") == 0)
            {
                ret = sys_reset(0);
            }

            else if (strcmp(msg_send.item_type, "firmware_version") == 0)
            {
                ret = get_zdevice_firmware_version(zdevice);
            }
            else if (strcmp(msg_send.item_type, "model_id") == 0)
            {
                ret = get_zdevice_model_id(zdevice);
            }
            else if (strcmp(msg_send.item_type, "power_source") == 0)
            {
                ret = get_zdevice_power_source(zdevice);
            }
            else if (strcmp(msg_send.item_type, "onoff") == 0)
            {
                if (strcmp(msg_send.data, "REFRESH") == 0)
                {
                    ret = read_onoff(zdevice, endpoint);
                }
                else
                {
                    value = (strcmp(msg_send.data, "ON") == 0) ? 1 : 0;
                    ret = write_onoff(zdevice, endpoint, value);
                    printf("Write onoff %d", ret);
                    ret = read_onoff(zdevice, endpoint);
                }
            }
            else if (strcmp(msg_send.item_type, "onoff_2") == 0)
            {
                if (strcmp(msg_send.data, "REFRESH") == 0)
                {
                    ret = get_zdevice_onoff2(zdevice);
                }
                else
                {
                    value = (strcmp(msg_send.data, "ON") == 0) ? 1 : 0;
                    ret = set_zdevice_onoff2(zdevice, value);
                }
            }
            else if (strcmp(msg_send.item_type, "level_control") == 0)
            {
            }
            else if (strcmp(msg_send.item_type, "local-temperature") == 0)
            {
                ret = get_local_temperature(zdevice);
            }
            else if (strcmp(msg_send.item_type, "target-temperature") == 0)
            {
                ret = set_target_temperature(zdevice, (int16_t) (atof(msg_send.data) * 100));
            }
            else if (strcmp(msg_send.item_type, "outdoor-temperature-measurement") == 0)
            {
            }
            else if (strcmp(msg_send.item_type, "system-mode") == 0)
            {
                uint8_t value = (strcmp(msg_send.data, "OFF")==0)? 0x00 : (strcmp(msg_send.data, "AUTO")==0)? 0x01:(strcmp(msg_send.data, "COOL")==0)? 0x03: 0x04;
                ret = set_system_mode(zdevice,value);
            }
            else if (strcmp(msg_send.item_type, "battery-percent") == 0)
            {
                ret = get_battery_percent(zdevice);
            }
            else if (strcmp(msg_send.item_type, "battery-low") == 0)
            {
            }
            else if (strcmp(msg_send.item_type, "temperature-measurement") == 0)
            {
            }
            else if (strcmp(msg_send.item_type, "humidity-measurement") == 0)
            {
            }
            else if (strcmp(msg_send.item_type, "lqi-value") == 0)
            {
                if (strcmp(msg_send.data, "REFRESH") == 0)
                {
                    ret = get_lqi_value(zdevice);
                }
            }
        }
        if (ret == -1)
        {
            sprintf(lm, "zigbee_send error command:%s device %s",msg_send.item_type, ieee_);
            logevent(L_DEBUG, lm);
        }
        if (msg_send.data)
        {
            mem_free(msg_send.data, msg_send.data_length);
        }
        if (msg_send.item_type)
        {
            mem_free(msg_send.item_type, msg_send.type_length);
        }
        if (msg_send.device_id)
        {
            mem_free(msg_send.device_id, msg_send.id_length);
        }
        ret = -1;
        if (msg_send.device_id != NULL && msg_send.id_length > 20)
        {
            mem_free(zdevice, sizeof(dbt_zdevice));
        }
    }
}

void rpc_recv(UArg a0, UArg a1)
{
    msg_rpc_t msg;
    Semaphore_post(sem_uart);

    while (1)
    {
        Mailbox_pend(m_rpc_arsp, &msg, BIOS_WAIT_FOREVER);
        mtProcess((uint8_t*) msg.data, msg.data_length);
        mem_free(msg.data, msg.data_length);
    }
}
void rpc_reciever(UArg a0, UArg a1)
{
    Task_Params taskParams;
// Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);
// Release the read semaphore
    Semaphore_post(sem_db_ready);

// Wait for NW ready
    Semaphore_pend(sem_online, BIOS_WAIT_FOREVER);
// Release the read semaphore
    Semaphore_post(sem_online);

    Task_sleep(1000);

    afResetResponseSemaphore();
// Create the can_new_msg task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_HIGH;
    taskParams.instance->name = "t_rpc_recv";
    t_rpc_recv = Task_create(rpc_recv, &taskParams, NULL);

    while (1)
    {
        rpcProcess();
    }
}

void delete_rpc_recv()
{
    Task_delete(&t_rpc_recv);
}

bool runConfigSteps_THERMB01(dbt_zdevice *zdev, uint8_t step)  //TODO: Make single function for any configuration receiving the step number
{
    switch (step) {
        case 1:
            return set_thermostat_Control_SeqOfOper(zdev, 0x0001);
        case 2:
            //configure_Rep_target_temp(zdev, 0x0001, 0, 600);
            return configure_rep_SM_OHeatSetp(zdev, 0x0001, 0, 600);
        case 3:
            return configure_Rep_local_temp(zdev, 0x0001, 480, 600);
        case 4:
            return configure_rep_system_mode(zdev, 0x0001, 0, 600);
        case 5:
            configure_powerconf(zdev, 0x0001, 540, 600);
            break;
        case 6:
            configure_diagnostic_lqi(zdev, 0x0001, 1680, 1800);
            break;
        case 7:
            read_lqi_value(zdev);
            break;
        case 8:
            read_battery_low(zdev);
            break;
        case 9:
            read_local_temperature(zdev);
            break;
        case 10:
            read_target_temperature(zdev);
            break;
        case 11:
            read_system_mode(zdev);
            break;
        default:
            break;
    }
    return true;    //For functions still not in the retry strategy it returns true

}

bool runConfigSteps_HTHERMB1(dbt_zdevice *zdev, uint8_t step)  //TODO: Make single function for any configuration receiving the step number
{
    switch (step) {
        case 1:
            return set_thermostat_Control_SeqOfOper(zdev, 0x0001);
        case 2:
            //configure_Rep_target_temp(zdev, 0x0001, 0, 600);
            return configure_rep_SM_OHeatSetp(zdev, 0x0001, 0, 600);
        case 3:
            return configure_Rep_local_temp(zdev, 0x0001, 480, 600);
        case 4:
            return configure_rep_system_mode(zdev, 0x0001, 0, 600);
        case 5:
            configure_battery_percentage(zdev, 0x0001, 540, 600);
            break;
        case 6:
            configure_valve_state(zdev, 0x0001, 0, 600);
            break;
        case 7:
            configure_report_child_lock_enabled(zdev, 0x0001, 0, 600);
            break;
        case 8:
            read_power_source(zdev);
            break;
        case 9:
            read_lqi_value(zdev);
            break;
        case 10:
            read_battery_low(zdev);
            break;
        case 11:
            read_battery_percent(zdev);
            break;
        case 12:
            read_local_temperature(zdev);
            break;
        case 13:
            read_target_temperature(zdev);
            break;
        case 14:
            read_system_mode(zdev);
            break;
        case 15:
            read_water_scale_proof(zdev, 0x0001);
            break;
        case 16:
            read_frost_enabled(zdev, 0x0001);
            break;
        case 17:
            read_window_check_enabled(zdev, 0x0001);
            break;
        case 18:
            read_child_lock_enabled(zdev, 0x0001);
            break;
        case 19:
            read_key_away_enabled(zdev, 0x0001);
            break;
        case 20:
            read_motor_opening_size(zdev, 0x0001);
            break;
        default:
            break;
    }
    return true;    //For functions still not in the retry strategy it returns true

}
