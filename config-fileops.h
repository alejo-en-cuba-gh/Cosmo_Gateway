/*
 * config-fileops.h
 *
 *  Created on: 18 dic 2023
 */

#ifndef CONFIG_FILEOPS_H_
#define CONFIG_FILEOPS_H_

#define f_getc(fp) (f_read(fp, &c, 1, &f_read_bytesread) == FR_OK ? c : EOF)
#define f_gets(str, num, stream) (f_read(stream, str, num, &f_read_bytesread) == FR_OK ? str : NULL)

extern Semaphore_Handle sem_db_rw;
extern const uint16_t vdb;

static bool check_endline(char **pos, uint16_t c);

#endif /* CONFIG_FILEOPS_H_ */
