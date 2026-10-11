/*
 * evlog.c
 *
 *  Created on: 9 nov 2023
 */

#include "common.h"
#include "evlog.h"

Task_Handle t_logger_udp = NULL;
Semaphore_Handle sem_logready;
bool logready = false, fr;

void logger(UArg arg0, UArg arg1)
{
    msg_log_t msg;
    msg_mqtt_t msg_mqtt;
    int8_t ll;
    #ifdef _LOGUDP_
    uint16_t port;
    #endif
    char mac[18];

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);

    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    // Wait for spiffs ready
    Semaphore_pend(sem_spiffs_mount, BIOS_WAIT_FOREVER);

    // Release the read semaphore
    Semaphore_post(sem_spiffs_mount);

    #ifdef _LOGUDP_
    port = get_logudp();
    #endif
    Semaphore_pend(sem_logready, BIOS_WAIT_FOREVER);
    logready = true;
    Semaphore_post(sem_logready);

    while (1)
    {
        // Receive the message from the mailbox
        Mailbox_pend(m_evlog, &msg, BIOS_WAIT_FOREVER);
        fr = true;
        ll = get_loglevel();

        #ifdef _LOGUDP_
        if (strcmp(msg.msg, "port") == 0)
        {
            // Port change
            port = get_logudp();
        }
        #endif

        if (ll == L_DEBUG && t_mqtt_send)
        {
            db_get_mac(mac);
            msg_mqtt.buf = NULL;
            msg_mqtt.size_buf = 0;

            msg_mqtt.size_topic = 60;
            msg_mqtt.topic = mem_alloc(msg_mqtt.size_topic);
            sprintf(msg_mqtt.topic, "smaway/%s/tocloud/config/122\0", mac);

            msg_mqtt.size_content = strlen(msg.msg)+1;
            msg_mqtt.content = mem_alloc(msg_mqtt.size_content);
            sprintf(msg_mqtt.content, "%s", msg.msg);

           if(!(sendToMqttCheckingMailboxNoWait(&msg_mqtt)/*Mailbox_post(m_mqtt_send, &msg_mqtt, BIOS_NO_WAIT)*/))
           {
               mem_free(msg_mqtt.buf, msg_mqtt.size_buf);
               mem_free(msg_mqtt.topic, msg_mqtt.size_topic);
           }

        }

        if (msg.level == L_EVENT && ll > 0)
        {

            log2_spiffs(false, msg.msg);

            #ifdef _LOGUDP_
            if (port && t_logger_udp && ll == L_DEBUG)
            {
                if (Mailbox_post(m_log_udp, &msg, BIOS_NO_WAIT))
                {
                    fr = false;
                }
            }
            #endif
        }
        else if (abs(ll) <= msg.level)
        {

            log2_spiffs(true, msg.msg);

            #ifdef _LOGUDP_
            if (port && t_logger_udp)
            {
                if (Mailbox_post(m_log_udp, &msg, BIOS_NO_WAIT))
                {
                    fr = false;
                }
            }
            #endif
        }

        if (fr)
        {
            // If UDP is enabled, UDP process will free memory
            mem_free(msg.msg, msg.size);
        }
    }
}

void logevent(loglevel ll, char *t)
{
    #ifdef _USELOGS_
    msg_log_t msg;
    char *txt, lt;
    struct tm now;

    if (get_logready())
    {
        msg.size = strlen(t) + 24;
        txt = mem_alloc(msg.size);
        get_ltime(&now);

        switch (ll)
        {
            case L_DEBUG:
                lt = 'D';
                break;
            case L_INFO:
                lt = 'I';
                break;
            case L_ERROR:
                lt = 'R';
                break;
            case L_EVENT:
                lt = 'E';
                break;
        }
        sprintf(txt, "%02d/%02d/%02d %02d:%02d:%02d %c: %s\n", now.tm_mday, now.tm_mon + 1, now.tm_year - 100, now.tm_hour, now.tm_min, now.tm_sec, lt,
                t);

        msg.level = ll;
        msg.msg = txt;
        if (Mailbox_post(m_evlog, &msg, BIOS_NO_WAIT) == false)
        {
            mem_free (msg.msg, msg.size);
        }
    }
    #endif
}

static bool get_logready()
{
    bool r;
    Semaphore_pend(sem_logready, BIOS_WAIT_FOREVER);
    r = logready;
    Semaphore_post(sem_logready);
    return r;
}

int8_t send_logs()
{
    char paths[MAX_STR_LEN];
    spiffs_file fd;
    uint32_t i = 0;
    int32_t fileSize = 0;
    uint8_t par_counter = 1;
    msg_mqtt_t msg;

    char mac[18];
    db_get_mac(mac);

    char *buffer;
    buffer = mem_alloc(1025); //5 kB packs of information

    uint16_t counter = 1;


    for(i = 0; i < counter; i++)
    {
        get_element_file(LOG_PATH, &counter, paths, i);
        par_counter = 1;
        fd = SPIFFS_open(&fs_spiffs, paths, SPIFFS_RDONLY, 0);

        if(fd > 0)
        {
            do
            {
                fileSize = SPIFFS_read(&fs_spiffs, fd, buffer, 512);
                if (fileSize < 0)
                {
                    // Handle error
                    return false;
                }

                if(fileSize!=0)
                {
                    buffer[fileSize] = '\0';

                    msg.size_topic = 70;
                    msg.topic = mem_alloc(msg.size_topic);
                    sprintf(msg.topic, "smaway/%s/tocloud/config/121", mac);

                    msg.size_content = 80 + fileSize;
                    msg.content = mem_alloc(msg.size_content);
                    sprintf(msg.content, "{\"FileName\":\"%s\",\"Par\":%d,\"Data\":\"%s\"}",paths, par_counter, buffer);

                    msg.size_buf = 0;
                    msg.buf = NULL;

                    sendToMqttCheckingMailbox(&msg);
                    //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
                    Task_sleep(2500);
                }
                par_counter++;

            }while(fileSize != 0);


            SPIFFS_close(&fs_spiffs, fd);
        }
    }
    mem_free(buffer, 1025);
    return 0;
}

