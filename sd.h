/*
 * sd.h
 *
 *  Created on: 15 dic 2023
 */

#ifndef SD_H_
#define SD_H_

int32_t fatfs_getFatTime(void);

/* Buffer size used for the file copy process */
#ifndef CPY_BUFF_SIZE
#define CPY_BUFF_SIZE       2048
#endif

/* String conversion macro */
#define STR_(n)             #n
#define STR(n)              STR_(n)

/* Drive number used for FatFs */
#define DRIVE_NUM           0

/* File name prefix for this filesystem for use with TI C RTS */
char fatfsPrefix[] = "fat";

static bool check_configdir();
static int oct2dec(char *oct, int size);

struct tar_header
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

#endif /* SD_H_ */
