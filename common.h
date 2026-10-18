/*
 * common.h
 *
 *  Created on: 27 set 2023
 */

#ifndef COMMON_H_
#define COMMON_H_

/* Common C headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

/* RTOS header files */
#include <ti/drivers/Board.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Semaphore.h>
//#include <ti/sysbios/knl/Queue.h>
//#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/hal/Hwi.h>
//#include <ti/sysbios/knl/Swi.h>
//#include <ti/sysbios/knl/Swi.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Error.h>

/* TI-DRIVERS Header files */
#include "ti_drivers_config.h"
#include <ti/drivers/apps/LED.h>
#include <ti/drivers/apps/Button.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/CAN.h>
#include <ti/drivers/SDFatFS.h>

/* Platform includes */
#include <ti/display/Display.h>

#include <ti/utils/json/json.h>
#include <third_party/spiffs/spiffs.h>
#include <third_party/spiffs/SPIFFSNVS.h>


//#define _USELOGS_
#ifdef _USELOGS_
    #define _LOGUDP_
#endif
#define _LOGTASK_
/* Stack size in bytes */
#define TASK_HUNGRY 4096
#define TASK_NORMAL 2048
#define TASK_LIGHT 1024

#define TASK_PRIO_LOW 2
#define TASK_PRIO_MEDIUM 4
#define TASK_PRIO_HIGH 6
#define TASK_PRIO_EXTREME 9


#ifdef _LOGTASK_
#define TASK_HISTORY_SIZE 5  // Keep last 5 tasks
#endif

#define MBOXSIZE 50
#define UNIT_ID 1 //always 0 for main unit

#define N_DBTABLES 14

// define macros for setting, getting, clearing, and toggling bits
#define SET_BIT(n, k) (n |= (1UL << k)) // set the k-th bit of n to 1
#define GET_BIT(n, k) ((n >> k) & 1UL) // get the value of the k-th bit of n
#define CLEAR_BIT(n, k) (n &= ~(1UL << k)) // clear the k-th bit of n to 0
#define TOGGLE_BIT(n, k) (n ^= (1UL << k)) // toggle the k-th bit of n
#define min(a,b) (((a)<(b))?(a):(b))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)
#define BREAK_UINT32(var, ByteNum) (uint8_t)((uint32_t)(((var)>>((ByteNum) * 8)) & 0x00FF))
#define BREAK_UINT64(var, ByteNum) (uint8_t)((uint64_t)(((var)>>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
                   + ((uint32_t)((Byte1) & 0x00FF) << 8) \
                   + ((uint32_t)((Byte2) & 0x00FF) << 16) \
                   + ((uint32_t)((Byte3) & 0x00FF) << 24)))

#define BUILD_UINT64(Byte0, Byte1, Byte2, Byte3, Byte4, Byte5, Byte6, Byte7) \
                     ((uint64_t)((uint64_t)((Byte0) & 0x00FF) \
                              + ((uint64_t)((Byte1) & 0x00FF) << 8) \
                              + ((uint64_t)((Byte2) & 0x00FF) << 16)\
                              + ((uint64_t)((Byte3) & 0x00FF) << 24)\
                              + ((uint64_t)((Byte4) & 0x00FF) << 32)\
                              + ((uint64_t)((Byte5) & 0x00FF) << 40)\
                              + ((uint64_t)((Byte6) & 0x00FF) << 48)\
                              + ((uint64_t)((Byte7) & 0x00FF) << 56)))

#define LOG_PATH "log_path"
#define LOG_SIZE "log_SIZE"
#define TEMPLATE_PATH "template_path"
#define MAX_NUMBER_BAYTE_ITEM (1024)

#define MQTT_MAILBOX_WAIT_EMPTY_LEN 5
#define MQTT_MAILBOX_DELAYMS_EMPTY  50

/* Tasks */
void db_init(UArg arg0, UArg arg1);
void net_init(UArg arg0, UArg arg1);
void net_update(UArg arg0, UArg arg1);
void event_in(UArg arg0, UArg arg1);
void event_out(UArg arg0, UArg arg1);
void timer_tick(UArg arg0, UArg arg1);
void timer30(UArg arg);
void logger(UArg arg0, UArg arg1);
#ifdef _LOGUDP_
void logger_udp(UArg arg0, UArg arg1);
#endif
void can_receiver(UArg arg0, UArg arg1);
void can_sender(UArg arg0, UArg arg1);
void net_upstate(UArg arg0, UArg arg1);
void ledstate(UArg arg0, UArg arg1);
void gpio_task(UArg arg0, UArg arg1);
void finder_service(UArg a0, UArg a1);
void rpc_reciever(UArg a0, UArg a1);
void rpc_send(UArg a0, UArg a1);
void mqtt_client(UArg a0, UArg a1);
void appsvr_connection_mqtt(UArg a0, UArg a1);
void mqtt_send(UArg a0, UArg a1);
void Wifi_command(UArg a0, UArg a1);
void wifi_connect(UArg a0, UArg a1);

/* Configuration database structures */
// Define a struct to hold the data for each array element
typedef struct dbt_item
{
    struct dbt_item *element;
    struct dbt_item *previous;
    struct dbt_item *next;
    uint16_t id;
    uint8_t unit;
    uint16_t component;
    uint8_t port;
    uint8_t type;
    uint8_t icon;
    uint8_t state;
    char name[25];
} dbt_item;

typedef struct dbt_component
{
    struct dbt_item *element;
    struct dbt_component *previous;
    struct dbt_component *next;
    uint16_t id;
    char name[25];
    uint8_t type;
} dbt_component;

typedef struct dbt_unit
{
    struct dbt_item *element;
    struct dbt_unit *previous;
    struct dbt_unit *next;
    uint16_t id;
    char hwid[13];
    uint8_t type;
    uint8_t vhw;
    uint16_t vfw;
    uint8_t acid;
    int8_t online;
    char name[25];
} dbt_unit;

typedef struct dbt_rule
{
    struct dbt_item *element;
    struct dbt_rule *previous;
    struct dbt_rule *next;
    uint16_t id;
    char name[25];
    bool enabled;
} dbt_rule;

typedef struct dbt_rulepart
{
    struct dbt_item *element;
    struct dbt_rulepart *previous;
    struct dbt_rulepart *next;
    uint16_t id;
    uint16_t rid;
    bool scond;
    uint16_t citem;
    char cop;
    uint16_t cvalue;
} dbt_rulepart;

typedef struct dbt_ruletimer
{
    struct dbt_item *element;
    struct dbt_ruletimer *previous;
    struct dbt_ruletimer *next;
    uint16_t id;
    uint16_t rid;
    uint8_t type;
    uint32_t day;
    uint16_t tstart;
    uint16_t tend;
} dbt_ruletimer;

typedef struct dbt_timer
{
    struct dbt_item *element;
    struct dbt_timer *previous;
    struct dbt_timer *next;
    uint16_t id;
    uint16_t rid;
    uint8_t type;
    uint32_t day;
    uint16_t tstart;
} dbt_timer;

typedef struct dbt_action
{
    struct dbt_item *element;
    struct dbt_action *previous;
    struct dbt_action *next;
    uint16_t id;
    uint16_t rid;
    uint8_t atype;
    uint16_t aitem;
    uint8_t aaction;
} dbt_action;

typedef struct dbt_device
{
    struct dbt_item *element;
    struct dbt_device *previous;
    struct dbt_device *next;
    char mac[13];
    uint32_t idate;
    char name[25];
    char location[25];
    bool lan_dhcp;
    uint32_t lan_ip;
    uint32_t lan_gw;
    uint32_t lan_dns;
    uint32_t lan_sn;
    uint32_t cur_ip;
    uint32_t cur_gw;
    uint32_t cur_dns;
    uint32_t cur_sn;
    char time[20];
    uint8_t dst;
    int8_t timezone;
    uint16_t vhw;
    uint16_t vfw;
    uint16_t vdb;
    int8_t loglev;
    uint16_t logudp;
    uint32_t acip;
    uint16_t acp;
} dbt_device;

typedef struct dbt_user
{
    struct dbt_item *element;
    struct dbt_user *previous;
    struct dbt_user *next;
    uint16_t id;
    char name[25];
    char email[50];
    int32_t pw;
    uint8_t level;
} dbt_user;

typedef struct dbt_group
{
    struct dbt_item *element;
    struct dbt_group *previous;
    struct dbt_group *next;
    uint16_t id;
    char name[25];
} dbt_group;

typedef struct dbt_groupitem
{
    struct dbt_item *element;
    struct dbt_groupitem *previous;
    struct dbt_groupitem *next;
    uint16_t id;
    uint16_t group;
    uint16_t item;
} dbt_groupitem;

typedef struct dbt_zdevice
{
    struct dbt_item *element;
    struct dbt_zdevice *previous;
    struct dbt_zdevice *next;
    uint16_t id;
    uint64_t ieee_address;
    char model_id[10];
    uint16_t nwk_address;
    uint8_t firmware_version;
    bool status;
    uint8_t mfg_id;
    uint8_t ep_count;
    uint64_t parent_address;
    uint8_t cap_info;
} dbt_zdevice;

typedef struct dbt_zendpoint
{
    struct dbt_item *element;
    struct dbt_zendpoint *previous;
    struct dbt_zendpoint *next;
    uint16_t id;
    uint16_t zdevice_id;
    uint16_t zendpoint_id;
    uint16_t profile_id;
    uint16_t device_id;
    uint8_t device_version;
    uint8_t num_in_cluster;
    uint16_t clusters[16];
} dbt_zendpoint;

typedef struct dbt_tversion
{
    uint16_t item;
    uint16_t component;
    uint16_t unit;
    uint16_t rule;
    uint16_t rulepart;
    uint16_t ruletimer;
    uint16_t timer;
    uint16_t action;
    uint16_t device;
    uint16_t user;
    uint16_t group;
    uint16_t groupitem;
} dbt_tversion;

typedef struct dbt_generic
{
    struct dbt_item *element;
    struct dbt_generic *previous;
    struct dbt_generic *next;
    uint16_t id;
} dbt_generic;

typedef struct dbt_db
{
    struct dbt_item *db_item;
    struct dbt_item *db_item_last;
    struct dbt_component *db_component;
    struct dbt_component *db_component_last;
    struct dbt_unit *db_unit;
    struct dbt_unit *db_unit_last;
    struct dbt_rule *db_rule;
    struct dbt_rule *db_rule_last;
    struct dbt_rulepart *db_rulepart;
    struct dbt_rulepart *db_rulepart_last;
    struct dbt_ruletimer *db_ruletimer;
    struct dbt_ruletimer *db_ruletimer_last;
    struct dbt_action *db_action;
    struct dbt_action *db_action_last;
    struct dbt_timer *db_timer;
    struct dbt_timer *db_timer_last;
    struct dbt_device *db_device;
    struct dbt_device *db_device_last;
    struct dbt_user *db_user;
    struct dbt_user *db_user_last;
    struct dbt_group *db_group;
    struct dbt_group *db_group_last;
    struct dbt_groupitem *db_groupitem;
    struct dbt_groupitem *db_groupitem_last;
    struct dbt_zdevice *db_zdevice;
    struct dbt_zdevice *db_zdevice_last;
    struct dbt_zendpoint *db_zendpoint;
    struct dbt_zendpoint *db_zendpoint_last;
} dbt_db;

typedef enum db_type
{
    ITEM = 1,
    COMPONENT = 2,
    UNIT = 3,
    RULE = 4,
    RULEPART = 5,
    RULETIMER = 6,
    ACTION = 7,
    TIMER = 8,
    DEVICE = 9,
    USER = 10,
    GROUP = 11,
    GROUPITEM = 12,
    ZDEVICE = 13,
    ZENDPOINT = 14
} db_type;

typedef enum item_type
{
    BOOL, NUMBER, STRING
} item_type;

typedef enum number_type
{
    N_INT = 0, N_UINT8, N_UINT16, N_UINT32, N_UINT64, N_INT8, N_INT16, N_INT32
} number_type;

typedef enum action_type
{
    A_RULEPART = 1, A_RULETIMER, A_ITEM, A_GROUP, A_MESSAGE
} action_type;

typedef enum timer_type
{
    T_DAILY = 1, T_DAYS, T_MONTHLY
} timer_type;

typedef enum loglevel
{
    L_DEBUG = 1, L_INFO = 2, L_ERROR = 3, L_EVENT = 10
} loglevel;

typedef enum oem_t
{
    M_SMABIT = 0, M_SCANVEST = 1
} oem_t;

typedef enum unit_t
{
    UT_8IO = 1, UT_8IO_S = 2, UT_MU_8IO_S = 3
} unit_t;

typedef enum mu_t
{
    MU_BASE = 0, MU_8IO = 1
} mu_t;

typedef enum item_t
{
    IT_IN = 0, IT_OUT = 1
} item_t;

typedef enum gw_status_t
{
    FACTORY_DEFAULT = 0, CONFIGURED = 1, FIRMWARE_UPGRADE = 2, RESTORE = 3, OFFLINE = 4
} gw_status_t;

typedef enum fl_var_type
{
    FL_FW_2 = 0, FL_INS_DATE_4 = 2, FL_OEM_1 = 6, FL_STATUS_1 = 8, WIFI_ROLE = 10, TOKEN = 12
} fl_var_type;

typedef enum sd_log_event_t
{
    SLE_SYSLOG, SLE_EVLOG, SLE_RESTART, SLE_EXIT
} sd_log_event_t;

typedef enum db_event_t
{
    DBE_BACKUP, DBE_EXPORT
} db_event_t;

// ATTENTION: change LS_ITEMS in buttonled.h when changing enum below!
typedef enum ledstate_item_t
{
    LS_NET = 1, LS_AC = 2, LS_SD = 3, LS_UPDATE = 4, LS_FTP = 5
} ledstate_item_t;

typedef enum CAN_MTYPE
{
    C_EVENT_IN = 0, C_EVENT_OUT = 1, C_REGISTRATION = 2, C_VERSIONS = 3, C_ALIVE = 4, C_RESET = 5, C_PING = 6, C_PONG = 7, C_ACMSG = 8
} CAN_MTYPE;

typedef enum appmsg_t
{
    AM_DBSET = 0,           // Change DB record
    AM_DBGET = 1,           // Get/send DB record
    AM_DBVER = 2,           // Get DB table versions
    AM_RESTART = 3,         // Restart SW
    AM_HELLO = 4,           // Keep-alive, SW responds with AM_HELLO
    AM_DISCONNECT = 5,      // Stop connection
    AM_AUTH = 6,            // Authenticate
    AM_SWITCH_ITEM = 7,     // Change item state
    AM_SWITCH_RULE = 8,     // Run rule manually

    AM_DBDELETE = 9,         // Delete DB record
    AM_TIME = 10,
    AM_SWITCH_GROUP = 11,   // Change group item state
    AM_ZIGBEE = 12          // For Zigbee Commands
} appmsg_t;

typedef enum zbmsg_t
{
    SYS_PING = 0,
    STARTNETWORK = 1,
    SETCHANNEL = 2,
    SETZDODIRECTCB = 3,
    SETPANID = 4,
    SETDEVTYPE = 5,

    NWK_SET_PERMIT_JOIN_REQ = 7,
    NWK_ZIGBEE_NWK_INFO_REQ = 8,
    NWK_GET_DEVICE_LIST_REQ = 9,
    NWK_REMOVE_DEVICE_REQ = 10,
    NWK_LEAVE_DEVICE_REQ = 11,
    NWK_GET_LOCAL_DEVICE_INFO_REQ = 12,
    NWK_ZIGBEE_SYSTEM_RESET_REQ = 13,
    NWK_SET_BINDING_ENTRY_REQ = 14,
    NWK_SET_UNBINDING_ENTRY_REQ = 15,
    NWK_GET_GW_ENDPOINT_INFO_REQ = 16,
    NWK_GET_NWK_KEY_REQ = 17,
    NWK_CHANGE_NWK_KEY_REQ = 18,

    DEV_SET_ONOFF_STATE_REQ = 19,
    DEV_GET_ONOFF_STATE_REQ = 20,
    DEV_SET_LEVEL_REQ = 21,
    DEV_GET_LEVEL_REQ = 22,
    DEV_GET_TEMP_REQ = 23,
    DEV_WINDOW_COVERING_ACTION_REQ = 24,

    GW_GET_DEVICE_ATTRIBUTE_LIST_REQ = 25,
    GW_SET_ATTRIBUTE_REPORTING_REQ = 26,
    GW_WRITE_DEVICE_ATTRIBUTE_REQ = 27,
    GW_READ_DEVICE_ATTRIBUTE_REQ = 28,
    GW_SEND_ZCL_FRAME_REQ = 29,
    DEV_SET_IDENTIFY_MODE_REQ = 30,

    COMISIOINING_NOTIFICATION = 50,
    ZDO_START_UP = 51,
    ZDO_STATE_CHANGE = 52,
    ZDO_PERMIT_JOIN = 53,
    ZDO_TC_DEV_IND = 54,
    ZDO_END_DEVICE_ANNCE = 55,
    SYS_RESET_ARSP = 56,

    //CMD ID - NwkMgr
    GetDeviceListReq = 200,
    GetDeviceListRsp,
    NetworkInfoReq,
    NetworkInfoRsp,
    NetworkKeyReq,
    NetworkKeyRsp,
    ChangeNetworkKeyReq,
    OpenNetwork,
    CloseNetwork,
    ReceiveDevice,
    BindReq,
    BindRsp,
    UnbindReq,
    UnbindRsp,
    RemoveDeviceReq,
    RemoveDeviceRsp,
    RemoveDeviceReport,
    SystemReset,

    //CMD ID - HaGateway
    SetConfigReportReq = 300,
    SetConfigReportRsp,
    ReadAttributeReq,
    ReadAttributeRsp,
    WriteAttributeReq,
    WriteAttributeRsp,
    SendZCLFrameReq,
    SendZCLFrameRsp,
    AttributeListReq,
    AttributeListRsp,
    AttributeReporting,
    IdentifyReq,

    firmware_version = 400,
    model_id,
    power_source,
    onoff,
    onoff_2,
    level_control,
    local_temperature,
    target_temperature,
    outdoor_temperature_measurement,
    system_mode,
    battery_percent,
    battery_low,
    temperature_measurement,
    humidity_measurement,
    lqi_value,
} zbmsg_t;

// Mailbox event types
typedef struct msg_event_t
{
    uint16_t unit;
    uint16_t port;
    uint8_t state;
} msg_event_t;

# define SLL_BUFSIZE 400
# define SLL_MAXMSG 40

#define MAX_ITEMS 20
#define MAX_STR_LEN 130
#define MAX_STR_LEN_2 15

typedef struct msg_sll_t
{
    uint16_t unit;
    uint16_t port;
    uint8_t state;
    char recv[SLL_MAXMSG];
} msg_sll_t;

typedef struct msg_log_t
{
    loglevel level;
    sd_log_event_t se;
    char *msg;
    size_t size;
} msg_log_t;

typedef struct msg_led_t
{
    ledstate_item_t led;
    bool state;
} msg_led_t;

typedef struct msg_dbev_t
{
    appmsg_t ev;
    void *s;
    db_type tab_id;
    uint16_t rec_id;
    char *entry;
    uint16_t esize;
} msg_dbev_t;

typedef struct msg_rpc_t
{
    char *device_id;
    uint16_t id_length;
    char *item_type;
    uint16_t type_length;
    uint8_t read_write;
    char *data;
    uint16_t data_length;
} msg_rpc_t;

/*
 typedef struct msg_rpc_t
 {
 bool is_app;
 uint16_t cmd_id;
 uint16_t item_type;
 char device_ieee[23];
 } msg_rpc_t;
 */
typedef struct msg_mqtt_t
{
    char *topic;
    char *content;
    void *buf;
    uint16_t size_topic;
    uint16_t size_content;
    uint16_t size_buf;
} msg_mqtt_t;

typedef enum {
    STATIC,
    DHCP
} IPMethod;

// Definition of structure to store WiFi network information
typedef struct {
    IPMethod Method;
    char IpAddress[16];
    char SubnetMask[16];
    char Gateway[16];
    char DnsServer[16];
} WiFiNetworkInfo;

typedef struct {
    char mode[12]; // STATION or ACCESSPOINT
    char wifiStatus[13]; // CONNECTED, DISCONNECTED, ACTIVE, INACTIVE
    char ssid[32]; // SSID of the network
    int signalStrength; // Signal strength in dBm
    char ipAddress[16]; // IP address (e.g., "192.168.1.100")
    int connectedDevices; // Number of devices connected in ACCESSPOINT mode
    char macAddress[18]; // MAC address (e.g., AA:BB:CC:DD:EE:FF)
} WifiSTATUS;

// Enumeration for security types
typedef enum {
    OPEN,
    WEP,
    WEP_SHARED,
    WPA_WPA2,
    WPA2_PLUS,
    WPA3,
    WPA_ENT,
    WPS_PBC,
    WPS_PIN,
    WPA_PMK
} SecurityType;

// Structure to store the active profile parameters
typedef struct {
    char SSID[33]; // Maximum size for SSID (32 characters + null terminator)
    char Password[65]; // Maximum size for Password (64 characters + null terminator)
    SecurityType SecType;
} ActiveProfile;

extern volatile bool wifi_connected;

extern Task_Handle t_main, t_led1, t_led2, t_json_db, t_net, t_net_update, t_process_rule, t_logger, t_sll, t_can_send, t_can_recv, t_logger_udp, t_net_upstate,
t_sd, t_8io, t_tick, t_led, t_appsvr_svc, t_appsvr_send, t_finder, t_flash, t_Wifi_command, t_mqtt_send, t_mqtt_client, t_mqtt_run, t_appsvr_connection_mqtt, t_wifi_connect;
extern Mailbox_Handle m_main, m_event_in, m_event_out, m_evlog, m_log_udp, m_sll_in, m_can_out, m_gpio, m_led, m_db, m_lappsend, m_rpc_reciever, m_rpc_send, m_wifi_send,
m_rpc_srsp, m_rpc_arsp, m_add_device, m_mtSys, m_mqtt_send, m_frame_proc_mqtt, m_mqtt_recv;
extern Display_Handle display;
extern Semaphore_Handle sem_db_ready, sem_db_updated, sem_ip_update, sem_ltime, sem_nw_ready, sem_nw_connect, sem_logready, sem_sd, sem_sd_gpio, sem_gpio,
sem_ttick, sem_sll_ready, sem_reboot, sem_rpc, sem_srsp, sem_mtprocess, sem_mtaf_response, sem_uart, sem_sntp, sem_mq_disc, sem_mq_connected, sem_mqtt_init, sem_appcon,
sem_online, sem_spiffs_mount, sem_wifi_ready, sem_role_wifi, sem_sta_ap_ready;
extern bool db_ready;
extern dbt_db config_db;
extern const mu_t mu;
extern const oem_t oem;
extern CAN_Handle can;
extern LED_Handle led_status;
extern spiffs fs_spiffs;

bool db_get_db_ready();
dbt_generic* db_add_item(db_type type, dbt_generic **db_item, dbt_generic **last_item);
bool db_delete_item(dbt_generic **item, dbt_generic **first_item, dbt_generic **last_item, db_type type);
dbt_generic* find_element(dbt_generic **item, char **search_item, bool next, db_type type, item_type s_type, bool s_bool, uint64_t s_number, char *s_string, number_type num_type);
dbt_generic* find_element_and(dbt_generic **base, bool next, db_type type, char **search_item1, item_type s_type1, bool s_bool1, int s_number1, char *s_string1, char **search_item2, item_type s_type2, bool s_bool2, int s_number2, char *s_string2);
dbt_generic* get_element(dbt_generic **e, db_type type);
bool set_element(dbt_generic **e, db_type type, bool freemem);
int8_t get_loglevel();
bool get_dhcp();
#ifdef _LOGUDP_
uint16_t get_logudp();
#endif
int16_t get_item_state(uint16_t unit, uint8_t port);
uint8_t create_items(unit_t type, uint8_t uid);
uint8_t restore_items(unit_t type, uint8_t uid, bool all_off);
bool alphacom_set();
uint16_t get_tversion(db_type t);
void inc_tversion(db_type t);
void parse_dbset(msg_dbev_t *msg);
void parse_dbget(msg_dbev_t *msg);
bool parse_dbdelete(msg_dbev_t *msg);
void get_tversion_all(char *s);
uint16_t get_next_dbset(char *s, int64_t *i, char *d);
void set_app_connected(bool y);
void db_get_mac(char *mac);
msg_rpc_t set_msg_sigbee(char *data, uint16_t data_length, char *device_id, uint16_t id_length, char *item_type, uint16_t type_length, uint8_t read_write);

uint16_t to_timestamp(uint8_t hour, uint8_t minute);
void from_timestamp(uint16_t time, uint8_t *hour, uint8_t *minute);
void set_day(uint32_t *month, uint8_t day, bool value);
bool get_day(uint32_t month, int day);
bool* get_days(uint32_t month);
void get_ltime(struct tm *mt);
bool nw_connected();

void run_rule(uint16_t rid);

bool check_pw(char *s, int h);
int hash_pw(char *s);

void logevent(loglevel ll, char *t);
int8_t send_logs();

int32_t get_host_ip(char *host, uint32_t *ip, uint8_t slnetif);
uint8_t update_sntp_time();
int8_t firmware_update(uint8_t version);
void set_ndk_prio();
void lan_connect(UArg arg);

uint8_t set_8io(uint8_t id, bool state);
void get_io_status();

char* substr(char *str, uint16_t pos, uint16_t len);
uint32_t ss_val(char *str, uint16_t pos, uint16_t len, uint8_t base);
bool get_sll_ready();
void set_sll_ready(bool s);

void ledbutton();

bool sd_ready(bool set_sem);
bool log2sd(bool event, char *msg);
void sd_pinevent(uint_least8_t index);
bool untar(char *filename);
void delete_old_files_by_size(char *folder, uint16_t mb_high, uint16_t mb_low);

bool flash_init();

int_fast16_t flash_read_val(uint32_t offset, void *data, uint8_t size);
int_fast16_t flash_write_val(uint32_t offset, void *data, uint8_t size);

int8_t ftpserver_init(void);
bool get_ftp_connect();

uint8_t get_ac_device(uint16_t uid);

void get_free_ram();
Ptr mem_alloc(size_t size);
void mem_free(Ptr p, size_t size);

bool get_mqtt_connected();
void mqtt_receive(msg_mqtt_t *msg);
void sendToMqttCheckingMailbox(msg_mqtt_t* msg);
bool sendToMqttCheckingMailboxNoWait(msg_mqtt_t* msg);

void err_mailbox_full(char *fct, Mailbox_Handle mb);

bool safeTaskDelete(Task_Handle *taskHandle);
void command_101(uint8_t status);
void send_zigbee(char *msg, char *topic, uint16_t size_topic, uint16_t size_msg);
uint16_t isWiFiCommand(const char* input);
int wifi_init();
void reset_CC3135();

void init_spiffs_external();
bool externalflash_ready();
bool log2_spiffs(bool event, char *msg);
bool save_item_spiffs(char *item, char *type);
void delete_old_files_by_size_spiffs(char *type, float maxMB, float minMB);
uint16_t  match_counter(char *string, const char *ch);
void get_index_match(const char *string, const char *ch, const uint16_t counter_match, uint16_t *indexes);
void parse_string(const char *string, const uint16_t *indexes, uint16_t counter, char items[MAX_ITEMS][MAX_STR_LEN_2]);
void get_elements_file(char *type, uint16_t *counter, char items[MAX_ITEMS][MAX_STR_LEN_2]);
uint16_t digit_counter(uint32_t number);
char* format_string_array(char** strings, uint16_t num_elements, const char* end_string);
int32_t get_file_size(const char* filePath);
void manage_files(char paths[MAX_ITEMS][MAX_STR_LEN_2], uint16_t *pathCount, float maxMB, float minMB);
char* concatenate_elements(char paths[MAX_ITEMS][MAX_STR_LEN_2], uint16_t lowerIndex, uint16_t upperIndex);
int32_t get_fileSize(const char* filePath);
int file_exists(const char* file_path);
int16_t send_files();
void get_element_string(const char *string, const uint16_t *indexes, uint16_t counter, char items[MAX_STR_LEN], const uint16_t index);
void get_element_file(char *type, uint16_t *counter, char item[MAX_STR_LEN], const uint16_t index);
void format_and_resize_string(spiffs_file fd, char **str, size_t *size_needed, const char *format, ...);
void erase_flash();
void send_file_operation_response(const char *filename, bool is_format);

void create_com_task();
void delete_com_task();

uint64_t ieee_to_uint64(char *mac);

#endif /* COMMON_H_ */
