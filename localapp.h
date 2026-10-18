/*
 * localapp.h
 *
 *  Created on: 28 feb 2024
 */

#ifndef LOCALAPP_H_
#define LOCALAPP_H_

#define APPSVR_PORT 2777
#define APPSVR_LTO 60    // listen timeout in s
#define APPSVR_MAXCONN 3 // maximum simultaneous app connections
#define TMSG_MAXSIZE 500 // maximum packet size
#define FINDER_RCV 26765
#define FINDER_SND 26766
#define MSG_CONFIRM

void appsvr_connection(UArg a0, UArg a1);
uint8_t get_appcount();
void add_appcon(SOCKET s);
void remove_appcon(SOCKET s);

#endif /* LOCALAPP_H_ */
