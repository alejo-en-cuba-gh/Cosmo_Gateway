/*
 * mqttc.h
 *
 *  Created on: 6 mar 2024
 */

#ifndef MQTTC_H_
#define MQTTC_H_

#define MQTTClientCbs_ConnackRC(data) (data & 0xff) // < CONNACK: Return Code (LSB)

#include <ti/net/mqtt/mqttclient.h>

struct publishMsgHeader
{
    uint32_t        topicLen;
    uint32_t        payLen;
    bool            retain;
    bool            dup;
    unsigned char   qos;
};

static void mqtt_cb(int32_t event , void * metaData , uint32_t metaDateLen , void *data , uint32_t dataLen);
static void set_mqtt_connected (bool c);
static void mqtt_disconnect();
void mqtt_run(UArg a0, UArg a1);

#endif /* MQTTC_H_ */
