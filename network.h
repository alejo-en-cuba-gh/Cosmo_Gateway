/*
 * network.h
 *
 *  Created on: 2 nov 2023
 */

#ifndef NETWORK_H_
#define NETWORK_H_

struct icmp_hdr
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} icmp_hdr;

uint8_t ip_config();
void remove_ip();
void startDHCP();
int ping(char *ip);
uint8_t http_get(char *server, char *request, char **response, uint16_t maxlength);
static int CheckPing( char *pBuf,uint16_t sSeq,int cc,struct sockaddr_in *from );
static void BuildPing( char *pBuf, uint16_t seq );
void on_ip_change(uint32_t IPAddr, unsigned int IfIdx, unsigned int fAdd);

#define USER_AGENT     "SMaBiT SMaWay RT"

// Defines for ping
#define MAXPACKET       1000
#define PING_IDENT      0x1234
#define NROUTES         9
#define DATALEN         100

#endif /* NETWORK_H_ */
