/*
 * localapp.c
 *
 *  Created on: 28 feb 2024
 */

#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/nettools/netcfg.h>
#include <ti/ndk/inc/socketndk.h>

#include <ti/ndk/slnetif/slnetifndk.h>
#include <ti/net/slnet.h>
#include <ti/net/slnetif.h>
#include <ti/net/slnetutils.h>

#include "common.h"
#include "localapp.h"

uint8_t appcount = 0; // active app connections
SOCKET appcon[APPSVR_MAXCONN] = { 0 };
Semaphore_Handle sem_appcon;
/*
void appsvr_service(UArg a0, UArg a1)
{
    struct sockaddr_in server_addr, client_addr;
    SOCKET listen_sock, client_sock;
    int len = sizeof(struct sockaddr_in);
    struct timeval timeout = { 10, 0 };
    Task_Params params;
    int error = 0;

    while (1)
    {
        if (sem_appcon == 0)
        {
            Semaphore_Params sp;
            Semaphore_Params_init(&sp);
            sp.mode = Semaphore_Mode_BINARY;
            sp.instance->name = "sem_appcon";
            sem_appcon = Semaphore_create(1, &sp, NULL);
        }

        memset(&server_addr, 0, sizeof(struct sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = NDK_htonl(INADDR_ANY);
        server_addr.sin_port = NDK_htons(APPSVR_PORT);

        fdOpenSession(TaskSelf());

        listen_sock = NDK_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_sock == INVALID_SOCKET)
        {
            error = fdError();
            NDK_shutdown(listen_sock, SHUT_RDWR);
            fdClose(listen_sock);
            fdCloseSession(TaskSelf());
            continue;
        }

        memset(&server_addr, 0, sizeof(struct sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        server_addr.sin_port = NDK_htons(APPSVR_PORT);

        if (NDK_bind(listen_sock, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
        {
            error = fdError();
            NDK_shutdown(listen_sock, SHUT_RDWR);
            fdClose(listen_sock);
            // Close the file session
            fdCloseSession(TaskSelf());
            continue;
        }

        if (NDK_listen(listen_sock, 5) == SOCKET_ERROR)
        {
            error = fdError();
            fdClose(listen_sock);
            continue;
        }

        // Set listen timeout to make watchdog work
        timeout.tv_sec = APPSVR_LTO;
        if (NDK_setsockopt(listen_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == SOCKET_ERROR)
        {
            error = fdError();
            fdClose(listen_sock);
            continue;
        }

        while (1)
        {
            if (get_appcount() == APPSVR_MAXCONN)
            {
                // Maximum connections reached
                Task_sleep(5000);
                continue;
            }
            client_sock = NDK_accept(listen_sock, (struct sockaddr*) &client_addr, &len);
            if (client_sock == INVALID_SOCKET)
            {
                error = fdError();
                // called when socket closed - TODO improve error handling
                NDK_shutdown(client_sock, SHUT_RDWR);
                fdClose(client_sock);
                ;
                break;
            }
            else
            {
                // Create new connection task
                Task_Params_init(&params);
                params.priority = TASK_PRIO_MEDIUM;
                params.stackSize = TASK_NORMAL;
                params.instance->name = "appsvr_connection";
                params.arg0 = (UArg) client_sock;
                Task_create(appsvr_connection, &params, NULL);
            }
        }
        NDK_shutdown(listen_sock, SHUT_RDWR);

        fdClose(listen_sock);

        // Close the file session
        fdCloseSession(TaskSelf());
    }
}
*/
/*void appsvr_connection(UArg a0, UArg a1)
 {
 SOCKET s = (SOCKET) a0;
 uint8_t ulev;
 int32_t opt = 1;
 int64_t j;
 uint16_t i = 0;
 int16_t n = 0;
 char buffer[TMSG_MAXSIZE], *p1, *p2, *p3;
 p1 = NULL;
 msg_dbev_t msg;
 msg_event_t ev_out;
 msg_rpc_t rpc;
 bool dc = false, auth = false;
 uint16_t id_user_auth = 0;
 db_event_t d;
 dbt_user *user;
 struct timeval to;
 struct tm now;
 char lm[50];

 sprintf(lm, "new app connection");
 logevent(L_DEBUG, lm);

 // Open the file session
 fdOpenSession(TaskSelf());

 // Disables the Nagle algorithm for TCP sockets
 NDK_setsockopt(s, IPPROTO_TCP, NDK_TCP_NODELAY, &opt, sizeof(opt));

 // Set receive timeout to 120 seconds
 to.tv_sec = 120;
 to.tv_usec = 0;
 setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

 msg.s = s;

 while (1)
 {
 if (p1 == NULL)
 {
 n = NDK_recv(s, &buffer[i], TMSG_MAXSIZE - 1 - i, 0);
 }

 if (n <= 0)
 {
 // Socket closed or error or timeout, disconnect
 n = fdError();
 if (n == 35)
 {
 // Connection timeout
 }
 else
 {
 // Connection error
 }
 dc = true;
 break;
 }
 else if (n > 0 || i)
 {
 // Data received
 i += n;
 buffer[i] = 0;
 p1 = strstr(buffer, "###");
 if (p1)
 {
 // Message found
 p2 = buffer;
 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 p2 += n;
 }

 switch (j)
 {
 case AM_DBSET:
 if (auth)
 {
 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 msg.tab_id = j;
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }
 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 msg.rec_id = j;
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }
 if (ulev == 3 || (msg.tab_id == USER && msg.rec_id == id_user_auth))
 {
 if (p1 > p2)
 {
 n = p1 - p2;
 msg.entry = mem_alloc(n + 1);
 strncpy(msg.entry, p2, n);
 msg.entry[n] = 0;
 msg.s = s;
 parse_dbset(&msg);
 mem_free(msg.entry, n + 1);
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }
 }
 }
 break;
 case AM_DBDELETE:
 if (auth && ulev == 3)
 {
 msg.esize = 30;
 msg.entry = mem_alloc(30);

 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 msg.tab_id = j;
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }

 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 msg.rec_id = j;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }

 if (parse_dbdelete(&msg))
 {
 //Send new table records without the deleted item

 d = DBE_BACKUP;
 Mailbox_post(m_db, &d, BIOS_WAIT_FOREVER);

 msg.rec_id = 65535;
 parse_dbget(&msg);
 }
 }
 break;
 case AM_DBGET:
 if (auth)
 {
 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 msg.tab_id = j;
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }

 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 msg.rec_id = j;
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }
 msg.s = s;
 parse_dbget(&msg);
 }
 break;
 case AM_DBVER:
 if (auth)
 {
 msg.s = s;
 msg.esize = 5 * N_DBTABLES + 5;
 msg.entry = malloc(msg.esize);
 sprintf(msg.entry, "%d,", AM_DBVER);
 get_tversion_all(&(msg.entry[2]));
 strcat(msg.entry, "###");
 Mailbox_post(m_lappsend, &msg, BIOS_WAIT_FOREVER);
 }
 break;
 case AM_RESTART:
 if (auth && ulev == 3)
 {
 // Request system reboot
 Semaphore_post(sem_reboot);
 }
 break;
 case AM_HELLO:
 msg.esize = 6;
 msg.entry = malloc(msg.esize);
 sprintf(msg.entry, "%d###", AM_HELLO);
 Mailbox_post(m_lappsend, &msg, BIOS_WAIT_FOREVER);
 break;
 case AM_DISCONNECT:
 dc = true;
 break;
 case AM_AUTH:
 auth = false;
 msg.entry = mem_alloc(30);
 msg.esize = 30;
 p3 = mem_alloc(30);
 n = get_next_dbset(p2, &j, p3);
 if (n)
 {
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }

 j = 0;
 get_next_dbset(p2, &j, NULL);

 user = find_element(&config_db.db_user, &config_db.db_user->name, false, USER, STRING, 0, 0, p3, 0);
 if (user)
 {
 if (user->pw == j)
 {
 if (auth == false)
 {
 // Register socket and increase connection counter to get status updates
 add_appcon(s);
 auth = true;
 id_user_auth = user->id;
 }

 ulev = user->level;
 }
 else
 {
 if (auth)
 {
 // Unregister socket
 remove_appcon(s);
 auth = false;
 }
 }
 }

 sprintf(msg.entry, "6,%d,%s,%d###", auth, p3, get_tversion(0));
 Mailbox_post(m_lappsend, &msg, BIOS_WAIT_FOREVER);
 mem_free(user, sizeof(dbt_user));
 mem_free(p3, 30);
 break;
 case AM_SWITCH_ITEM:
 if (auth && ulev > 1)
 {
 msg.tab_id = ITEM;
 //p2 = buffer + 2;
 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 // Item ID
 ev_out.unit = 0;
 ev_out.port = j;
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }

 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 // New state
 ev_out.state = j;
 p2 += n;
 n = p1 - p2;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }

 Mailbox_post(m_event_out, &ev_out, BIOS_WAIT_FOREVER);
 }
 break;
 case AM_SWITCH_RULE:
 break;
 case AM_TIME:
 if (auth && ulev > 1)
 {
 get_ltime(&now);
 msg.esize = sizeof(now);
 msg.entry = malloc(msg.esize);
 sprintf(msg.entry, "%d,%d,%d,%d,%d,%d###", AM_TIME, now.tm_mday, now.tm_mon, now.tm_year, now.tm_hour, now.tm_min);
 Mailbox_post(m_lappsend, &msg, BIOS_WAIT_FOREVER);
 }
 break;
 case AM_SWITCH_GROUP:
 if (auth && ulev > 1)
 {
 msg.tab_id = GROUP;
 //p2 = buffer + 2;
 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 //GROUP ID
 ev_out.unit = 0;
 ev_out.port = j;
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }

 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 // New state
 ev_out.state = j;
 p2 += n;
 n = p1 - p2;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }

 set_group_state(ev_out.port, ev_out.state);
 }
 break;

 case AM_ZIGBEE:
 if (auth && ulev > 1)
 {
 n = get_next_dbset(p2, &j, NULL);
 if (n)
 {
 rpc.cmd0 = j;
 p2 += n;
 }
 else
 {
 // Invalid message
 dc = true;
 break;
 }
 rpc.is_app = true;
 n = p1 - p2;
 rpc.length = n + 3;
 rpc.data = mem_alloc(rpc.length);
 strncpy(rpc.data, p2, n + 2);
 rpc.data[n + 3] = 0;

 Mailbox_post(m_rpc_send, &rpc, BIOS_WAIT_FOREVER);
 }
 break;
 }

 if (p1[3])
 {
 // Move start of next message to beginning of buffer
 p1 += 3;
 i = strlen(p1);
 strcpy(buffer, p1);
 p1 = strstr(buffer, "###");
 }
 else
 {
 // Reset buffer
 i = 0;
 p1 = NULL;
 }

 if (dc)
 {
 break;
 }
 }
 }
 }

 if (auth)
 {
 // Unregister socket
 remove_appcon(s);
 }

 NDK_shutdown(s, SHUT_RDWR);

 // Close the client socket
 fdClose(s);

 // Close the file session
 fdCloseSession(TaskSelf());

 sprintf(lm, "app disconnection");
 logevent(L_DEBUG, lm);

 // End task
 Task_exit();
 }*/
/*
void appsvr_send(UArg a0, UArg a1)
{
    uint8_t i;
    uint16_t s;
    int16_t n;
    msg_dbev_t msg;

    // Open the file session
    fdOpenSession(TaskSelf());

    while (1)
    {
        Mailbox_pend(m_lappsend, &msg, BIOS_WAIT_FOREVER);

        if (msg.s)
        {
            n = NDK_send(msg.s, msg.entry, strlen(msg.entry), 0);
            if (n == -1)
            {
                n = fdError();
                //sprintf(lm, "sll_send_message - send error: %d, %s", fdError(), r);
                //logevent(L_ERROR, lm);
            }
        }
        else
        {
            // Send to all connected clients
            s = strlen(msg.entry);
            Semaphore_pend(sem_appcon, BIOS_WAIT_FOREVER);
            for (i = 0; i < appcount; i++)
            {
                n = NDK_send(appcon[i], msg.entry, s, 0);
                if (n == -1)
                {
                    //sprintf(lm, "sll_send_message - send error: %d, %s", fdError(), r);
                    //logevent(L_ERROR, lm);
                }
            }
            Semaphore_post(sem_appcon);
        }

        mem_free(msg.entry, msg.esize);
    }
}

uint8_t get_appcount()
{
    uint8_t c;

    Semaphore_pend(sem_appcon, BIOS_NO_WAIT);
    c = appcount;
    Semaphore_post(sem_appcon);
    return c;
}

void add_appcon(SOCKET s)
{
    // Function does not check max. value as it is assumed the server task has done that
    bool first = false;

    Semaphore_pend(sem_appcon, BIOS_WAIT_FOREVER);
    appcon[appcount] = s;
    appcount++;
    if (appcount == 1)
    {
        first = true;
    }
    Semaphore_post(sem_appcon);

    if (first)
    {
        set_app_connected(true);
    }
}

void remove_appcon(SOCKET s)
{
    uint8_t a;
    int16_t id = -1;

    Semaphore_pend(sem_appcon, BIOS_WAIT_FOREVER);
    for (a = 0; a < appcount; a++)
    {
        if (appcon[a] == s)
        {
            id = a;
        }
    }

    if (id != -1)
    {
        if (appcount > id + 1)
        {
            // Move last ID the position of the Id to be deleted
            appcon[id] = appcon[appcount - 1];
        }
        appcon[appcount - 1] = 0;
        appcount--;
    }
    a = appcount;
    Semaphore_post(sem_appcon);

    if (appcount == 0)
    {
        // No clients connected anymore
        set_app_connected(false);
    }
}
*/
void finder_service(UArg a0, UArg a1)
{
    struct sockaddr_in broadcast_addr, client_addr;
    SOCKET sock_r, sock_s;
    char buffer[100];
    int broadcast_enable = 1;
    dbt_device *dev;
    uint16_t n;

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);
    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    // Wait for NW ready
    Semaphore_pend(sem_nw_ready, BIOS_WAIT_FOREVER);
    // Release the read semaphore
    Semaphore_post(sem_nw_ready);

    fdOpenSession(TaskSelf());

    if ((sock_r = NDK_socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        fdCloseSession(TaskSelf());
        return;
    }

    if ((sock_s = NDK_socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        fdClose(sock_r);
        fdCloseSession(TaskSelf());
        return;
    }

    setsockopt(sock_s, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = NDK_htons(FINDER_RCV);

    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = NDK_htons(FINDER_SND);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    if (NDK_bind(sock_r, (struct sockaddr*) &client_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
    {
        fdClose(sock_r);
        fdClose(sock_s);
        fdCloseSession(TaskSelf());
        return;
    }

    while (1)
    {
        n = NDK_recvfrom(sock_r, (char*) buffer, sizeof(buffer), MSG_WAITALL, NULL, NULL);
        buffer[n] = 0;

        if (strcmp(buffer, "coqon") == 0)
        {
            dev = get_element(&config_db.db_device, DEVICE);
            if (dev)
            {
                sprintf(buffer, "SW-RT MAC:%c%c:%c%c:%c%c:%c%c:%c%c:%c%c IP:%d.%d.%d.%d FW_V:%d HW_V:%d DB_V:%d", dev->mac[0], dev->mac[1], dev->mac[2],
                        dev->mac[3],
                        dev->mac[4], dev->mac[5], dev->mac[6], dev->mac[7], dev->mac[8], dev->mac[9], dev->mac[10], dev->mac[11],
                        (uint8_t) (dev->cur_ip >> 24),
                        (uint8_t) (dev->cur_ip >> 16), (uint8_t) (dev->cur_ip >> 8), (uint8_t) (dev->cur_ip >> 0), dev->vfw,
                        dev->vhw,
                        dev->vdb);
                mem_free(dev, sizeof(dbt_device));
                NDK_sendto(sock_s, buffer, strlen(buffer), 0, (const struct sockaddr*) &broadcast_addr, sizeof(broadcast_addr));
            }
        }
    }
}
