/**
 * @file ftpserver.h
 *
 * @brief Server implementation.
 *
 */
/* Copyright (c) 2017-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __FTPSERVER_H_
#define __FTPSERVER_H_

#include "modules.h"

/*
 * Defines
 */
#define BUFFER_SIZES                    (1024)
#define NAME_AND_PASSWD_LEN             (25)
#define DATA_BUFFER_SIZE                (512)

/*
 * Type definitions
 */
typedef struct io_handler_T
{
    SOCKET socket;
    SOCKET data_socket;
    boolean_t running;
    char *send_buffer;
    char *receive_buffer;
    char *current_working_dir_path;
    char *temp;
    char *DataBuf;
    DIR DirObj;
    FILINFO FileInfo;
    FIL FileObj;
    char user[NAME_AND_PASSWD_LEN];
    char password[NAME_AND_PASSWD_LEN];
    bool authenticated;
} io_handler_t;

typedef struct ftp_cmd_handler_T
{
    char *cmd;
    uint8_t len;
    int8_t (*func)(io_handler_t*);
} ftp_cmd_handler_t;

int8_t ftp_filerout_read(io_handler_t *ioh, char *name);
int8_t ftp_filerout_write(io_handler_t *ioh, char *name);
int8_t ftp_filerout_list(io_handler_t *ioh, bool dir_mode);
int8_t ftp_filerout_changeDir(io_handler_t *ioh, char *path);
int8_t ftp_filerout_write(io_handler_t *ioh, char *name);
int8_t ftp_filerout_delete(io_handler_t *ioh, char *name);
int8_t ftp_filerout_mkdir(io_handler_t *ioh, char *name);
int8_t ftp_filerout_rmdir(io_handler_t *ioh, char *name);
static void get_curdir(char *r, io_handler_t *ioh);
static void set_ftp_connect(bool s);

#endif
