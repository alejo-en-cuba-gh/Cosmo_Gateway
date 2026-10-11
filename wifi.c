/*
 * wifi.c
 *
 *  Created on: 12 Dec 2024
 *      Author: Lzaro
 */

#include "wifi.h"

Semaphore_Handle sem_wifi_ready, sem_role_wifi, sem_sta_ap_ready;

int wifi_init() {
    int32_t ret;
    pthread_t spawn_thread = (pthread_t) NULL;
    pthread_attr_t pattrs_spawn;
    struct sched_param pri_param;
    static uint8_t vie = 0;
    uint8_t j;

    Semaphore_pend(sem_spiffs_mount, BIOS_WAIT_FOREVER);
    Semaphore_post(sem_spiffs_mount);

    WiFi_init();

    flash_read_val(TOKEN, &token, 4);

    if (!vie) {
        pthread_attr_init(&pattrs_spawn);
        pri_param.sched_priority = TASK_PRIO_MEDIUM;
        pthread_attr_setschedparam(&pattrs_spawn, &pri_param);
        pthread_attr_setstacksize(&pattrs_spawn, TASK_NORMAL);
        pthread_attr_setdetachstate(&pattrs_spawn, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&spawn_thread, &pattrs_spawn, sl_Task, NULL);
        if (ret != 0) {
            return ret;
        }
        vie = 1;
    }

    Network_IF_ResetMCUStateMachine();
    Network_IF_DeInitDriver();

    ret = sl_Start(0, 0, 0);
    if (ret < 0) {
        printf("Error starting the device: %ld\n", ret);
    }

    Task_sleep(500);
    read_wifi_role();

    for (j = 0; j < 5; j++) {
        if (initialize_access_point() == 0)
            break;
    }

    Task_sleep(10000);

    if (get_role_wifi() != ROLE_STA) {
        write_wifi_role(ROLE_AP);
    } else {
        role_wifi = ROLE_AP;
        credentials = get_wifi_credentials();
        initialize_station(credentials.ssid, credentials.password, credentials.securityType);
        Task_sleep(5000);
    }

    Semaphore_post(sem_wifi_ready);

    return ret;
}


void wifi_connect(UArg a0, UArg a1)
{
    WifiSTATUS wifiInfo;
    char mac[18];
    db_get_mac(mac);

    Semaphore_pend(sem_wifi_ready, BIOS_WAIT_FOREVER);
    // Release the read semaphore
    Semaphore_post(sem_wifi_ready);

    while(1)
    {
        Semaphore_pend(sem_sta_ap_ready, BIOS_WAIT_FOREVER);
        if(get_role_wifi() == ROLE_STA)
        {
            if(wifi_connected == false)
            {
                initialize_station(credentials.ssid, credentials.password, credentials.securityType);
                if(wifi_connected)
                {
                    if (get_mqtt_connected())
                    {
                        get_wifi_info(&wifiInfo, mac);
                    }
                }
            }
        }
        Semaphore_post(sem_sta_ap_ready);
        Task_sleep(20000);
    }
}

void Wifi_command(UArg a0, UArg a1)
{
    msg_wifi_t msg_send = { 0 };

    char mac[18];
    db_get_mac(mac);
    int16_t ret = -1;
    Json_Handle hTemplate;
    Json_Handle hObject;
    WifiSTATUS wifiInfo;
    WiFiNetworkInfo info_wifi;
    int j;

    while(1)
    {
        Mailbox_pend(m_wifi_send, &msg_send, BIOS_WAIT_FOREVER);
        if (strcmp(msg_send.item_type, "304") == 0)
        {
            ret = create_json(&hObject, &hTemplate, msg_send.item_type);
            ret = parse_json(hObject, msg_send.data, msg_send.data_length);

            uint16_t accessible_wiFi_networks_size;
            uint16_t ip_method_size;
            uint16_t saved_profile_size;
            uint16_t wifi_status_size;
            char *accessible_wiFi_networks = NULL;
            char *ip_method = NULL;
            char *saved_profile = NULL;
            char *wifi_status = NULL;

            accessible_wiFi_networks = get_jsonstring(hObject, "\"AccessibleWiFiNetworks\"", &accessible_wiFi_networks_size);
            ip_method                = get_jsonstring(hObject, "\"IpMethod\"", &ip_method_size);
            saved_profile            = get_jsonstring(hObject, "\"SavedProfile\"", &saved_profile_size);
            wifi_status              = get_jsonstring(hObject, "\"WifiStatus\"", &wifi_status_size);

            ret = Json_destroyObject(hObject);
            ret = Json_destroyTemplate(hTemplate);

            if (strcmp(accessible_wiFi_networks, "TRUE") == 0)
            {
                scan_wifi_networks(mac);
                if(get_role_wifi() == ROLE_AP)
                {
                    Semaphore_pend(sem_sta_ap_ready, BIOS_WAIT_FOREVER);
                    for(j = 0; j < 5; j++)
                    {
                    if (initialize_access_point() == 0)
                        break;
                    }
                    Semaphore_post(sem_sta_ap_ready);
                }

            }
            free(accessible_wiFi_networks);
            if (strcmp(ip_method, "TRUE") == 0)
            {
                get_wifi_acquisition_method(&info_wifi, mac);
            }
            free(ip_method);
            if (strcmp(saved_profile, "TRUE") == 0)
            {

            }
            free(saved_profile);
            if (strcmp(wifi_status, "TRUE") == 0)
            {
                Task_sleep(1000);
                get_wifi_info(&wifiInfo, mac);
            }
            free(wifi_status);

        }
        else if (strcmp(msg_send.item_type, "302") == 0)
        {
            ret = create_json(&hObject, &hTemplate, msg_send.item_type);
            ret = parse_json(hObject, msg_send.data, msg_send.data_length);

            uint16_t SSID_size;
            uint16_t Password_size;
            uint16_t SecurityType_size;
            char *SSID = NULL;
            char *Password = NULL;
            char *SecurityType = NULL;

            SSID         = get_jsonstring(hObject, "\"SSID\"", &SSID_size);
            Password     = get_jsonstring(hObject, "\"Password\"", &Password_size);
            SecurityType = get_jsonstring(hObject, "\"SecurityType\"", &SecurityType_size);

            int SecurityType_ST = get_security_type_from_string(SecurityType);
            memset(&credentials, 0, sizeof(WiFiCredentials));
            strncpy(credentials.ssid, SSID, SSID_size);
            strncpy(credentials.password, Password, Password_size);
            credentials.securityType = SecurityType_ST;
            store_wifi_credentials(SSID, Password, SecurityType_ST);
            Task_sleep(3000);
            Semaphore_pend(sem_sta_ap_ready, BIOS_WAIT_FOREVER);
            initialize_station(SSID, Password, SecurityType_ST);
            Semaphore_post(sem_sta_ap_ready);
            get_wifi_info(&wifiInfo, mac);

            ret = Json_destroyObject(hObject);
            ret = Json_destroyTemplate(hTemplate);

            free(SSID);
            free(Password);
            free(SecurityType);

        }
        else if (strcmp(msg_send.item_type, "300") == 0)
        {
            write_wifi_role(ROLE_AP);
            Semaphore_pend(sem_sta_ap_ready, BIOS_WAIT_FOREVER);
            for(j = 0; j < 5; j++)
            {
            if (initialize_access_point() == 0)
                break;
            }
            Semaphore_post(sem_sta_ap_ready);
            get_wifi_info(&wifiInfo, mac);
        }
        else if (strcmp(msg_send.item_type, "301") == 0)
        {
            ret = create_json(&hObject, &hTemplate, msg_send.item_type);
            ret = parse_json(hObject, msg_send.data, msg_send.data_length);

            uint16_t Method_size;
            uint16_t IpAddress_size;
            uint16_t SubnetMask_size;
            uint16_t Gateway_size;
            uint16_t DnsServer_size;
            char *Method = NULL;
            char *IpAddress = NULL;
            char *SubnetMask = NULL;
            char *Gateway = NULL;
            char *DnsServer = NULL;

            Method     = get_jsonstring(hObject, "\"Method\"", &Method_size);
            IpAddress                = get_jsonstring(hObject, "\"IpAddress\"", &IpAddress_size);
            SubnetMask            = get_jsonstring(hObject, "\"SubnetMask\"", &SubnetMask_size);
            Gateway                = get_jsonstring(hObject, "\"Gateway\"", &Gateway_size);
            DnsServer            = get_jsonstring(hObject, "\"DnsServer\"", &DnsServer_size);

            set_wifi_acquisition_method(Method, IpAddress, SubnetMask, Gateway, DnsServer);
            get_wifi_acquisition_method(&info_wifi, mac);
            initialize_wifi_ST_AP(mac);

            ret = Json_destroyObject(hObject);
            ret = Json_destroyTemplate(hTemplate);

            free(Method);
            free(IpAddress);
            free(SubnetMask);
            free(Gateway);
            free(DnsServer);
        }

    }
}

int initialize_wifi_ST_AP(char mac[18])
{
    uint8_t j;
    WifiSTATUS wifiInfo;
    uint8_t status;

    if (get_role_wifi() == ROLE_STA)
    {
        Semaphore_pend(sem_sta_ap_ready, BIOS_WAIT_FOREVER);
        status = initialize_station(credentials.ssid, credentials.password, credentials.securityType);
        Semaphore_post(sem_sta_ap_ready);
    }
    else if (get_role_wifi() == ROLE_AP)
    {
        Semaphore_pend(sem_sta_ap_ready, BIOS_WAIT_FOREVER);
        for(j = 0; j < 5; j++)
        {
            status = initialize_access_point();
        if (status == 0)
            break;
        }
        Semaphore_post(sem_sta_ap_ready);
    }
    else
    {
        printf("Role UNKNOWN");
    }
    get_wifi_info(&wifiInfo, mac);
    return status;
}

int get_security_type_from_string(const char *securityType)
{
    if (strcmp(securityType, "OPEN") == 0)
    {
        return SL_WLAN_SEC_TYPE_OPEN;
    }
    else if (strcmp(securityType, "WEP") == 0)
    {
        return SL_WLAN_SEC_TYPE_WEP;
    }
    else if (strcmp(securityType, "WEP_SHARED") == 0)
    {
        return SL_WLAN_SEC_TYPE_WEP_SHARED;
    }
    else if (strcmp(securityType, "WPA_WPA2") == 0)
    {
        return SL_WLAN_SEC_TYPE_WPA_WPA2;
    }
    else if (strcmp(securityType, "WPA2_PLUS") == 0)
    {
        return SL_WLAN_SEC_TYPE_WPA2_PLUS;
    }
    else if (strcmp(securityType, "WPA3") == 0)
    {
        return SL_WLAN_SEC_TYPE_WPA3;
    }
    else if (strcmp(securityType, "WPA_ENT") == 0)
    {
        return SL_WLAN_SEC_TYPE_WPA_ENT;
    }
    else if (strcmp(securityType, "WPS_PBC") == 0)
    {
        return SL_WLAN_SEC_TYPE_WPS_PBC;
    }
    else if (strcmp(securityType, "WPS_PIN") == 0)
    {
        return SL_WLAN_SEC_TYPE_WPS_PIN;
    }
    else if (strcmp(securityType, "WPA_PMK") == 0)
    {
        return SL_WLAN_SEC_TYPE_WPA_PMK;
    }
    else
    {
        return -1;
    }
}


void scan_wifi_networks(char mac[18])
{
    SlWlanNetworkEntry_t netEntries[20];
    int16_t status;
    char jsonBuffer[500]; // Buffer size for the JSON
    size_t jsonBufferSize = sizeof(jsonBuffer);
    int offset = 0;        // Current position in the buffer
    msg_mqtt_t msg;

    Task_sleep(500);

    // Configure WiFi module in station mode
    if(get_role_wifi() != ROLE_STA)
    {
        reset_CC3135();
        Task_sleep(500);
        status = sl_WlanSetMode(ROLE_STA);
        status = sl_Stop(SL_STOP_TIMEOUT);
        sl_Start(NULL, NULL, NULL);
    }
    Task_sleep(500);
    // Enable scan policy
    status = sl_WlanPolicySet(SL_WLAN_POLICY_SCAN, SL_WLAN_SCAN_POLICY_EN(1), NULL, 0);
    if (status < 0)
    {
        printf("Error: Failed to enable scan policy.\n");
        return;
    }

    // Wait for scan results
    printf("Waiting for scan results...\n");
    Task_sleep(1000);

    // Try to get the network list, retrying if necessary
    int retries = 0;
    while (retries < MAX_RETRIES)
    {
        Task_sleep(500);
        status = sl_WlanGetNetworkList(0, 20, netEntries);

        if (status == SL_ERROR_WLAN_GET_NETWORK_LIST_EAGAIN)
        {
            printf("Error: Temporary issue, retrying to get WiFi network list... Attempt %d/%d\n", retries + 1, MAX_RETRIES);
            retries++;
            Task_sleep(RETRY_DELAY_MS);
        }
        else if (status < 0)
        {
            printf("Error: Failed to get WiFi network list (code %d).\n", status);
            return;
        }
        else
        {
            break;
        }
    }

    if (status == SL_ERROR_WLAN_GET_NETWORK_LIST_EAGAIN)
    {
        printf("Error: Failed to get network list after %d retries.\n", retries);
        return;
    }

    int i;
    for (i = 0; i < status; i++)
    {
        offset = 0;
        // Map SecurityInfo to string
        const char *securityTypeStr = "";
        switch (netEntries[i].SecurityInfo)
        {
            case SL_WLAN_SEC_TYPE_OPEN:
                securityTypeStr = "OPEN";
                break;
            case SL_WLAN_SEC_TYPE_WEP:
                securityTypeStr = "WEP";
                break;
            case SL_WLAN_SEC_TYPE_WEP_SHARED:
                securityTypeStr = "WEP_SHARED";
                break;
            case SL_WLAN_SEC_TYPE_WPA_WPA2:
                securityTypeStr = "WPA_WPA2";
                break;
            case SL_WLAN_SEC_TYPE_WPA2_PLUS:
                securityTypeStr = "WPA2_PLUS";
                break;
            case SL_WLAN_SEC_TYPE_WPA3:
                securityTypeStr = "WPA3";
                break;
            case SL_WLAN_SEC_TYPE_WPA_ENT:
                securityTypeStr = "WPA_ENT";
                break;
            case SL_WLAN_SEC_TYPE_WPS_PBC:
                securityTypeStr = "WPS_PBC";
                break;
            case SL_WLAN_SEC_TYPE_WPS_PIN:
                securityTypeStr = "WPS_PIN";
                break;
            case SL_WLAN_SEC_TYPE_WPA_PMK:
                securityTypeStr = "WPA_PMK";
                break;
            default:
                securityTypeStr = "WPA_WPA2";
                break;
        }

        offset += snprintf(jsonBuffer + offset, jsonBufferSize - offset,
                           "{ \"SSID\": \"%s\", \"SignalStrength\": %d, \"SecurityType\": \"%s\" }",
                           netEntries[i].Ssid, netEntries[i].Rssi, securityTypeStr);

        if (offset < 0 || offset >= jsonBufferSize)
        {
            printf("Error: JSON buffer overflow.\n");
            return;
        }

        printf("Found network: SSID=%s, RSSI=%d, SecurityType=%s\n",
               netEntries[i].Ssid, netEntries[i].Rssi, securityTypeStr);


        // Ensure JSON is null-terminated
        jsonBuffer[offset] = '\0';

        // Prepare the MQTT message
        msg.size_content = offset + 1; // Include null terminator
        msg.content = (char *)malloc(msg.size_content);
        if (!msg.content)
        {
            printf("Error: Insufficient memory for msg.content.\n");
            return;
        }
        memcpy(msg.content, jsonBuffer, msg.size_content);

        msg.size_topic = 50;
        msg.topic = (char *)malloc(msg.size_topic);
        if (!msg.topic)
        {
            printf("Error: Insufficient memory for msg.topic.\n");
            free(msg.content);
            return;
        }
        snprintf(msg.topic, msg.size_topic, "smaway/%s/toapp/config/308\0", mac);

        // Send the message via MQTT
        sendToMqttCheckingMailbox(&msg);
        //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);

    }
}




int get_wifi_acquisition_method(WiFiNetworkInfo *config, char mac[18])
{
    unsigned char configOpt;
    unsigned char len = sizeof(SlNetCfgIpV4Args_t);
    SlNetCfgIpV4Args_t ipV4Config;
    char jsonBuffer[225];
    size_t jsonBufferSize = sizeof(jsonBuffer);
    msg_mqtt_t msg;

    Task_sleep(500);

    int ret = sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &configOpt, &len, (unsigned char *)&ipV4Config);
    if (ret < 0)
    {
        return ret;
    }

    int offset = 0;

    if (configOpt == SL_NETCFG_ADDR_DHCP)
    {
        config->Method = DHCP;

        // Build JSON for DHCP
        offset = snprintf(jsonBuffer, jsonBufferSize,
                          "{ \"Method\": \"DHCP\", \"IpAddress\": \"\", \"SubnetMask\": \"\", \"Gateway\": \"\", \"DnsServer\": \"\" }");
    }
    else if (configOpt == SL_NETCFG_ADDR_STATIC)
    {
        config->Method = STATIC;

        snprintf(config->IpAddress, sizeof(config->IpAddress), "%lu.%lu.%lu.%lu",
                 SL_IPV4_BYTE(ipV4Config.Ip, 3), SL_IPV4_BYTE(ipV4Config.Ip, 2),
                 SL_IPV4_BYTE(ipV4Config.Ip, 1), SL_IPV4_BYTE(ipV4Config.Ip, 0));

        snprintf(config->SubnetMask, sizeof(config->SubnetMask), "%lu.%lu.%lu.%lu",
                 SL_IPV4_BYTE(ipV4Config.IpMask, 3), SL_IPV4_BYTE(ipV4Config.IpMask, 2),
                 SL_IPV4_BYTE(ipV4Config.IpMask, 1), SL_IPV4_BYTE(ipV4Config.IpMask, 0));

        snprintf(config->Gateway, sizeof(config->Gateway), "%lu.%lu.%lu.%lu",
                 SL_IPV4_BYTE(ipV4Config.IpGateway, 3), SL_IPV4_BYTE(ipV4Config.IpGateway, 2),
                 SL_IPV4_BYTE(ipV4Config.IpGateway, 1), SL_IPV4_BYTE(ipV4Config.IpGateway, 0));

        snprintf(config->DnsServer, sizeof(config->DnsServer), "%lu.%lu.%lu.%lu",
                 SL_IPV4_BYTE(ipV4Config.IpDnsServer, 3), SL_IPV4_BYTE(ipV4Config.IpDnsServer, 2),
                 SL_IPV4_BYTE(ipV4Config.IpDnsServer, 1), SL_IPV4_BYTE(ipV4Config.IpDnsServer, 0));

        offset = snprintf(jsonBuffer, jsonBufferSize,
                          "{ \"Method\": \"STATIC\", \"IpAddress\": \"%s\", \"SubnetMask\": \"%s\", \"Gateway\": \"%s\", \"DnsServer\": \"%s\" }",
                          config->IpAddress, config->SubnetMask, config->Gateway, config->DnsServer);
    }
    else
    {
        return -1;
    }

    if (offset < 0 || (size_t)offset >= jsonBufferSize)
    {
        printf("Error: JSON buffer overflow.\n");
        return -1;
    }

    // Ensure JSON is null-terminated
    jsonBuffer[offset] = '\0';

    // Allocate memory for MQTT message
    msg.size_content = offset + 1; // Include null terminator
    msg.content = (char *)malloc(msg.size_content);
    if (!msg.content)
    {
        printf("Error: Insufficient memory for msg.content.\n");
        return -1;
    }
    memcpy(msg.content, jsonBuffer, msg.size_content);

    msg.size_topic = 50;
    msg.topic = (char *)malloc(msg.size_topic);
    if (!msg.topic)
    {
        printf("Error: Insufficient memory for msg.topic.\n");
        free(msg.content);
        return -1;
    }
    snprintf(msg.topic, msg.size_topic, "smaway/%s/toapp/config/305\0", mac);

    // Send the message via MQTT
    sendToMqttCheckingMailbox(&msg);
    //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
    return 0;
}



int get_active_profile(ActiveProfile *profile)
{
    unsigned char configOpt;
    unsigned char len;
    SlWlanSecParams_t secParams;
    signed char ssid[33];
    signed short ssidLen = 0;
    SlWlanGetSecParamsExt_t pSecExtParams;

    Task_sleep(500);

    // Retrieve the SSID of the active profile
    len = sizeof(ssid);
    int ret = sl_WlanProfileGet(0, ssid, &ssidLen, NULL, &secParams, &pSecExtParams, NULL);
    if (ret < 0)
    {
       return ret; // Error retrieving the profile
    }

    strncpy(profile->SSID, (char *)ssid, ssidLen);
    profile->SSID[ssidLen] = '\0';

    // Determine the security type
    switch (secParams.Type)
    {
       case SL_WLAN_SEC_TYPE_OPEN:
           profile->SecType = OPEN;
           break;
       case SL_WLAN_SEC_TYPE_WEP:
           profile->SecType = WEP;
           break;
       case SL_WLAN_SEC_TYPE_WPA_WPA2:
           profile->SecType = WPA_WPA2;
           break;
       case SL_WLAN_SEC_TYPE_WPA2_PLUS:
           profile->SecType = WPA2_PLUS;
           break;
       case SL_WLAN_SEC_TYPE_WPA3:
           profile->SecType = WPA3;
           break;
       case SL_WLAN_SEC_TYPE_WPA_ENT:
           profile->SecType = WPA_ENT;
           break;
       case SL_WLAN_SEC_TYPE_WPS_PBC:
           profile->SecType = WPS_PBC;
           break;
       case SL_WLAN_SEC_TYPE_WPS_PIN:
           profile->SecType = WPS_PIN;
           break;
       case SL_WLAN_SEC_TYPE_WPA_PMK:
           profile->SecType = WPA_PMK;
           break;
       default:
           return -1; // Unknown security type
    }

    // Retrieve the password if applicable for the security type
    if (secParams.KeyLen > 0)
    {
       strncpy(profile->Password, (char *)secParams.Key, secParams.KeyLen);
       profile->Password[secParams.KeyLen] = '\0';
    }
    else
    {
       strcpy(profile->Password, "");
    }

    return 0; // Success
}

int set_wifi_acquisition_method(const char* method, const char* ipAddress, const char* subnetMask, const char* gateway, const char* dnsServer)
{
    int32_t status;
    SlNetCfgIpV4Args_t ipV4Config;

    // Set IP acquisition method (STATIC or DHCP)
    if (strcmp(method, "DHCP") == 0)
    {
        Task_sleep(500);

        status = sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE, SL_NETCFG_ADDR_DHCP, 0, NULL);
        if (status < 0)
        {
            printf("Error configuring DHCP: %ld\n", status);
            return status;
        }
    }
    else if (strcmp(method, "STATIC") == 0)
    {
        memset(&ipV4Config, 0, sizeof(SlNetCfgIpV4Args_t));

        Task_sleep(500);

        // Configure static IP parameters
        ipV4Config.Ip = htonl(inet_addr(ipAddress));
        ipV4Config.IpMask = htonl(inet_addr(subnetMask));
        ipV4Config.IpGateway = htonl(inet_addr(gateway));
        ipV4Config.IpDnsServer = htonl(inet_addr(dnsServer));
        status = sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE, SL_NETCFG_ADDR_STATIC, sizeof(SlNetCfgIpV4Args_t), (uint8_t*)&ipV4Config);
        if (status < 0)
        {
            printf("Error configuring static IP: %ld\n", status);
            return status;
        }
    }
    else
    {
        printf("Invalid method: %s\n", method);
        return -1;
    }

    Task_sleep(500);

    // Restart to apply changes
    status = sl_Stop(SL_STOP_TIMEOUT);
    if (status < 0)
    {
        printf("Error stopping device: %ld\n", status);
        return status;
    }

    status = sl_Start(0, 0, 0);
    if (status < 0)
    {
        printf("Error starting device: %ld\n", status);
        return status;
    }
    printf("Configuration completed successfully.\n");
    return 0;
}

void update_wifi_status()
{
    SlWlanConnStatusParam_t connStatus;
    unsigned char len;
    len = sizeof(SlWlanConnStatusParam_t);
    int status;
    // Get the connection status
    Task_sleep(3000);
    status = sl_WlanGet(SL_WLAN_CONNECTION_INFO, 0, &len, (unsigned char*)&connStatus);
    // Check if the function executed successfully
    if (status < 0)
    {
        // Error handling, if necessary
        return;
    }

    // Determine the connection mode and status
    if (get_role_wifi() == ROLE_STA)
    {
        if (connStatus.ConnStatus == SL_WLAN_CONNECTED_STA)
        {
            wifi_connected = true;
        }
        else
        {
            wifi_connected = false;
        }
    }
}

int get_wifi_info(WifiSTATUS* wifiInfo, char mac[18])
{
    SlNetCfgIpV4Args_t ipV4Config;
    unsigned char configOpt;
    unsigned char len;
    SlWlanSecParams_t secParams;
    SlWlanConnStatusParam_t connStatus;
    unsigned char macAddressVal[6];
    unsigned char macAddressLen = sizeof(macAddressVal);
    char jsonBuffer[512]; // Appropriate size for JSON
    size_t jsonBufferSize = sizeof(jsonBuffer);
    msg_mqtt_t msg;

    // Initialize WifiSTATUS structure
    memset(wifiInfo, 0, sizeof(WifiSTATUS));

    Task_sleep(500);
    len = sizeof(SlWlanConnStatusParam_t);
    // Get the connection status
    sl_WlanGet(SL_WLAN_CONNECTION_INFO, 0, &len, (unsigned char*)&connStatus);

    // Determine the connection mode and status
    if (get_role_wifi() == ROLE_STA)
    {
        strcpy(wifiInfo->mode, "STATION");
        if (connStatus.ConnStatus == SL_WLAN_CONNECTED_STA)
        {
            strcpy(wifiInfo->wifiStatus, "CONNECTED");
            wifi_connected = true;
        }
        else
        {
            strcpy(wifiInfo->wifiStatus, "DISCONNECTED");
            wifi_connected = false;
        }
        // Get the SSID
        strcpy(wifiInfo->ssid, credentials.ssid);
    }
    else if (get_role_wifi() == ROLE_AP)
    {
        strcpy(wifiInfo->mode, "ACCESSPOINT");
        wifi_connected = false;
        strcpy(wifiInfo->wifiStatus, "ACTIVE");
        // Get the SSID
        strcpy(wifiInfo->ssid, SSID_NAME_AP);
    }
    else
    {
        strcpy(wifiInfo->mode, "UNKNOWN");
        strcpy(wifiInfo->wifiStatus, "UNKNOWN");
    }

    // Get the signal strength (only in STATION mode)
    if (strcmp(wifiInfo->mode, "STATION") == 0)
    {

        Task_sleep(500);

        SlWlanGetRxStatResponse_t rxStat;
        sl_WlanRxStatGet(&rxStat, 0);
        wifiInfo->signalStrength = rxStat.AvarageMgMntRssi;
    }
    else
    {
        wifiInfo->signalStrength = 0;
    }

    Task_sleep(500);

    // Get the IP address
    len = sizeof(ipV4Config);
    sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &configOpt, &len, (unsigned char*)&ipV4Config);
    snprintf(wifiInfo->ipAddress, sizeof(wifiInfo->ipAddress), "%d.%d.%d.%d",
             SL_IPV4_BYTE(ipV4Config.Ip, 3), SL_IPV4_BYTE(ipV4Config.Ip, 2),
             SL_IPV4_BYTE(ipV4Config.Ip, 1), SL_IPV4_BYTE(ipV4Config.Ip, 0));

    // Get the number of connected devices (only in ACCESSPOINT mode)
    if (strcmp(wifiInfo->mode, "ACCESSPOINT") == 0)
    {
        wifiInfo->connectedDevices = 0; // No equivalent API, should be implemented if necessary
    }
    else
    {
        wifiInfo->connectedDevices = 0; // Not applicable in STATION mode
    }

    Task_sleep(500);

    // Get the MAC address
    sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, 0, &macAddressLen, macAddressVal);
    snprintf(wifiInfo->macAddress, sizeof(wifiInfo->macAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
             macAddressVal[0], macAddressVal[1], macAddressVal[2],
             macAddressVal[3], macAddressVal[4], macAddressVal[5]);

    // Create JSON with the information
    int offset = snprintf(jsonBuffer, jsonBufferSize,
                          "{ \"Mode\": \"%s\", \"WiFiStatus\": \"%s\", \"SSID\": \"%s\", "
                          "\"SignalStrength\": %d, \"IpAddress\": \"%s\", \"ConnectedDevices\": %d, "
                          "\"MacAddress\": \"%s\" }",
                          wifiInfo->mode, wifiInfo->wifiStatus, wifiInfo->ssid,
                          wifiInfo->signalStrength, wifiInfo->ipAddress,
                          wifiInfo->connectedDevices, wifiInfo->macAddress);

    if (offset < 0 || offset >= jsonBufferSize)
    {
        printf("Error: JSON buffer overflow.\n");
        return -1;
    }

    jsonBuffer[offset] = '\0'; // Ensure null termination

    // Configure the MQTT message
    msg.size_content = offset + 1; // Includes null terminator
    msg.content = (char *)malloc(msg.size_content);
    if (!msg.content)
    {
        printf("Error: Insufficient memory for msg.content.\n");
        return -1;
    }
    memcpy(msg.content, jsonBuffer, msg.size_content);

    msg.size_topic = 50;
    msg.topic = malloc(msg.size_topic);
    if (!msg.topic)
    {
        printf("Error: Insufficient memory for msg.topic.\n");
        free(msg.content);
        return -1;
    }
    snprintf(msg.topic, msg.size_topic, "smaway/%s/toapp/config/307\0", mac);

    // Send the message via MQTT
    sendToMqttCheckingMailbox(&msg);
    //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);

    return 0;
}


int initialize_access_point()
{
    int32_t status;

    const char* password = "password123";
    int channel = 6;
    int securityType = SL_WLAN_SEC_TYPE_WPA_WPA2;
    // Validate input parameters
    if (SSID_NAME_AP == NULL || strlen(SSID_NAME_AP) > 32)
    {
        printf("Error: Invalid SSID.\n");
        return -1;
    }
    if (password == NULL || (securityType != SL_WLAN_SEC_TYPE_OPEN && strlen(password) < 8))
    {
        printf("Error: Invalid password.\n");
        return -1;
    }
    if (channel < 1 || channel > 11)
    {
        printf("Error: Invalid channel. Use a value between 1 and 11.\n");
        return -1;
    }

    Task_sleep(500);

    // Configure the SSID
    status = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SSID, strlen(SSID_NAME_AP), (unsigned char*)SSID_NAME_AP);
    if (status < 0)
    {
        printf("Error configuring the SSID: %ld\n", status);
        return status;
    }

    // Configure the channel
    status = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_CHANNEL, sizeof(channel), (unsigned char*)&channel);
    if (status < 0)
    {
        printf("Error configuring the channel: %ld\n", status);
        return status;
    }

    // Configure the security type
    status = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_SECURITY_TYPE, sizeof(securityType), (unsigned char*)&securityType);
    if (status < 0)
    {
        printf("Error configuring the security type: %ld\n", status);
        return status;
    }

    // Configure the password (only if the security type is not open)
    if (securityType != SL_WLAN_SEC_TYPE_OPEN)
    {
        status = sl_WlanSet(SL_WLAN_CFG_AP_ID, SL_WLAN_AP_OPT_PASSWORD, strlen(password), (unsigned char*)password);
        if (status < 0)
        {
            printf("Error configuring the password: %ld\n", status);
            return status;
        }
    }

    // Set the device to Access Point mode
    status = sl_WlanSetMode(ROLE_AP);
    if (status < 0)
    {
        printf("Error setting AP mode: %ld\n", status);
        return status;
    }

    Task_sleep(500);

    // Restart the device to apply the changes
    status = sl_Stop(SL_STOP_TIMEOUT);
    if (status < 0)
    {
        printf("Error stopping the device: %ld\n", status);
        return status;
    }

    status = sl_Start(0, 0, 0);
    if (status < 0)
    {
        printf("Error starting the device: %ld\n", status);
        return status;
    }

    printf("Access Point initialized successfully.\n");
    return 0;
}


int initialize_station(const char *ssid, const char *password, int securityType)
{
    char lm[250];
    int32_t status;
    SlWlanSecParams_t secParams;

    // Validate input parameters
    if (ssid == NULL || strlen(ssid) > 32)
    {
        printf("Error: Invalid SSID.\n");
        return -1;
    }
    if (securityType != SL_WLAN_SEC_TYPE_OPEN)
    {
        if (password == NULL || strlen(password) < 8)
        {
            printf("Error: Invalid password.\n");
            return -1;
        }
    }

    if (get_role_wifi() != ROLE_STA)
    {

        write_wifi_role(ROLE_STA);
        Task_sleep(1000);

        // Configure the device to station mode
        status = sl_WlanSetMode(ROLE_STA);
        if (status < 0)
        {
            printf("Error configuring station mode: %ld\n", status);
            return status;
        }
    }

    Task_sleep(1000);

    // Restart the device to apply mode changes
    status = sl_Stop(SL_STOP_TIMEOUT);
    if (status < 0)
    {
        printf("Error stopping the device: %ld\n", status);
        return status;
    }

    status = sl_Start(0, 0, 0);
    if (status != ROLE_STA)
    {
        printf("Error: Could not start in station mode. Code: %ld\n", status);
        return status;
    }

    printf("Station mode initialized successfully.\n");

    // Configure security parameters
    memset(&secParams, 0, sizeof(SlWlanSecParams_t));
    if (securityType == SL_WLAN_SEC_TYPE_OPEN)
    {
        secParams.Key = NULL;
        secParams.KeyLen = 0;
    }
    else
    {
        secParams.Key = (signed char *)password;
        secParams.KeyLen = strlen(password);
    }
    secParams.Type = securityType;

    // Attempt to connect to the Wi-Fi network
    printf("Attempting to connect to Wi-Fi network: %s\n", ssid);
    Task_sleep(1000);

    status = sl_WlanConnect((const signed char *)ssid, strlen(ssid), 0, &secParams, 0);
    if (status < 0)
    {
        printf("Error attempting to connect to Wi-Fi network: %ld\n", status);
        return status;
    }

    printf("Wi-Fi connection initiated, waiting for confirmation...\n");

    int i;
    // Wait for connection confirmation
    for (i = 0; i < 10; i++) // Wait up to 10 seconds
    {
        Task_sleep(1000);

        SlNetCfgIpV4Args_t ipV4Config;
        unsigned char len = sizeof(ipV4Config);
        unsigned char wlanStatus;

        Task_sleep(500);

        status = sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &wlanStatus, &len, (unsigned char *)&ipV4Config);
        if (status == 0) {
            printf("Connected.\n");
            wifi_connected = true;
            Semaphore_post(sem_online);
            sprintf(lm, "Wifi Configuration\n"
                    "  IPv4 Address  . . . %lu.%lu.%lu.%lu\n"
                    "  Default Gateway . . %lu.%lu.%lu.%lu\n"
                    "  DNS server . . . .  %lu.%lu.%lu.%lu\n",
                    SL_IPV4_BYTE(ipV4Config.Ip, 3), SL_IPV4_BYTE(ipV4Config.Ip, 2), SL_IPV4_BYTE(ipV4Config.Ip, 1), SL_IPV4_BYTE(ipV4Config.Ip, 0),
                    SL_IPV4_BYTE(ipV4Config.IpGateway, 3), SL_IPV4_BYTE(ipV4Config.IpGateway, 2), SL_IPV4_BYTE(ipV4Config.IpGateway, 1), SL_IPV4_BYTE(ipV4Config.IpGateway, 0),
                    SL_IPV4_BYTE(ipV4Config.IpDnsServer, 3), SL_IPV4_BYTE(ipV4Config.IpDnsServer, 2),
                    SL_IPV4_BYTE(ipV4Config.IpDnsServer, 1), SL_IPV4_BYTE(ipV4Config.IpDnsServer, 0));
            #ifdef _DEBUG_
            printf("%s",lm);
            #endif
            logevent(L_INFO, lm);
            return 0;
        }

        printf("Waiting for connection (%d/10)...\n", i + 1);
    }
    wifi_connected = false;
    printf("Error: Could not connect within the expected time.\n");
    return -1;
}

int set_mac_address(const char* macAddress)
{
    unsigned char macBytes[6];
    int32_t status;

    // Validate input format (AA:BB:CC:DD:EE:FF)
    if (strlen(macAddress) != 17)
    {
        printf("Error: Invalid MAC address format. Expected format: AA:BB:CC:DD:EE:FF\n");
        return -1;
    }

    // Convert MAC address string to byte array
    if (sscanf(macAddress, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &macBytes[0], &macBytes[1], &macBytes[2],
               &macBytes[3], &macBytes[4], &macBytes[5]) != 6)
    {
        printf("Error: Failed to parse MAC address.\n");
        return -1;
    }

    // Set the MAC address
    //status = sl_NetCfgSet(SL_NETCFG_MAC_ADDRESS_SET, 1, sizeof(macBytes), macBytes);
    _u8 MAC_Address[6];
    MAC_Address[0] = 0x8;
    MAC_Address[1] = 0x0;
    MAC_Address[2] = 0x30;
    MAC_Address[3] = 0x22;
    MAC_Address[4] = 0x69;
    MAC_Address[5] = 0x32;

    Task_sleep(500);

    status = sl_NetCfgSet(SL_NETCFG_MAC_ADDRESS_SET,1,SL_MAC_ADDR_LEN,(_u8 *)MAC_Address);
    if (status < 0)
    {
        printf("Error: Failed to set MAC address. Code: %ld\n", status);
        return status;
    }

    printf("MAC address set successfully to: %s\n", macAddress);

    Task_sleep(500);

    // Restart the device to apply the new MAC address
    status = sl_Stop(SL_STOP_TIMEOUT);
    if (status < 0)
    {
        printf("Error: Failed to stop the device. Code: %ld\n", status);
        return status;
    }

    status = sl_Start(0, 0, 0);
    if (status < 0)
    {
        printf("Error: Failed to start the device. Code: %ld\n", status);
        return status;
    }

    printf("Device restarted successfully to apply the new MAC address.\n");
    return 0;
}


void update_CC3135_time(const struct tm *time_info)
{
    SlDateTime_t CC3135_time;

    // Convert struct tm to SlDateTime_t
    CC3135_time.tm_day = time_info->tm_mday;
    CC3135_time.tm_mon = time_info->tm_mon + 1; // tm_mon is 0-based, SlDateTime_t is 1-based
    CC3135_time.tm_year = time_info->tm_year + 1900; // tm_year is from 1900
    CC3135_time.tm_hour = time_info->tm_hour;
    CC3135_time.tm_min = time_info->tm_min;
    CC3135_time.tm_sec = time_info->tm_sec;

    Task_sleep(500);

    // Call SimpleLink function to set the time
    int32_t status = sl_DeviceSet(SL_DEVICE_GENERAL,
                                  SL_DEVICE_GENERAL_DATE_TIME,
                                  sizeof(SlDateTime_t),
                                  (uint8_t *)&CC3135_time);

    // Handle errors
    if (status < 0)
    {
        printf("Error updating time in CC3135: %ld\n", status);
    }
    else
    {
        printf("Time successfully updated in CC3135.\n");
    }
}

void print_CC3135_time()
{
    SlDateTime_t CC3135_time;
    uint16_t size = sizeof(SlDateTime_t);
    uint16_t config_opt = SL_DEVICE_GENERAL_DATE_TIME;

    Task_sleep(500);

    // Get the current time from the CC3135
    int32_t status = sl_DeviceGet(SL_DEVICE_GENERAL, &config_opt, &size, (uint8_t *)&CC3135_time);

    // Check if an error occurred
    if (status < 0)
    {
        printf("Error getting the CC3135 time: %ld\n", status);
        return;
    }

    // Print the obtained time
    printf("Current time from CC3135:\n");
    printf("Year: %d\n", CC3135_time.tm_year);
    printf("Month: %d\n", CC3135_time.tm_mon);
    printf("Day: %d\n", CC3135_time.tm_day);
    printf("Hour: %d\n", CC3135_time.tm_hour);
    printf("Minute: %d\n", CC3135_time.tm_min);
    printf("Second: %d\n", CC3135_time.tm_sec);
}

void read_wifi_role()
{
    Semaphore_pend(sem_role_wifi, BIOS_WAIT_FOREVER);
    flash_read_val(WIFI_ROLE, &role_wifi, 1);
    Semaphore_post(sem_role_wifi);
}

void write_wifi_role(uint8_t role)
{
    Semaphore_pend(sem_role_wifi, BIOS_WAIT_FOREVER);
    role_wifi = role;
    flash_write_val(WIFI_ROLE, &role_wifi, 1);
    printf("Role: %d \n", role_wifi);
    Semaphore_post(sem_role_wifi);
}

uint8_t get_role_wifi()
{
    Semaphore_pend(sem_role_wifi, BIOS_WAIT_FOREVER);
    uint8_t role = role_wifi;
    //printf("Role: %d \n", role_wifi);
    Semaphore_post(sem_role_wifi);
    return role;
}

// Function to generate a secure token based on the device UID
_u32 generate_secure_token()
{
    _u32 uid_part1 = SYSCTL->DID0;  // Lower part of the UID (DID0 register)
    _u32 uid_part2 = SYSCTL->DID1;  // Upper part of the UID (DID1 register)

    // Use XOR to combine the two parts of the UID
    return uid_part1 ^ uid_part2;  // Return the combined token
}

// Function to store WiFi SSID, password, and security type in secure storage
int store_wifi_credentials(const char* ssid, const char* password, int securityType)
{
    int32_t fileHandle;

    // Ensure the SSID and password lengths are within the allowed limits
    if (strlen(ssid) > MAX_SSID_SIZE || strlen(password) > MAX_PASSWORD_SIZE)
    {
        return -1;  // Error: SSID or password is too long
    }

    // Create a WiFiCredentials structure to hold SSID, password, and security type
    WiFiCredentials credentials;
    strncpy(credentials.ssid, ssid, MAX_SSID_SIZE);
    strncpy(credentials.password, password, MAX_PASSWORD_SIZE);
    credentials.securityType = securityType;


    printf("Direccion de token: %p, Valor: %lu\n", (void*)&token, token);
    // Open the file in secure mode to create or overwrite the file
    fileHandle = sl_FsOpen((unsigned char *)WIFI_CREDENTIALS_FILE_NAME,
                           SL_FS_CREATE | SL_FS_OVERWRITE | SL_FS_CREATE_SECURE | SL_FS_CREATE_NOSIGNATURE | SL_FS_CREATE_MAX_SIZE(sizeof(WiFiCredentials) +1),
                           &token);

    if (fileHandle < 0)
    {
        // Error opening the file
        return -1;  // Return error code
    }

    flash_write_val(TOKEN, &token, 4);

    Task_sleep(3000);
    // Write the credentials structure to the file
    _i32 ret = sl_FsWrite(fileHandle, 0, (unsigned char *)&credentials, sizeof(WiFiCredentials));

    ret = sl_FsClose(fileHandle, NULL, NULL, 0);  // Close the file after writing

    return 0;  // Success
}

// Function to retrieve WiFi SSID, password, and security type from secure storage
WiFiCredentials get_wifi_credentials()
{
    static WiFiCredentials retrieved_credentials;  // Static buffer to store the retrieved credentials
    int32_t fileHandle;

    flash_read_val(TOKEN, &token, 4);
    printf("Direccion de token: %p, Valor: %lu\n", (void*)&token, token);

    Task_sleep(3000);
    // Reopen the file for reading
    fileHandle = sl_FsOpen((unsigned char *)WIFI_CREDENTIALS_FILE_NAME, SL_FS_READ, &token);

    if (fileHandle <= 0)
    {
        printf("fileHandle %d \n", fileHandle);
        if (fileHandle == -2005)
        {
            reset_CC3135();
        }
        else if(fileHandle == -10288)
        {
            sl_FsDel((unsigned char *)WIFI_CREDENTIALS_FILE_NAME, &token);
        }
        // Error reopening the file
        memset(&retrieved_credentials, 0, sizeof(WiFiCredentials));  // Clear the struct in case of error
        return retrieved_credentials;  // Return empty credentials
    }

    // Read the credentials structure from the file
    _i32 a = sl_FsRead(fileHandle, 0, (unsigned char *)&retrieved_credentials, sizeof(WiFiCredentials));

    a = sl_FsClose(fileHandle, NULL, NULL, 0);  // Close the file after reading

    // Return the retrieved credentials
    return retrieved_credentials;
}

void reset_CC3135()
{
    _i16 ret;
    Network_IF_ResetMCUStateMachine();

    Network_IF_DeInitDriver();

    ret = sl_Start(0, 0, 0);
    if (ret < 0)
    {
        printf("Error starting the device: %ld\n", ret);
    }
}
