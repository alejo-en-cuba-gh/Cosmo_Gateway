/*
 * network.c
 *
 *  Created on: 1 nov 2023
 */

#include <strings.h>

#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/stkmain.h>
#include <ti/ndk/inc/_stack.h>
#include <ti/ndk/inc/nettools/netcfg.h>

#include <ti/ndk/slnetif/slnetifndk.h>
#include <ti/net/slnet.h>
#include <ti/net/slnetif.h>
#include <ti/net/slnetutils.h>

#include <ti/drivers/emac/EMACMSP432E4.h>

#include "common.h"
#include "network.h"
#include "network_ndk.h"

#define INET_ADDRSTRLEN 16

Semaphore_Handle sem_ip_update, sem_nw_ready, sem_nw_connect, sem_sntp, sem_online;
bool nw_ready = false, nw_connect = false, nw_connecting = false, temp_ip = false;

void net_init(UArg arg0, UArg arg1)
{
    int status = -1;
    char lm[75];
    msg_led_t l;

    l.led = LS_NET;
    l.state = true;
    if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false)
    {
        err_mailbox_full(__func__, m_led);
    }

    // initialize SlNet interface(s)
    while (status < 0)
    {
        status = ti_net_SlNet_initConfig();
        if (status < 0)
        {
            sprintf(lm, "Failed to initialize SlNet interface(s) - status (%d)", status);
            logevent(L_ERROR, lm);
        }

        Task_sleep(5000);
    }

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);

    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    ip_config();
    Task_sleep(2000);
    //ftpserver_init();
    update_sntp_time();
    Semaphore_post(sem_sntp);
    // In the future, use this task to start/stop network services
    // End it for now when jobs are done
    Task_exit();
}

void net_update(UArg arg0, UArg arg1)
{
    dbt_device *dbd;
    uint32_t lan_ip, lan_gw, lan_dns, lan_sn;
    int size;
    void *hCfg = NULL;
    void *handle;
    CI_IPNET ip_cfg;
    CI_ROUTE ip_route;
    char lm[250];
    msg_led_t l;

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);

    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    while (1)
    {
        Semaphore_pend(sem_ip_update, BIOS_WAIT_FOREVER);
        Task_sleep(2000);

        CfgGetEntryCnt(hCfg, CFGTAG_IPNET, 1);

        size = sizeof(CI_IPNET);
        CfgGetEntry(hCfg, CFGTAG_IPNET, 1, 1, &handle);
        CfgEntryGetData(handle, &size, (uint8_t*) &ip_cfg);

        size = sizeof(CI_ROUTE);
        CfgGetEntry(hCfg, CFGTAG_ROUTE, 0, 1, &handle);
        CfgEntryGetData(handle, &size, (uint8_t*) &ip_route);
        lan_ip = htonl(ip_cfg.IPAddr);
        lan_sn = htonl(ip_cfg.IPMask);
        lan_gw = htonl(ip_route.IPGateAddr);

        CfgGetImmediate(hCfg, CFGTAG_SYSINFO, CFGITEM_DHCP_DOMAINNAMESERVER, 1, sizeof(lan_dns), (unsigned char*) &lan_dns);
        lan_dns = htonl(lan_dns);

        // Get device table
        dbd = get_element(&config_db.db_device, DEVICE);

        sprintf(lm, "net_update: IP Configuration\n"
                "  MAC Address . . . . %c%c:%c%c:%c%c:%c%c:%c%c:%c%c\n"
                "  IPv4 Address  . . . %d.%d.%d.%d\n"
                "  Subnet Mask . . . . %d.%d.%d.%d\n"
                "  Default Gateway . . %d.%d.%d.%d\n"
                "  DNS server . . . .  %d.%d.%d.%d",
                dbd->mac[0], dbd->mac[1], dbd->mac[2], dbd->mac[3], dbd->mac[4], dbd->mac[5], dbd->mac[6], dbd->mac[7], dbd->mac[8], dbd->mac[9], dbd->mac[10],
                dbd->mac[11],
                (uint8_t) (lan_ip >> 24), (uint8_t) (lan_ip >> 16), (uint8_t) (lan_ip >> 8), (uint8_t) (lan_ip >> 0), (uint8_t) (lan_sn >> 24),
                (uint8_t) (lan_sn >> 16),
                (uint8_t) (lan_sn >> 8), (uint8_t) (lan_sn >> 0), (uint8_t) (lan_gw >> 24), (uint8_t) (lan_gw >> 16),
                (uint8_t) (lan_gw >> 8),
                (uint8_t) (lan_gw >> 0), (uint8_t) (lan_dns >> 24), (uint8_t) (lan_dns >> 16), (uint8_t) (lan_dns >> 8),
                (uint8_t) (lan_dns >> 0));
        logevent(L_INFO, lm);

        // Store current configuration
        dbd->cur_ip = lan_ip;
        dbd->cur_sn = lan_sn;
        dbd->cur_gw = lan_gw;
        dbd->cur_dns = lan_dns;
        set_element((dbt_generic**) &dbd, DEVICE, true);

        if (nw_ready == false)
        {
            // Semaphore for processes waiting for 1st network start
            nw_ready = true;
            Semaphore_post(sem_nw_ready);
            Semaphore_post(sem_online);
        }

        if (nw_connected() == false)
        {
            // Semaphore for LAN status
            Semaphore_pend(sem_nw_connect, BIOS_WAIT_FOREVER);
            nw_connect = true;
            nw_connecting = false;
            Semaphore_post(sem_nw_connect);

            l.led = LS_NET;
            l.state = false;
            if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false)
            {
                err_mailbox_full(__func__, m_led);
            }
        }
        update_sntp_time();
    }
}

void net_upstate(UArg arg0, UArg arg1)
{
    msg_led_t l;
    // Wait 17+3 seconds until first start to get NW stack running
    Task_sleep(17000);
    uint8_t dhcp_check = 0;
    char lm[65];
    CI_IPNET NA;
    void *hCfg = NULL;

    while (1)
    {
        Task_sleep(3000);
        Semaphore_pend(sem_nw_connect, BIOS_WAIT_FOREVER);
        if (EMACMSP432E4_isLinkUp())
        {
            if (nw_connect == false)
            {
                // network state changed to up
                // Do not change nw_connect yet, ip_config will trigger an ip_update which will set the variable
                if (nw_connecting == false)
                {
                    ip_config();
                    nw_connecting = true;
                }
                else
                {
                    if (get_dhcp())
                    {
                        if (dhcp_check == 2)
                        {
                            dhcp_check = 0;
                            remove_ip();

                            // Set temporary static IP
                            memset(&NA, 0, sizeof(NA));
                            NA.IPAddr = 1342286016; //192.168.1.80
                            NA.IPMask = 16777215; //255.255.255.0
                            NA.NetType = 0;
                            CfgAddEntry(hCfg, CFGTAG_IPNET, 1, 0, sizeof(CI_IPNET), (unsigned char*) &NA, 0);
                            sprintf(lm, "dhcp_check - DHCP request failed, set static IP 192.168.1.80");
                            logevent(L_ERROR, lm);
                            temp_ip = true;
                        }
                        else
                        {
                            dhcp_check++;
                        }
                    }
                }
                Task_sleep(10000);
            }
            else if (temp_ip)
            {
                // 5 minutes with emergency IP
                if (dhcp_check == 100)
                {
                    dhcp_check = 0;
                    if (get_ftp_connect() == false)
                    {
                        // Reset temporary address and retry DHCP
                        temp_ip = false;
                        nw_connect = false;
                        l.led = LS_NET;
                        l.state = true;
                        if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false)
                        {
                            err_mailbox_full(__func__, m_led);
                        }
                        ip_config();
                        nw_connecting = true;
                    }
                }
                else
                {
                    dhcp_check++;
                }
            }
        }
        else
        {
            if (nw_connect)
            {
                // network state changed to down
                nw_connect = false;
                nw_connecting = false;
                dhcp_check = 0;
                temp_ip = false;
                /*
                 if (oem == M_SCANVEST)
                 {
                 // Disconnet Alphacom
                 set_sll_ready(false);
                 }*/

                l.led = LS_NET;
                l.state = true;
                if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false)
                {
                    err_mailbox_full(__func__, m_led);
                }
            }
        }
        Semaphore_post(sem_nw_connect);
    }
}

uint8_t ip_config()
{
    static uint8_t init = true;
    dbt_device *dbd;
    dbt_unit *dbu;
    uint8_t macAddress[6];
    uint16_t macAddressLen = 6;
    NIMU_IF_REQ if_req;
    void *hCfg = NULL;
    CI_IPNET NA;
    CI_ROUTE RT;
    if_req.name[0] = 0;
    if_req.index = 1; /* ethernet IF # */

    // Get device settings
    dbd = get_element(&config_db.db_device, DEVICE);
    //dbu = get_element(&config_db.db_unit, UNIT);
    NIMUIoctl(NIMU_GET_DEVICE_MAC, &if_req, macAddress, macAddressLen);

    // Update MAC address
    sprintf(dbd->mac, "%02X%02X%02X%02X%02X%02X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
    //strcpy(dbu->hwid, dbd->mac);
    if (init)
    {
        set_element((dbt_generic**) &dbd, DEVICE, false);
        //set_element(&dbu, UNIT, false, false);
        init = false;
    }

    // Delete current IP configuration
    remove_ip();
    Task_sleep(2000);

    if (dbd->lan_dhcp)
    {
        // Enable DHCP client
        startDHCP();
    }
    else
    {
        // Set static IP
        memset(&NA, 0, sizeof(NA));
        NA.IPAddr = ntohl(dbd->lan_ip);
        NA.IPMask = ntohl(dbd->lan_sn);
        NA.NetType = 0;
        CfgAddEntry(hCfg, CFGTAG_IPNET, 1, 0, sizeof(CI_IPNET), (unsigned char*) &NA, 0);

        memset(&RT, 0, sizeof(RT));
        RT.IPDestAddr = 0;
        RT.IPDestMask = 0;
        RT.IPGateAddr = ntohl(dbd->lan_gw);

        CfgAddEntry(hCfg, CFGTAG_ROUTE, 0, 0, sizeof(CI_ROUTE), (unsigned char*) &RT, 0);

        uint32_t IPAddr = ntohl(dbd->lan_dns);
        CfgAddEntry(hCfg, CFGTAG_SYSINFO, CFGITEM_DHCP_DOMAINNAMESERVER, 1, sizeof(IPAddr), (unsigned char*) &IPAddr, 0);
    }

    mem_free(dbd, sizeof(dbt_device));
    return 1;
}

void on_ip_change(uint32_t IPAddr, unsigned int IfIdx, unsigned int fAdd)
{
    uint32_t ip = htonl(IPAddr);
    char lm[60];

    if (fAdd)
    {
        sprintf(lm, "on_ip_change - Network Added: %d.%d.%d.%d", (uint8_t) (ip >> 24), (uint8_t) (ip >> 16), (uint8_t) (ip >> 8), (uint8_t) (ip >> 0));
        Semaphore_post(sem_ip_update);
    }
    else
    {
        sprintf(lm, "on_ip_change - Network Removed");
    }
    logevent(L_INFO, lm);
}

bool nw_connected()
{
    bool c;
    Semaphore_pend(sem_nw_connect, BIOS_WAIT_FOREVER);
    c = nw_connect;
    Semaphore_post(sem_nw_connect);
    return c;
}

void remove_ip()
{
    void *hCfg = 0, *hCfgIpAddr;
    void *entry;
    int status;
    int32_t ret;
    CI_IPNET ip;

    /* get the default configuration */
    hCfg = CfgGetDefault();

    /* get the DHCP client configuration */
    status = CfgGetEntry(hCfg, CFGTAG_SERVICE, CFGITEM_SERVICE_DHCPCLIENT, 1, &entry);
    if (status == 1)
    {
        // remove the DHCP client configuration
        status = CfgRemoveEntry(hCfg, entry);
    }

    hCfg = 0;
    // Remove any existing entries
    ret = CfgGetEntry(hCfg, CFGTAG_IPNET, 1, 1, &hCfgIpAddr);
    while (ret > 0)
    {
        ret = sizeof(ip);
        CfgEntryGetData(hCfgIpAddr, &ret, (uint8_t*) &ip);
        CfgRemoveEntry(hCfg, hCfgIpAddr);
        ret = CfgGetEntry(hCfg, CFGTAG_IPNET, 1, 1, &hCfgIpAddr);
    }
}

void startDHCP()
{
    void *hCfg = 0;
    hCfg = CfgGetDefault();

    /* Use DHCP to obtain IP address on interface 1 */
    {
        CI_SERVICE_DHCPC dhcpc;
        uint8_t DHCP_OPTIONS[] = {
        DHCPOPT_SUBNET_MASK,
                                   DHCPOPT_NAME_SERVERS, };

        /* Specify DHCP Service on IF specified by "IfIdx" */
        bzero(&dhcpc, sizeof(dhcpc));
        dhcpc.cisargs.Mode = 9;
        dhcpc.cisargs.IfIdx = 1;
        dhcpc.param.pOptions = DHCP_OPTIONS;
        dhcpc.param.len = 2;
        int ret = CfgAddEntry(hCfg, CFGTAG_SERVICE, CFGITEM_SERVICE_DHCPCLIENT, 0, sizeof(dhcpc), (uint8_t*) &dhcpc, 0);
        // System_printf("RET %d\n", ret); System_flush();
    }
}

int ping(char *ip)
{
    uint32_t IPAddr = inet_addr(ip);
    NDK_fd_set ibits;
    int cc, fromlen;
    uint16_t cnt_send = 0, cnt_recv = 0; /* Packets send and received */
    SOCKET s; /* ICMP set file descriptor */
    struct sockaddr_in to; /* who to ping */
    struct sockaddr_in from; /* who replied */
    char *pBuf = 0, lm[70];
    struct timeval timeout; /* Timeout struct for select */
    uint32_t timestart;

    // Create a raw set
    fdOpenSession(TaskSelf());
    s = NDK_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (s < 0)
    {
        logevent(L_ERROR, "ping - socket error");
        return 0;
    }

    /* Create to "to" address */
    memset(&to, 0, sizeof(struct sockaddr_in));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = IPAddr;

    /* Create to "from" address */
    memset(&from, 0, sizeof(struct sockaddr_in));
    from.sin_family = AF_INET;

    /* Set set option to allow us to broadcast */
    cc = 1;
    NDK_setsockopt(s, SOL_SOCKET, SO_BROADCAST, &cc, sizeof(int));

    /* Configure our timeout to be 0.5 seconds */
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    /* Allocate a working buffer */
    if (!(pBuf = mmBulkAlloc(MAXPACKET)))
    {
        logevent(L_ERROR, "ping - failed allocate working buffer");
        return 0;
    }

    while (cnt_send < 5)
    {
        /* Use send count for sequence - starts at "1" */
        cnt_send++;

        /* Build the ping packet */
        BuildPing(pBuf, cnt_send);

        /* Get the time now */
        timestart = llTimerGetTime(0);
        cnt_recv = 0;

        /* Send the ping */
        cc = NDK_sendto(s, pBuf, DATALEN, 0, (struct sockaddr*) &to, sizeof(to));
        if (cc < 0)
        {
            sprintf(lm, "ping - failed sendto (%d)", fdError());
            logevent(L_ERROR, lm);
        }
        else if (cc != DATALEN)
        {
            sprintf(lm, "ping - partial write (%d)", cc);
            logevent(L_ERROR, lm);
        }

        /* Select for timeout period second */
        NDK_FD_ZERO(&ibits);
        NDK_FD_SET(s, &ibits);
        /* fdSelect 1st arg is a don't care, pass 0 64-bit compatibility */
        fdSelect(0, &ibits, 0, 0, &timeout);

        again:
        /* Select for timeout period second */
        NDK_FD_ZERO(&ibits);
        NDK_FD_SET(s, &ibits);
        /* fdSelect 1st arg is a don't care, pass 0 64-bit compatibility */
        fdSelect(0, &ibits, 0, 0, &timeout);

        /* Check for a reply */
        if (NDK_FD_ISSET(s, &ibits))
        {
            fromlen = sizeof(from);
            cc = (int) NDK_recvfrom(s, pBuf, MAXPACKET, 0, (struct sockaddr*) &from, &fromlen);
            if (cc < 0)
            {
                sprintf(lm, "ping - failed recvfrom (%d)", fdError());
                logevent(L_ERROR, lm);
                goto pingleave;
            }

            /* Check the reply. Bump the recv count if good reply */
            if (CheckPing(pBuf, cnt_send, cc, &from))
            {
                cnt_recv++;
            }
        }

        /* Check for about 2 seconds */
        if ((timestart + 2) > llTimerGetTime(0))
        {
            goto again;
        }

        /* If send is more than reply, then we missed a reply */
        if (!cnt_recv)
        {
            logevent(L_INFO, "ping - reply timeout");
        }
    }

    pingleave: if (pBuf)
    {
        mmBulkFree(pBuf);
    }
    if (s != INVALID_SOCKET)
    {
        fdClose(s);
    }

    fdCloseSession(TaskSelf());
    return 1;
}

static void BuildPing(char *pBuf, uint16_t seq)
{
    ICMPHDR *pIcHdr;
    ICMPREQHDR *pReqHdr;
    int cc;

    /* Fill in the ping data buffer */
    for (cc = 8; cc < DATALEN; cc++)
        *(pBuf + cc) = '0' + cc;

    /* Get pointers to the ICMP and ICMPREQ headers */
    pIcHdr = (ICMPHDR*) pBuf;
    pReqHdr = (ICMPREQHDR*) pIcHdr->Data;

    /* Fill out echo request */
    pIcHdr->Type = ICMP_ECHO;
    pIcHdr->Code = 0;
    pReqHdr->Id = HNC16(PING_IDENT);
    pReqHdr->Seq = HNC16(seq);
    pIcHdr->Checksum = 0;
}

static int CheckPing(char *pBuf, uint16_t sSeq, int cc, struct sockaddr_in *from)
{
    char IpStr[24], lm[80];
    ICMPHDR *pIcHdr;
    ICMPREQHDR *pReqHdr;
    IPHDR *pIpHdr;
    int IPHdrLen;
    int ICMPLen;
    uint16_t Id, Seq;
    int i, j, size;
    unsigned char *cp, *cpMark;
    uint32_t l;

    /* Get header pointers */
    pIpHdr = (IPHDR*) pBuf;
    IPHdrLen = (pIpHdr->VerLen & 0xF) * 4;
    pIcHdr = (ICMPHDR*) (pBuf + IPHdrLen);
    pReqHdr = (ICMPREQHDR*) pIcHdr->Data;

    /* Get the total length of the ICMP message */
    ICMPLen = (uint32_t) (HNC16(pIpHdr->TotalLen)) - IPHdrLen;

    /* Verify the ICMP type */
    if (pIcHdr->Type != ICMP_ECHOREPLY)
        return (0);

    /* Get the seq and the id */
    Seq = (int) HNC16(pReqHdr->Seq);
    Id = (int) HNC16(pReqHdr->Id);

    /* Get an ASCII string of who replied */
    NtIPN2Str(from->sin_addr.s_addr, IpStr);

    /* If the reply is incorrect, don't continue */
    if (Id != PING_IDENT || Seq != sSeq)
    {
        sprintf(lm, "CheckPing - Non-matching echo reply from %s", IpStr);
        logevent(L_ERROR, lm);
        return (0);
    }

    /* Print out data on correct reply */
    sprintf(lm, "CheckPing - Reply from %s, %d bytes, Seq=%u, TTL = %d", IpStr, ICMPLen - 8, Seq, pIpHdr->Ttl);
    logevent(L_INFO, lm);

    /* Validate the data payload */
    if (ICMPLen != DATALEN)
    {
        sprintf(lm, "CheckPing - Data length error in reply (%d of %d)", cc, DATALEN);
        logevent(L_INFO, lm);
    }
    for (cc = 8; cc < DATALEN; cc++)
        if (*(unsigned char*) ((pBuf + IPHdrLen + cc)) != (unsigned char) ('0' + cc))
        {
            sprintf(lm, "Data verification error in reply (%d 0x%x 0x%x)", cc, *(unsigned char*) ((pBuf + IPHdrLen + cc)), '0' + cc);
            logevent(L_INFO, lm);
            break;
        }

    /* Display any IP Header options */
    cp = (unsigned char*) pBuf + IPHDR_SIZE;

    for (; IPHdrLen > IPHDR_SIZE; --IPHdrLen, ++cp)
    {
        switch (*cp)
        {
        case IPOPT_EOL:
            IPHdrLen = 0;
            break;
        case IPOPT_LSRR:
            case IPOPT_SSRR:
            sprintf(lm, "CheckPing - SSRR/LSRR: ");
            cpMark = cp;
            size = j = *++cp;
            ++cp;
            if (j > IPOPT_MINOFF)
            {
                for (;;)
                {
                    l = (uint32_t) (*(cp)) << 24;
                    l |= (uint32_t) (*(cp + 1)) << 16;
                    l |= (uint32_t) (*(cp + 2)) << 8;
                    l |= (uint32_t) (*(cp + 3));
                    cp += 4;
                    NtIPN2Str(NDK_htonl(l), IpStr);
                    strcat(lm, IpStr);
                    j -= 4;
                    if (j <= IPOPT_MINOFF)
                        break;
                }
            }
            logevent(L_INFO, lm);
            cp = cpMark + size;
            IPHdrLen -= size;
            break;
        case IPOPT_RR:
            cpMark = cp;
            size = j = *++cp;
            i = *++cp; /* pointer */
            ++cp; /* Pad */
            if (i > j)
            {
                i = j;
            }
            i -= IPOPT_MINOFF;
            if (i <= 0)
            {
                goto contRR;
            }
            sprintf(lm, "CheckPing - RR: ");
            for (;;)
            {
                l = (uint32_t) (*(cp)) << 24;
                l |= (uint32_t) (*(cp + 1)) << 16;
                l |= (uint32_t) (*(cp + 2)) << 8;
                l |= (uint32_t) (*(cp + 3));
                cp += 4;
                NtIPN2Str(NDK_htonl(l), IpStr);
                strcat(lm, IpStr);
                i -= 4;
                if (i <= 0)
                {
                    break;
                }
            }
            contRR: cp = cpMark + size;
            IPHdrLen -= size;
            logevent(L_INFO, lm);
            break;
        case IPOPT_NOP:
            sprintf(lm, "CheckPing - NOP");
            logevent(L_INFO, lm);
            break;
        default:
            sprintf(lm, "CheckPing - unknown option %x", *cp);
            logevent(L_INFO, lm);
            break;
        }
    }
    return (1);
}

void set_ndk_prio()
{
    // Set NDK task priorities
    void *hCfg = 0;
    unsigned char p;

    hCfg = CfgGetDefault();

    /* config low priority task level */
    p = TASK_PRIO_LOW;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKPRILOW, CFG_ADDMODE_UNIQUE, sizeof(uint32_t), &p, 0);

    /* config normal priority task level */
    p = TASK_PRIO_MEDIUM;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKPRINORM, CFG_ADDMODE_UNIQUE, sizeof(uint32_t), (unsigned char*) &p, 0);

    /* config high priority task level */
    p = TASK_PRIO_HIGH;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKPRIHIGH, CFG_ADDMODE_UNIQUE, sizeof(uint32_t), (unsigned char*) &p, 0);

    /* config kernel priority task level */
    // p = 20;
    // CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKPRIKERN, CFG_ADDMODE_UNIQUE, sizeof(uint32_t), (unsigned char*) &p, 0);
}

#ifdef _LOGUDP_
void logger_udp(UArg arg0, UArg arg1)
{
    SOCKET sock;
    struct sockaddr_in addr;
    int optval = 1;
    msg_log_t msg;

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);
    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    // Wait for NW ready
    Semaphore_pend(sem_nw_ready, BIOS_WAIT_FOREVER);
    // Release the read semaphore
    Semaphore_post(sem_nw_ready);

    fdOpenSession(TaskSelf());
    sock = NDK_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        fdCloseSession(TaskSelf());
        t_logger_udp = NULL;
        Task_exit();
    }

    // enable broadcast option for the socket (optval = 1)
    NDK_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int));

    // fill the socket address structure with the broadcast address and port
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(get_logudp());

    while (1)
    {
        Mailbox_pend(m_log_udp, &msg, BIOS_WAIT_FOREVER);

        if (strcmp(msg.msg, "port") == 0)
        {
            // Port change
            addr.sin_port = htons(get_logudp());
        }

        if (nw_connected())
        {
            NDK_sendto(sock, msg.msg, strlen(msg.msg), 0, &addr, sizeof(struct sockaddr_in));
            /*if (status < 0)
             {
             // For debug purposes
             status = fdError();
             }*/
        }

        mem_free(msg.msg, msg.size);
    }
}
#endif
