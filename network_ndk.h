/*
 * network_ndk.h
 *
 *  Created on: 11 nov 2023
 */

#ifndef NETWORK_NDK_H_
#define NETWORK_NDK_H_

#define htons(a) NDK_htons(a)
#define htonl(a) NDK_htonl(a)
#define ntohl(a) NDK_ntohl(a)
#define ntohs(a) NDK_ntohs(a)

#endif /* NETWORK_NDK_H_ */
