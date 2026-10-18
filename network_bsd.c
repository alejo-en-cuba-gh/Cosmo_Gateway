/*
 * network_bsd.c
 *
 *  Created on: 9 nov 2023
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* BSD support */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include <ti/net/slnetif.h>
#include <ti/net/slnetutils.h>
#include <ti/net/slnetsock.h>
#include <ti/net/http/httpclient.h>
#include <ti/net/slnet.h>
#include <ti/ndk/slnetif/slnetifndk.h>

#include "common.h"
#include "network.h"

#define HOSTNAME      "cosmo.smabit.eu"

#define DATA_SIZE 1024

extern void fdOpenSession();
extern void fdCloseSession();
extern void* TaskSelf();

int32_t get_host_ip(char *host, uint32_t *ip, uint8_t slnetif)
{
    int32_t status;
    struct addrinfo *res = NULL;
    char lm[70], sip[INET_ADDRSTRLEN];

    /* Obtain addresses suitable for binding to */
    fdOpenSession(TaskSelf());
    status = SlNetUtil_getAddrInfo(slnetif, host, "0", NULL, &res);
    fdCloseSession(TaskSelf());

    if (status == 0)
    {
        // DNS resolved
        *ip = ((struct sockaddr_in*) (res->ai_addr))->sin_addr.s_addr; // get the uint32_t value of the IP address
        freeaddrinfo(res);
        // buffer for storing the string

        inet_ntop(AF_INET, ip, sip, INET_ADDRSTRLEN); // convert the uint32_t value to a string
    }
    else
    {
        // Null-terminate sip to avoid log error
        sip[0] = 0;
    }
    sprintf(lm, "get_host_ip: %s -> %s, status %d", host, sip, status);
    logevent(L_DEBUG, lm);

    return status;
}

uint8_t update_sntp_time()
{
    int16_t r;
    SlNetSock_Timeval_t timeval;
    uint64_t ntpTimeStamp = 0;
    uint32_t ip;
    uint32_t ip_wifi;
    SlNetSock_AddrIn_t ipv4addr;
    dbt_device *dbd;
    struct tm now;
    struct timespec ts;
    char lm[50], data[100];
    bool nt = false;

    dbd = get_element(&config_db.db_device, DEVICE);

    if (dbd->time == NULL || dbd->time[0] == '0')
    {
        // Internet time services disabled
        return 0;
    }
    else if (dbd->time[0] == '1')
    {
        // Use time.smabit.eu over HTTP
        if (http_get("http://time.smabit.eu", "/", &data, sizeof(data)) == 1)
        {
            ntpTimeStamp = strtol(data, NULL, 10);
            if (ntpTimeStamp > 1672527600)
            {
                // Date value after 01/01/2023
                ts.tv_sec = ntpTimeStamp;
                ts.tv_nsec = 0;
                nt = true;
            }
        }
    }
    else
    {
        // User-defined SNTP
        r = get_host_ip(dbd->time, &ip, SLNETIF_ID_1);
        r &= get_host_ip(dbd->time, &ip_wifi, SLNETIF_ID_2);
        if (r)
        {
            sprintf(lm, "update_sntp_time: get_host_ip error %d", r);
            logevent(L_ERROR, lm);
            return 0;
        }

        ipv4addr.sin_family = SLNETSOCK_AF_INET;
        ipv4addr.sin_port = SlNetUtil_htons(123);
        ipv4addr.sin_addr.s_addr = ip;

        timeval.tv_sec = 5;
        timeval.tv_usec = 0;
        fdOpenSession(TaskSelf());
        r = SNTP_getTimeByAddr((SlNetSock_Addr_t*) &ipv4addr, &timeval, &ntpTimeStamp);
        if(r < 0)
        {
            ipv4addr.sin_family = SLNETSOCK_AF_INET;
            ipv4addr.sin_port = SlNetUtil_htons(123);
            ipv4addr.sin_addr.s_addr = ip_wifi;

            timeval.tv_sec = 5;
            timeval.tv_usec = 0;
            fdOpenSession(TaskSelf());
            r = SNTP_getTimeByAddr((SlNetSock_Addr_t*) &ipv4addr, &timeval, &ntpTimeStamp);
        }
        fdCloseSession(TaskSelf());
        if (r)
        {
            sprintf(lm, "update_sntp_time: SNTP_getTimeByAddr %d", r);
            logevent(L_ERROR, lm);

            ipv4addr.sin_addr.s_addr = ip_wifi;
            r = SNTP_getTimeByAddr((SlNetSock_Addr_t*) &ipv4addr, &timeval, &ntpTimeStamp);
            if (r)
            {
                sprintf(lm, "update_sntp_time: SNTP_getTimeByAddr %d", r);
                logevent(L_ERROR, lm);
                mem_free(dbd, sizeof(dbt_device));
                return 0;
            }
        }
        else
        {
            ts.tv_sec = (ntpTimeStamp >> 32) - 2208988800;
            ts.tv_nsec = ((uint64_t) (ntpTimeStamp & 0xFFFFFFFF) * 1000000000) >> 32;
            nt = true;
        }
    }

    mem_free(dbd, sizeof(dbt_device));

    if (nt)
    {
        // Get semaphore for localtime
        Semaphore_pend(sem_ltime, BIOS_WAIT_FOREVER);

        // Update clock time
        clock_settime(CLOCK_REALTIME, &ts);

        // Release semaphore for localtime
        Semaphore_post(sem_ltime);

        get_ltime(&now);
        sprintf(lm, "update_sntp_time: %02d:%02d:%02d %02d/%02d/%d", now.tm_hour, now.tm_min, now.tm_sec, now.tm_mday, now.tm_mon + 1, now.tm_year + 1900);
        printf(lm);
        logevent(L_INFO, lm);
        return 1;
    }
    else
    {
        return 0;
    }
}

int8_t firmware_update(uint8_t version)
{
    char name[20] = "firmware.fw";

    char request[25];
    char lm[60];

    sprintf(request, "/firmware/prod/%d.fw\0", version);

    sprintf(lm, "http_get: %s %s", HOSTNAME, request);
    logevent(L_DEBUG, lm);

    bool moreDataFlag = false;
    char *data = mem_alloc(DATA_SIZE);

    int16_t ret = 0;
    uint32_t len = 0;
    HTTPClient_Handle httpClientHandle;
    int16_t statusCode;

    msg_led_t l;
    db_event_t e;

    l.led = LS_UPDATE;
    l.state = true;
    if (Mailbox_post(m_led, &l, BIOS_WAIT_FOREVER) == false)
    {
        err_mailbox_full(__func__, m_led);
    }

    //Report status to platform
    command_101(FIRMWARE_UPGRADE);
    Task_sleep(1000);

    //Update database
    e = DBE_EXPORT;
    if (Mailbox_post(m_db, &e, BIOS_WAIT_FOREVER) == false)
    {
        err_mailbox_full(__func__, m_db);
    }
    Task_sleep(3000);

    Semaphore_post(sem_mq_disc);

    Task_delete(&t_mqtt_send);
    Task_delete(&t_mqtt_client);
    Task_delete(&t_mqtt_run);

    Task_sleep(2000);

    fdOpenSession(TaskSelf());

    httpClientHandle = HTTPClient_create(&statusCode, 0);
    if (statusCode < 0)
    {
        sprintf(lm, "firmware_update: creation of http client handle failed %d\n", ret);
        logevent(L_ERROR, lm);
        return -1;
    }

    ret = HTTPClient_setHeader(httpClientHandle, HTTPClient_HFIELD_REQ_USER_AGENT, USER_AGENT, strlen(USER_AGENT) + 1, HTTPClient_HFIELD_PERSISTENT);
    if (ret < 0)
    {
        sprintf(lm, "firmware_update: setting request header failed %d\n", ret);
        logevent(L_ERROR, lm);
        return -1;
    }

    ret = HTTPClient_connect(httpClientHandle, HOSTNAME, 0, 0);
    if (ret < 0)
    {
        sprintf(lm, "firmware_update: HTTP client connect failed %d", ret);
        logevent(L_ERROR, lm);
        HTTPClient_destroy(httpClientHandle);
        fdCloseSession(TaskSelf());
        return -1;
    }

    ret = HTTPClient_sendRequest(httpClientHandle, HTTP_METHOD_GET, &request[0], NULL, 0, 0);
    if (ret != HTTP_SC_OK)
    {
        sprintf(lm, "firmware_update: HTTP client send failed %d", ret);
        logevent(L_ERROR, lm);
        HTTPClient_destroy(httpClientHandle);
        fdCloseSession(TaskSelf());
        return -1;
    }

    spiffs_file fd;
    SPIFFS_remove(&fs_spiffs, name);
    fd = SPIFFS_open(&fs_spiffs, name, SPIFFS_CREAT | SPIFFS_WRONLY, 0);

    len = 0;
    do
    {
        ret = HTTPClient_readResponseBody(httpClientHandle, data, DATA_SIZE, &moreDataFlag);
        if (ret < 0)
        {
            sprintf(lm, "firmware_update: HTTP response body error %d", ret);
            logevent(L_ERROR, lm);
        }

        if (SPIFFS_write(&fs_spiffs, fd, data, ret) < 0)
        {
            sprintf(lm, "firmware_update: writing file error");
            logevent(L_ERROR, lm);
            mem_free(data , DATA_SIZE);

            SPIFFS_close(&fs_spiffs, fd);
            SPIFFS_remove(&fs_spiffs, name);

            HTTPClient_disconnect(httpClientHandle);
            HTTPClient_destroy(httpClientHandle);
            fdCloseSession(TaskSelf());
            return -1;
        }

        len += ret;
    }
    while (moreDataFlag);

    mem_free(data , DATA_SIZE);

    SPIFFS_close(&fs_spiffs, fd);

    HTTPClient_disconnect(httpClientHandle);
    HTTPClient_destroy(httpClientHandle);
    fdCloseSession(TaskSelf());

    Task_sleep(5000);
    Semaphore_post(sem_reboot);

    return 0;
}

uint8_t http_get(char *server, char *request, char **response, uint16_t maxlength)
{
    HTTPClient_Handle httpClientHandle;
    int16_t r;
    uint8_t rv = 1;
    char lm[60];
    bool md;

    fdOpenSession(TaskSelf());

    sprintf(lm, "http_get: %s %s", server, request);
    logevent(L_DEBUG, lm);

    httpClientHandle = HTTPClient_create(&r, 0);
    if (r < 0)
    {
        sprintf(lm, "http_get: HTTP client creation error %d", r);
        logevent(L_ERROR, lm);
        fdCloseSession(TaskSelf());
        return 0;
    }

    r = HTTPClient_setHeader(httpClientHandle, HTTPClient_HFIELD_REQ_USER_AGENT,
    USER_AGENT,
                             strlen(USER_AGENT) + 1, HTTPClient_HFIELD_PERSISTENT);
    if (r < 0)
    {
        sprintf(lm, "http_get: HTTP client getting request header failed %d", r);
        logevent(L_ERROR, lm);
        fdCloseSession(TaskSelf());
        return 0;
    }

    r = HTTPClient_connect(httpClientHandle, server, 0, 0);
    if (r < 0)
    {
        sprintf(lm, "http_get: HTTP client connect failed %d", r);
        logevent(L_ERROR, lm);
        HTTPClient_destroy(httpClientHandle);
        fdCloseSession(TaskSelf());
        return 0;
    }

    r = HTTPClient_sendRequest(httpClientHandle, HTTP_METHOD_GET, request, NULL, 0, 0);
    if (r != HTTP_SC_OK)
    {
        sprintf(lm, "http_get: HTTP client send failed %d", r);
        logevent(L_ERROR, lm);
        HTTPClient_destroy(httpClientHandle);
        fdCloseSession(TaskSelf());
        return 0;
    }

    r = HTTPClient_readResponseBody(httpClientHandle, response, maxlength, &md);
    if (r < 0)
    {
        sprintf(lm, "http_get: HTTP response body error %d", r);
        logevent(L_ERROR, lm);
        rv = 0;
    }

    if (md)
    {
        sprintf(lm, "http_get: HTTP response body larger than buffer!");
        logevent(L_ERROR, lm);
        rv = 99;
    }

    HTTPClient_disconnect(httpClientHandle);
    HTTPClient_destroy(httpClientHandle);
    fdCloseSession(TaskSelf());
    return rv;
}
