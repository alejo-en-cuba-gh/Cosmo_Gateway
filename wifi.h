/*
 * wifi.h
 *
 *  Created on: 20 Dec 2024
 *      Author: Lzaro
 */

#ifndef WIFI_H_
#define WIFI_H_

#include "common.h"
#include "network_if.h"
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/porting/SIMPLELINKWIFI.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>
#include <ti/drivers/net/wifi/wlan.h>
#include <arpa/inet.h>
#include <ti/devices/msp432e4/inc/msp432e401y.h>

// Configuración de hardware WiFi
const SIMPLELINKWIFI_HWAttrsV1 wifiSimplelinkHWAttrs =
{
    .spiIndex = CONFIG_SPI_WIFI,
    .hostIRQPin = MSP_EXP432E401Y_HOST_IRQ,
    .nHIBPin = MSP_EXP432E401Y_nHIBPin,
    .csPin = MSP_EXP432E401Y_CS_pin,
    .maxDMASize = 1024,
    .spiBitRate = 12000000
};

// Configuración de WiFi
const uint_least8_t WiFi_count = 1;

const WiFi_Config WiFi_config[1] =
{
    {
        .hwAttrs = &wifiSimplelinkHWAttrs,
    }
};

/*
*  ======== SimpleLinkWifi_config ========
*/
const SlWifiCC32XXConfig_t SimpleLinkWifiCC32XX_config = {
   .Mode = ROLE_STA,
   .Ipv4Mode = SL_NETCFG_IPV4_STA_ADDR_MODE,
   .ConnectionPolicy = SL_WLAN_CONNECTION_POLICY(1,0,0,0),
   .PMPolicy = SL_WLAN_NORMAL_POLICY,
   .MaxSleepTimeMS = 0,
   .ScanPolicy = SL_WLAN_SCAN_POLICY(0,0),
   .ScanIntervalInSeconds = 0,
   .Ipv4Config = SL_NETCFG_ADDR_DHCP,
   .Ipv4 = 0,
   .IpMask = 0,
   .IpGateway = 0,
   .IpDnsServer = 0,
   .ProvisioningStop = 1,
   .DeleteAllProfile = 0
};

#define msg_wifi_t msg_rpc_t
#define MAX_RETRIES 5
#define RETRY_DELAY_MS 1000
#define SLNET_IF_WIFI_PRIO          (5)

// Define the file name for storing the WiFi credentials
#define WIFI_CREDENTIALS_FILE_NAME "Credentials_Wifi"
#define MAX_SSID_SIZE 32   // Maximum size for SSID (WiFi SSID max length is 32)
#define MAX_PASSWORD_SIZE 64  // Maximum size for a WiFi password (WPA2/WPA3 allows up to 64 characters in hex)

// WiFi credentials structure to store SSID, password, and security type as a number
typedef struct {
    char ssid[MAX_SSID_SIZE];
    char password[MAX_PASSWORD_SIZE];
    int securityType;  // Security type as an integer
} WiFiCredentials;

WiFiCredentials credentials;
unsigned char role_wifi = 4;
char SSID_NAME_AP[32] = "MyAP";


volatile bool wifi_connected = false;

_u32 token;

void scan_wifi_networks(char mac[18]);
int get_wifi_acquisition_method( WiFiNetworkInfo *config, char mac[18]);
int get_active_profile(ActiveProfile *profile);
int set_wifi_acquisition_method(const char* method, const char* ipAddress, const char* subnetMask, const char* gateway,
                                const char* dnsServer);
int get_wifi_info(WifiSTATUS* wifiInfo, char mac[18]);
int initialize_access_point();
int initialize_station(const char* ssid, const char* password, int securityType);
int set_mac_address(const char* macAddress);
int get_security_type_from_string(const char *securityType);
int initialize_wifi_ST_AP(char mac[18]);
void update_wifi_status();
void read_wifi_role();
void write_wifi_role(uint8_t role);
uint8_t get_role_wifi();
_u32 generate_secure_token();
int store_wifi_credentials(const char* ssid, const char* password, int securityType);
WiFiCredentials get_wifi_credentials();

#endif /* WIFI_H_ */
