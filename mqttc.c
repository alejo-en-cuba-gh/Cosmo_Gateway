/*
 * mqttc.c
 *
 *  Created on: 6 mar 2024
 */

#include "common.h"
#include "mqttc.h"
#include <ti/net/slnetif.h>
#include <xdc/runtime/System.h>
#include <ti/drivers/net/wifi/wlan.h>
#include "network_if.h"
#include <ti/drivers/net/wifi/slnetifwifi.h>

static bool IsInit = true;
extern uint16_t vfw;

MQTTClient_Handle mc = NULL;
MQTTClient_Handle mc_wifi = NULL;
MQTTClient_Handle mc_active = NULL;

Task_Handle t_mqtt_run = NULL, t_mqtt_send = NULL;
Semaphore_Handle sem_mq_disc, sem_mq_connected, sem_mqtt_init;
bool mq_connected = false;
char mt[60], mt2[60], st[60];

extern uint8_t sc_sec_ca[], sc_sec_pk[], sc_sec_pc[];
extern uint16_t sc_sec_ca_l, sc_sec_pk_l, sc_sec_pc_l;

extern void fdOpenSession();
extern void fdCloseSession();
extern void* TaskSelf();

extern bool safeTaskDelete(Task_Handle *taskHandle);
void sendToMqttMailbox(msg_mqtt_t* msg);
bool sendToMqttMailboxNoWait(msg_mqtt_t* msg);
UInt checkChannelMailbox(/*sendchannel_type channel*/);
UInt checkMqttMailbox();

#define MAX_TOPIC_SIZE 128
#define MAX_CONTENT_SIZE 600

void mqtt_client(UArg a0, UArg a1)
{
    MQTTClient_Params mp_wifi;
    MQTTClient_ConnParams mcp_wifi;
    WifiSTATUS wifiInfo;
    uint8_t status = 0;
    MQTTClient_Params mp;
    MQTTClient_ConnParams mcp;
    MQTTClient_SubscribeParams msp[2];
    MQTTClient_Will wp;
    Task_Params taskParams;
    int16_t e;
    uint8_t conn_attempt_cnt = 0;
    char mac[18];

    msg_led_t l;

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);
    Semaphore_post(sem_db_ready);

    wifi_init();
    Task_sleep(5000);

    status = SlNetIf_init(0);
    if (status == 0) status = SlNetSock_init(0);
    if (status == 0) SlNetUtil_init(0);
    if (status == 0) status = SlNetIf_add(SLNETIF_ID_2, "CC31xx", (const SlNetIf_Config_t *)&SlNetIfConfigWifi, 5);

    // Wait for NW ready
    Semaphore_pend(sem_online, BIOS_WAIT_FOREVER);
    Semaphore_post(sem_online);

    // Wait for time to update
    Semaphore_pend(sem_sntp, BIOS_WAIT_FOREVER);
    Semaphore_post(sem_sntp);

    Task_sleep(3000);
    delete_old_files_by_size_spiffs(LOG_PATH, (float)0.4, (float)0.3);

    db_get_mac(mac);

    char *msf_wifi[4] = { "key.key", "cert.der", "ca-cert.der", NULL };
    char *msf[3] = { "key.pem", "cert.pem", "ca-cert.pem"};
    char *offline_msg = mem_alloc(90);
    if(offline_msg==NULL)
        Semaphore_post(sem_reboot);

    sprintf(offline_msg, "{\"Part\":1,\"Last\":1,\"Status\":4,\"Components\":[],\"IsInit\":false,\"BoxVersion\":%d,\"LogLevel\":%d}", vfw, get_loglevel());

    mcp_wifi.netconnFlags = MQTTCLIENT_NETCONN_URL | MQTTCLIENT_NETCONN_SEC | MQTTCLIENT_NETCONN_SKIP_CERTIFICATE_CATALOG_VERIFICATION;
    mcp_wifi.serverAddr = "a1kshpxbz3o3ld-ats.iot.us-west-2.amazonaws.com";
    mcp_wifi.port = 8883;
    mcp_wifi.method = SLNETSOCK_SEC_METHOD_TLSV1_2;
    mcp_wifi.cipher = SLNETSOCK_SEC_CIPHER_FULL_LIST;
    mcp_wifi.nFiles = 4;
    mcp_wifi.secureFiles = msf_wifi;

    mp_wifi.clientId = mem_alloc(strlen(mac));
    sprintf(mp_wifi.clientId, "%s", mac);
    mp_wifi.connParams = &mcp_wifi;
    mp_wifi.mqttMode31 = false;
    mp_wifi.blockingSend = true;

    mcp.netconnFlags = MQTTCLIENT_NETCONN_URL | MQTTCLIENT_NETCONN_SEC | MQTTCLIENT_NETCONN_SKIP_CERTIFICATE_CATALOG_VERIFICATION;
    mcp.serverAddr = "a1kshpxbz3o3ld-ats.iot.us-west-2.amazonaws.com";
    mcp.port = 8883;
    mcp.method = SLNETSOCK_SEC_METHOD_TLSV1_2;
    mcp.cipher = SLNETSOCK_SEC_CIPHER_FULL_LIST;
    mcp.nFiles = 3;
    mcp.secureFiles = msf;

    mp.clientId = mem_alloc(strlen(mac));
    if(mp.clientId==NULL)
        Semaphore_post(sem_reboot);
    sprintf(mp.clientId, "%s", mac);
    mp.connParams = &mcp;
    mp.mqttMode31 = false;
    mp.blockingSend = true;

    sprintf(mt, "smaway/%s/tohub/config/#", mac);
    sprintf(mt2, "smaway/%s/tohub/item/+/#", mac,mac);
    sprintf(st, "smaway/%s/tocloud/config/101", mac);

    msp[0].topic = mt;
    msp[0].qos = MQTT_QOS_1;
    msp[0].persistent = false;

    msp[1].topic = mt2;
    msp[1].qos = MQTT_QOS_1;
    msp[1].persistent = false;

    wp.willTopic = st;
    wp.willMsg = offline_msg;
    wp.willQos = MQTT_QOS_1;
    wp.retain = false;

    l.led = LS_AC;
    l.state = true;
    if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false) {
        err_mailbox_full(__func__, m_led);
    }

    unsigned char der[2048];
    int len;

    fdOpenSession(TaskSelf());

    e = SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_CERTIFICATE, msf[2], strlen(msf[2]), sc_sec_ca, sc_sec_ca_l, SLNETIF_ID_1);
    e = SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_CERTIFICATE, msf[1], strlen(msf[1]), sc_sec_pc, sc_sec_pc_l, SLNETIF_ID_1);
    e = SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_RSA_PRIVATE_KEY, msf[0], strlen(msf[0]), sc_sec_pk, sc_sec_pk_l, SLNETIF_ID_1);

    while (1)
    {
        e = -1;
        //printf("mattTask: entering mqttClient loop\n");

        update_wifi_status();

        if (wifi_connected && (conn_attempt_cnt < 3))
        {
            update_sntp_time();
            struct tm current_time;
            get_ltime(&current_time);
            update_CC3135_time(&current_time);
            print_CC3135_time();

            #pragma FUNC_CANNOT_INLINE(pem_to_der)
            #pragma OPTIMIZE_OFF

            len = pem_to_der(sc_sec_ca, sc_sec_ca_l, der);
            e = SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_CERTIFICATE, msf_wifi[2], strlen(msf_wifi[2]), der, len, SLNETIF_ID_2);

            len = pem_to_der(sc_sec_pc, sc_sec_pc_l, der);
            e = SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_CERTIFICATE, msf_wifi[1], strlen(msf_wifi[1]), der, len, SLNETIF_ID_2);

            e = SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_RSA_PRIVATE_KEY, msf_wifi[0], strlen(msf_wifi[0]), sc_sec_pk, sc_sec_pk_l, SLNETIF_ID_2);

            #pragma OPTIMIZE_ON

            mc_wifi = MQTTClient_create(mqtt_cb, &mp_wifi);
            mc_active = mc_wifi;
            if(mc_active == NULL) {

                //TODO LOG
            }
            MQTTClient_set(mc_active, MQTTClient_WILL_PARAM, &wp, sizeof(wp));

            Task_Params_init(&taskParams);
            taskParams.priority = TASK_PRIO_MEDIUM;
            taskParams.stackSize = TASK_HUNGRY;
            taskParams.instance->name = "t_mqtt_run";
            t_mqtt_run = Task_create(mqtt_run, &taskParams, NULL);

            e = MQTTClient_connect(mc_active);
            if (e < 0)
            {
                printf("mqttTask: connection failed WIFI %d\n", e);
                MQTTClient_delete(mc_active);
            }
        }

        if (e < 0)
        {
            if ((sem_nw_ready && (conn_attempt_cnt >= 3)) || (wifi_connected == false))
            {
                mc = MQTTClient_create(mqtt_cb, &mp);
                mc_active = mc;
                if(mc_active) {
                    MQTTClient_set(mc_active, MQTTClient_WILL_PARAM, &wp, sizeof(wp));

                    Task_Params_init(&taskParams);
                    taskParams.priority = TASK_PRIO_MEDIUM;
                    taskParams.stackSize = TASK_HUNGRY;
                    taskParams.instance->name = "t_mqtt_run";
                    t_mqtt_run = Task_create(mqtt_run, &taskParams, NULL);
                    //If it fails to allocate space for mqtt_run
                    //it deletes the client and sets 'e' to retry MQTT
                    if(t_mqtt_run)
                    //t_mqtt_run = Task_create(mqtt_run, &taskParams, NULL);
                    {
                        e = MQTTClient_connect(mc_active);

                        printf("\n\n------.:Trying to run the MQTT client:.------\n");
                        if (e < 0)
                        {
                            printf("[MQTT_Client] Connection Failed: %d\n", e);
                            printf("[MQTT_Client] Trying again\n");
                            MQTTClient_delete(mc_active);
                            mc_active = NULL;
                            safeTaskDelete(&t_mqtt_run);
                        }
                        #ifdef _DEBUG_
                        else
                            printf("[MQTT_Client] Connection Successful\n");
                        #endif
                    }
                    else
                    {
                        #ifdef _DEBUG_
                        printf("[MQTT_Client] Unable to create the client: %d\n", e);
                        #endif
                        MQTTClient_delete(mc_active);
                        mc_active = NULL;

                        e = -1;
                    }
                }
            }
        }

        if (e == 0)
        {

            Task_Params_init(&taskParams);
            taskParams.stackSize = TASK_NORMAL;
            taskParams.priority = TASK_PRIO_MEDIUM;
            taskParams.instance->name = "t_mqtt_send";
            t_mqtt_send = Task_create(mqtt_send, &taskParams, NULL);
            //If it fails to allocate space for t_mqtt_send
            //it deletes the client and closes t_mqtt_run and gets ready to retry MQTT connection
            if(t_mqtt_send)
            {
                l.led = LS_AC;
                l.state = false;
                Mailbox_post(m_led, &l, BIOS_NO_WAIT);

                l.led = LS_NET;
                l.state = false;
                Mailbox_post(m_led, &l, BIOS_NO_WAIT);

                if (wifi_connected)
                    Task_sleep(15000);

                create_com_task();
                e = MQTTClient_subscribe(mc_active, msp, 2);   //TODO: Add a better handling of subscription
                #ifdef _DEBUG_
                printf("[MQTT_Client] Subscribe Topic Successful \n");    //TODO: Check if it recovers from negative 'e' status
                printf("------*********************************------\n");
                #endif
                if(e == 0){
                    conn_attempt_cnt = 0;
                    Task_sleep(800);
                    flash_read_val(FL_STATUS_1, &status, 1);
                    command_101(CONFIGURED);    //command_101(status);
                    Task_sleep(800);
                    get_wifi_info(&wifiInfo, mac);
                    Semaphore_reset(sem_mq_disc,0);
                    Semaphore_pend(sem_mq_disc, BIOS_WAIT_FOREVER);
                }

                l.led = LS_AC;
                l.state = true;
                Mailbox_post(m_led, &l, BIOS_NO_WAIT);

                if(mc_active) MQTTClient_delete(mc_active);
                safeTaskDelete(&t_mqtt_run);
                safeTaskDelete(&t_mqtt_send);

                delete_com_task();
            }
            else
            {
                if(mc_active) MQTTClient_delete(mc_active);
                safeTaskDelete(&t_mqtt_run);
            }
        }

        if (conn_attempt_cnt == 6)
        {
            printf("[MQTT_Client] Reboot after 6 connection attempt");
            printf("------*********************************------\n");
            Semaphore_post(sem_reboot);
        }

        //reset_CC3135();
        //wifi_init();
        #ifdef _DEBUG_
        printf("[MQTT_Client] Connection attempt no: %d\n", conn_attempt_cnt);
        printf("------*********************************------\n");
        #endif
        conn_attempt_cnt++;
        Task_sleep(10000);
    }
}


void mqtt_run(UArg a0, UArg a1)
{
    fdOpenSession(TaskSelf());
    MQTTClient_run(mc_active);
    fdCloseSession(TaskSelf());
}

static void mqtt_cb(int32_t event, void *metaData, uint32_t metaDateLen, void *data, uint32_t dataLen)
{
    msg_mqtt_t msg;
    uint32_t bufSizeReqd = 0;
    uint32_t topicOffset;
    uint32_t payloadOffset;
    struct publishMsgHeader msgHead;
    char *pubBuff = NULL;
    MQTTClient_RecvMetaDataCB *recvMetaData;

    switch ((MQTTClient_EventCB) event)
    {
        case MQTTClient_OPERATION_CB_EVENT:
        {
            switch (((MQTTClient_OperationMetaDataCB*) metaData)->messageType)
            {
                case MQTTCLIENT_OPERATION_CONNACK:
                {
                    uint16_t *ConnACK = (uint16_t*) data;
                    if (0 == (MQTTClientCbs_ConnackRC(*ConnACK)))
                    {
                        // Connected
                        set_mqtt_connected(true);
                    }
                    else
                    {
                        //Disconnected with error *ConnACK);
                        set_mqtt_connected(false);
                        Semaphore_post(sem_mq_disc);
                    }
                    break;
                }
            }
        break;
        }
        case MQTTClient_RECV_CB_EVENT:
        {
            // Message received
            recvMetaData = (MQTTClient_RecvMetaDataCB*) metaData;
            topicOffset = sizeof(struct publishMsgHeader);
            payloadOffset = sizeof(struct publishMsgHeader) + recvMetaData->topLen + 1;

            bufSizeReqd += sizeof(struct publishMsgHeader);
            bufSizeReqd += recvMetaData->topLen + 1;
            bufSizeReqd += dataLen + 1;
            pubBuff = mem_alloc(bufSizeReqd);

            msgHead.topicLen = recvMetaData->topLen;
            msgHead.payLen = dataLen;
            msgHead.retain = recvMetaData->retain;
            msgHead.dup = recvMetaData->dup;
            msgHead.qos = recvMetaData->qos;
            memcpy((void*) pubBuff, &msgHead, sizeof(struct publishMsgHeader));

            /* copying the topic name into the buffer                         */
            memcpy((void*) (pubBuff + topicOffset), (const void*) recvMetaData->topic, recvMetaData->topLen);
            memset((void*) (pubBuff + topicOffset + recvMetaData->topLen), '\0', 1);

            /* copying the payload into the buffer                            */
            memcpy((void*) (pubBuff + payloadOffset), (const void*) data, dataLen);
            memset((void*) (pubBuff + payloadOffset + dataLen), '\0', 1);

            msg.size_topic = msgHead.topicLen + 1;
            msg.size_content = msgHead.payLen + 1;

            msg.topic = mem_alloc(msg.size_topic);
            msg.content = mem_alloc(msg.size_content);

            memcpy(msg.topic, (void*) (pubBuff + topicOffset), msg.size_topic);
            memcpy(msg.content, (void*) (pubBuff + payloadOffset), msg.size_content);

            mem_free(pubBuff, bufSizeReqd);


            printf("\n\n------.:What the gateway receives?:.------\n");
            printf("[MQTT_CB] Topic: %s\n",msg.topic);
            printf("[MQTT_CB] Playload: %s\n",msg.content);
            printf("------*****************************------\n");

            mqtt_receive(&msg);
            break;
        }
        case MQTTClient_DISCONNECT_CB_EVENT:
        {
            mqtt_disconnect();
            break;
        }
    }
}

bool get_mqtt_connected()
{
    bool c;

    Semaphore_pend(sem_mq_connected, BIOS_WAIT_FOREVER);
    c = mq_connected;
    Semaphore_post(sem_mq_connected);
    return c;
}

static void set_mqtt_connected(bool c)
{
    Semaphore_pend(sem_mq_connected, BIOS_WAIT_FOREVER);
    mq_connected = c;
    Semaphore_post(sem_mq_connected);
}

void mqtt_receive(msg_mqtt_t *msg)
{
    uint8_t ulev;
    uint16_t i;
    bool auth;

    if (get_mqtt_connected())
    {
        if(msg->size_content && msg->size_topic)
        {
            Mailbox_post(m_frame_proc_mqtt, msg, BIOS_WAIT_FOREVER);
            //send_zigbee(msg->content, msg->topic, msg->size_topic, msg->size_content);
        }
    }
    else
    {
        mem_free(msg->content, msg->size_content);
        mem_free(msg->topic, msg->size_topic);
    }
}

void mqtt_send(UArg a0, UArg a1)
{
    msg_mqtt_t msg;
    int16_t i;

    // Definir un buffer est�tico m�ximo para el tema y el contenido
    static char static_topic[MAX_TOPIC_SIZE];
    static char static_content[MAX_CONTENT_SIZE];

    fdOpenSession(TaskSelf());

    while (1)
    {
        Mailbox_pend(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);

        memcpy(static_topic, msg.topic, msg.size_topic);    //TODO: Add some semaphore to make a clean copy
                static_topic[msg.size_topic] = '\0';

                memcpy(static_content, msg.content, msg.size_content);
                static_content[msg.size_content] = '\0';
        if (msg.content)
        {
            mem_free(msg.content, msg.size_content);
        }
        if (msg.topic)
        {
            mem_free(msg.topic, msg.size_topic);
        }

        #ifdef _DEBUG_
        printf("Topic: %s\n", static_topic);
        printf("Topic size: %d\n", msg.size_topic);
        printf("Content: %s\n", static_content);
        printf("Content size: %d\n", msg.size_content);
        #endif

        if (get_mqtt_connected())
        {
            if (msg.size_topic < MAX_TOPIC_SIZE && msg.size_content < MAX_CONTENT_SIZE)
            {
                i = MQTTClient_publish(mc_active, static_topic, strlen(static_topic), static_content, strlen(static_content), MQTT_QOS_1);
                Task_sleep(1200);
            }
            else
            {
                printf("Error: Message too large for static buffers.\n");
            }
        }
    }

    fdCloseSession(TaskSelf());
}

static void mqtt_disconnect()
{
    set_mqtt_connected(false);
    Semaphore_post(sem_mq_disc);
}

void send_zigbee(char *msg, char *topic, uint16_t size_topic, uint16_t size_msg)
{
    msg_rpc_t msg_send;
    char *aux1;
    char *aux2;
    char *cmd_id;
    uint8_t size_aux;
    uint16_t command = 0;
    uint8_t i = 0;

    db_event_t e;
    dbt_device *dev = NULL;

    char mac[18];
    db_get_mac(mac);

    aux2 = topic;
    for (i = 0; i < 3; i++)
    {
        aux1 = strchr(aux2, '/');
        aux2 = aux1;
        aux2++;
    }
    aux1 = strchr(aux2, '/');

    if (aux1 != NULL)
    {
        size_aux = aux1 - aux2;
        cmd_id = mem_alloc(size_aux);
        memcpy((void*) cmd_id, aux2, size_aux);
        memset((void*) (cmd_id + size_aux), '\0', 1);
        aux1++;

        if (strcmp(cmd_id, "config") == 0)
        {
            mem_free(cmd_id, size_aux);
            msg_send.data_length = size_msg;
            msg_send.data = mem_alloc(msg_send.data_length);
            memcpy((void*) msg_send.data, msg, size_msg);
            memset((void*) (msg_send.data + msg_send.data_length), '\0', 1);

            msg_send.type_length = strlen(aux1);
            msg_send.item_type = mem_alloc(msg_send.type_length);
            memcpy((void*) msg_send.item_type, aux1, strlen(aux1));
            memset((void*) (msg_send.item_type + msg_send.type_length), '\0', 1);

            msg_send.id_length = 0;
            msg_send.device_id = NULL;
            msg_send.read_write = 0;
        }
        else
        {
            mem_free(cmd_id, size_aux);
            aux2 = strchr(aux1, '/');
            if ((aux2 == NULL) == 0)
            {
                size_aux = aux2 - aux1;
                char id[size_aux];
                aux2++;
                strncpy(id, aux1, (size_aux));
                aux1 = id;
                aux1[size_aux] = '\0';

                msg_send.data_length = size_msg;
                msg_send.data = mem_alloc(msg_send.data_length);
                memcpy((void*) msg_send.data, msg, size_msg);
                memset((void*) (msg_send.data + msg_send.data_length), '\0', 1);

                msg_send.type_length = strlen(aux2);
                msg_send.item_type = mem_alloc(msg_send.type_length);
                memcpy((void*) msg_send.item_type, aux2, strlen(aux2));
                memset((void*) (msg_send.item_type + msg_send.type_length), '\0', 1);

                msg_send.id_length = strlen(aux1);
                msg_send.device_id = mem_alloc(msg_send.id_length);
                memcpy((void*) msg_send.device_id, aux1, strlen(aux1));
                memset((void*) (msg_send.device_id + msg_send.id_length), '\0', 1);

            }
            else
            {
                return;
            }
        }
        command = isWiFiCommand(msg_send.item_type);
        if(command)
        {
            Json_Handle json = NULL;
            Json_Handle hTemplate = NULL;

            char *network = NULL;
            uint16_t network_size = 0;

            char *ip_method = NULL;
            uint16_t ip_method_size = 0;

            int16_t ret = -1;

            ret = create_json(&json, &hTemplate, msg_send.item_type);
            ret = parse_json(json, msg_send.data, msg_send.data_length);

            network = get_jsonstring(json, "\"Network\"", &network_size);

            if(strcmp(network, "ETHERNET") == 0)
            {
                msg_mqtt_t msg;
                msg.buf = NULL;
                msg.content = NULL;
                msg.topic = NULL;
                msg.size_buf = 0;
                msg.size_content = 0;
                msg.size_topic = 0;

                switch(command)
                {
                case 301:
                    dev = (dbt_device*)get_element((dbt_generic**)&config_db.db_device, DEVICE);

                    ip_method = get_jsonstring(json, "\"Method\"", &ip_method_size);
                    dev->lan_dhcp = (strcmp(ip_method, "DHCP") == 0)? true : false;
                    free(ip_method);
                    if(dev->lan_dhcp == false)
                    {
                        uint8_t values[4];

                        ip_method = get_jsonstring(json, "\"IpAddress\"", &ip_method_size);
                        sscanf(ip_method, "%d.%d.%d.%d",&values[0],&values[1],&values[2],&values[3]);
                        dev->lan_ip = BUILD_UINT32(values[3], values[2], values[1], values[0]);
                        free(ip_method);

                        ip_method = get_jsonstring(json, "\"SubnetMask\"", &ip_method_size);
                        sscanf(ip_method, "%d.%d.%d.%d",&values[0],&values[1],&values[2],&values[3]);
                        dev->lan_sn = BUILD_UINT32(values[3], values[2], values[1], values[0]);
                        free(ip_method);

                        ip_method = get_jsonstring(json, "\"Gateway\"", &ip_method_size);
                        sscanf(ip_method, "%d.%d.%d.%d",&values[0],&values[1],&values[2],&values[3]);
                        dev->lan_gw = BUILD_UINT32(values[3], values[2], values[1], values[0]);
                        free(ip_method);

                        ip_method = get_jsonstring(json, "\"DnsServer\"", &ip_method_size);
                        sscanf(ip_method, "%d.%d.%d.%d",&values[0],&values[1],&values[2],&values[3]);
                        dev->lan_dns = BUILD_UINT32(values[3], values[2], values[1], values[0]);
                        free(ip_method);

                    }
                    set_element((dbt_generic**) &dev, DEVICE, false);

                    e = DBE_EXPORT;
                    if (Mailbox_post(m_db, &e, BIOS_WAIT_FOREVER) == false)
                    {
                        err_mailbox_full(__func__, m_db);
                    }
                    Task_sleep(3000);
                    Semaphore_post(sem_reboot);

                    break;

                case 304:

                    ip_method = get_jsonstring(json, "\"IpMethod\"", &ip_method_size);
                    if(strcmp(ip_method, "TRUE") == 0)
                    {
                            dev = (dbt_device*)get_element((dbt_generic**)&config_db.db_device, DEVICE);

                            msg.size_topic = 75;
                            msg.topic = mem_alloc(msg.size_topic);
                            sprintf(msg.topic, "smaway/%s/toapp/config/305\0", mac);

                            msg.size_content = 500;
                            msg.content = mem_alloc(msg.size_content);

                            sprintf(msg.content, "{\"Network\":\"ETHERNET\",\"Method\":\"%s\",\"IpAddress\":\"%d.%d.%d.%d\",\"SubnetMask\":\"%d.%d.%d.%d\",\"Gateway\":\"%d.%d.%d.%d\",\"DnsServer\":\"%d.%d.%d.%d\"}"
                                    , (dev->lan_dhcp)? "DHCP":"STATIC",
                                    (uint8_t) (dev->cur_ip >> 24), (uint8_t) (dev->cur_ip >> 16), (uint8_t) (dev->cur_ip >> 8), (uint8_t) (dev->cur_ip >> 0),
                                    (uint8_t) (dev->cur_sn >> 24), (uint8_t) (dev->cur_sn >> 16), (uint8_t) (dev->cur_sn >> 8), (uint8_t) (dev->cur_sn >> 0),
                                    (uint8_t) (dev->cur_gw >> 24), (uint8_t) (dev->cur_gw >> 16), (uint8_t) (dev->cur_gw >> 8), (uint8_t) (dev->cur_gw >> 0),
                                    (uint8_t) (dev->cur_dns >> 24), (uint8_t) (dev->cur_dns >> 16), (uint8_t) (dev->cur_dns >> 8), (uint8_t) (dev->cur_dns >> 0)
                                    );

                            mem_free(dev, sizeof(dbt_device));

                            sendToMqttCheckingMailbox(&msg);
                            //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
                    }
                    mem_free(ip_method, ip_method_size);
                    break;
                }
            }
            else
            {
                Mailbox_post(m_wifi_send, &msg_send, BIOS_WAIT_FOREVER);
            }
            mem_free(network,network_size);

            ret = Json_destroyObject(json);
            ret = Json_destroyTemplate(hTemplate);

        }
        else
        {
            //printf("Se da cuenta que %s no es un wifi command\n",msg_send.item_type);
            //printf("Playload: %s\n",msg_send.data );
            Mailbox_post(m_rpc_send, &msg_send, BIOS_WAIT_FOREVER);
        }
    }
    else
    {
        return;
    }
}

void command_101(uint8_t status)
{
    uint8_t counter = 0;
    uint8_t part_number = 1;
    uint8_t processed_devices = 0;
    uint8_t total_devices = 0;

    dbt_zdevice *zdev = NULL;
    char mac[18];
    db_get_mac(mac);

    zdev = (dbt_zdevice*) get_element((dbt_generic**) &config_db.db_zdevice, ZDEVICE);

    void *r;
    uint16_t r_size = 0;
    char component[150];

    msg_mqtt_t msg;

    while (zdev != NULL)
    {
        r = zdev->element->next;
        mem_free(zdev, sizeof(dbt_zdevice));
        zdev = (dbt_zdevice*)get_element((dbt_generic**) &r, ZDEVICE);

        if (zdev != NULL)
        {
            total_devices++;
        }
    }

    zdev = (dbt_zdevice*) get_element((dbt_generic**) &config_db.db_zdevice, ZDEVICE);

    while(total_devices > processed_devices)
    {
        msg.buf = NULL;
        msg.size_buf = 0;

        msg.size_topic = 75;
        msg.topic = mem_alloc(msg.size_topic);
        sprintf(msg.topic, "smaway/%s/tocloud/config/101\0", mac);

        msg.size_content = 500;
        msg.content = mem_alloc(msg.size_content);

        uint8_t remaining_devices = total_devices - processed_devices;
        uint8_t is_last = (remaining_devices <= 2) ? 1 : 0;

        sprintf(msg.content, "{\"Part\": %d, \"Last\": %d, \"Status\": %d, \"Components\": [", part_number, is_last, status);

        while ((zdev != NULL) && (counter < 2))
        {
            r = zdev->element->next;
            mem_free(zdev, sizeof(dbt_zdevice));
            zdev = (dbt_zdevice*)get_element((dbt_generic**) &r, ZDEVICE);

            if (zdev != NULL)
            {
                sprintf(component, "{\"ComponentStatus\":\"%s\",\"DeviceId\":\"%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\",\"Model\":\"%s\",\"FirmwareVersion\":\"%d\"}", (zdev->status) ? "ONLINE" : "OFFLINE",
                        BREAK_UINT64(zdev->ieee_address, 7),
                        BREAK_UINT64(zdev->ieee_address, 6),
                        BREAK_UINT64(zdev->ieee_address, 5),
                        BREAK_UINT64(zdev->ieee_address, 4),
                        BREAK_UINT64(zdev->ieee_address, 3),
                        BREAK_UINT64(zdev->ieee_address, 2),
                        BREAK_UINT64(zdev->ieee_address, 1),
                        BREAK_UINT64(zdev->ieee_address, 0), zdev->model_id, zdev->firmware_version);
                if ((msg.size_content - strlen(msg.content)) > strlen(component))
                {
                    strcat(msg.content, component);
                }
                else
                {
                    r_size = msg.size_content + 500;
                    r = mem_alloc(r_size);
                    memcpy(r, msg.content, strlen(msg.content));
                    mem_free(msg.content, msg.size_content);
                    msg.content = r;
                    msg.size_content = r_size;

                    strcat(msg.content, component);
                }
                processed_devices++;
                counter++;
                if ((zdev->element->next) && (counter < 2))
                {
                    strcat(msg.content, ",");
                }
            }
        }

        counter = 0;

        sprintf(component, "],\"IsInit\": %s,\"BoxVersion\": %d,\"LogLevel\": %d }", (IsInit) ? "true" : "false", vfw, get_loglevel());

        if ((msg.size_content - strlen(msg.content)) > strlen(component))
        {
            strcat(msg.content, component);
        }
        else
        {
            r_size = msg.size_content + 30;
            r = mem_alloc(r_size);
            memcpy(r, msg.content, strlen(msg.content));
            mem_free(msg.content, msg.size_content);
            msg.content = r;
            msg.size_content = r_size;

            strcat(msg.content, component);
        }

        IsInit = false;

        sendToMqttCheckingMailbox(&msg);
        //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
        part_number++;
    }
    if(total_devices == 0)
    {
        msg.buf = NULL;
        msg.size_buf = 0;

        msg.size_topic = 75;
        msg.topic = mem_alloc(msg.size_topic);
        sprintf(msg.topic, "smaway/%s/tocloud/config/101\0", mac);

        msg.size_content = 500;
        msg.content = mem_alloc(msg.size_content);

        uint8_t remaining_devices = total_devices - processed_devices;
        uint8_t is_last = (remaining_devices <= 2) ? 1 : 0;

        sprintf(msg.content, "{\"Part\": %d, \"Last\": %d, \"Status\": %d, \"Components\": [],\"IsInit\": %s,\"BoxVersion\": %d,\"LogLevel\": %d }", part_number, is_last, status, (IsInit) ? "true" : "false", vfw, get_loglevel());

        IsInit = false;

        sendToMqttCheckingMailbox(&msg);
        //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
    }

}

uint16_t isWiFiCommand(const char* input) {
    if (input == NULL) {
        return false;
    }

    char* endPtr;

    uint16_t number = strtol(input, &endPtr, 10);

    if (*endPtr != '\0' || input == endPtr) {
        return false;
    }

    if (number >= 300) {
        return number;
    }

    return false;
}

void appsvr_connection_mqtt(UArg a0, UArg a1)
{
//    msg_rpc_t msg_send;
    msg_mqtt_t msg_send;
/*    SOCKET currSocket = NULL;
    uint16_t currentStartIdx = 0;
    int16_t frameValReadStrLen;
    char buffer[MSG_MAXSIZE_GRAL], *endCharPtr;     //TODO: Update buffer size to actual value
    endCharPtr = NULL;
    bool dc = false, auth = MQTT_CLOUD_ISAUTH;
    uint8_t userLevel = MQTT_CLOUD_USERLEVEL;
    msg_dbev_t msg;
    msg_event_t ev_out;
    uint16_t id_user_auth;
*/
    // Open the file session


    fdOpenSession(TaskSelf());

    while(1)
    {

        Mailbox_pend(m_frame_proc_mqtt, &msg_send, BIOS_WAIT_FOREVER);

        if(!msg_send.size_content){
            continue;
        }

        send_zigbee(msg_send.content, msg_send.topic, msg_send.size_topic, msg_send.size_content);

        mem_free(msg_send.content, msg_send.size_content);
        mem_free(msg_send.topic, msg_send.size_topic);
    }
}

void sendToMqttCheckingMailbox(msg_mqtt_t* msg)
{
    while(checkChannelMailbox(/*CH_MQTT*/)>MQTT_MAILBOX_WAIT_EMPTY_LEN)
        Task_sleep(MQTT_MAILBOX_DELAYMS_EMPTY);

    sendToMqttMailbox(msg);
}

bool sendToMqttCheckingMailboxNoWait(msg_mqtt_t* msg)
{
    while(checkChannelMailbox(/*CH_MQTT*/)>MQTT_MAILBOX_WAIT_EMPTY_LEN)
        Task_sleep(MQTT_MAILBOX_DELAYMS_EMPTY);

    return sendToMqttMailboxNoWait(msg);
}

void sendToMqttMailbox(msg_mqtt_t* msg)
{
    Mailbox_post(m_mqtt_send, msg, BIOS_WAIT_FOREVER);   //TODO: Check if return value can be used
}

bool sendToMqttMailboxNoWait(msg_mqtt_t* msg)
{
    return Mailbox_post(m_mqtt_send, msg, BIOS_NO_WAIT);   //TODO: Check if return value can be used

}

UInt checkChannelMailbox(/*sendchannel_type channel*/)
{
    return checkMqttMailbox();

    /*
    switch(channel)
    {
        #ifdef _MQTT_
        case CH_MQTT:
            return checkMqttPendingSendMessages();
        #endif
        case CH_ALL:
            return checkCommSystemMailbox();
        default:
            return 0;
    }*/
}

UInt checkMqttMailbox()
{
    return Mailbox_getNumPendingMsgs(m_mqtt_send);
}
