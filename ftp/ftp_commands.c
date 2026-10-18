/**
 * @file ftp_commands.c
 *
 * @brief FTP Command handlers.
 *
 */

/*
 * Include files
 */
#include "modules.h"
#include "ftpserver.h"

/*
 * Local functions
 */
static int8_t func_opts(io_handler_t *ioh);
static int8_t func_user(io_handler_t *ioh);
static int8_t func_pass(io_handler_t *ioh);
static int8_t func_stor(io_handler_t *ioh);
static int8_t func_dele(io_handler_t *ioh);
static int8_t func_xrmd(io_handler_t *ioh);
static int8_t func_cwd(io_handler_t *ioh);
static int8_t func_xpwd(io_handler_t *ioh);
static int8_t func_retr(io_handler_t *ioh);
static int8_t func_list(io_handler_t *ioh);
static int8_t func_port(io_handler_t *ioh);
static int8_t func_nlst(io_handler_t *ioh);
static int8_t func_quit(io_handler_t *ioh);
static int8_t func_feat(io_handler_t *ioh);
static int8_t func_mkdir(io_handler_t *ioh);
static int8_t func_rmdir(io_handler_t *ioh);
static int8_t func_reboot(io_handler_t *ioh);

/*
 * Local variables
 */
ftp_cmd_handler_t cmd_handler[] = { { "OPTS", 4, func_opts }, { "USER", 4, func_user }, { "PASS", 4, func_pass }, { "STOR", 4, func_stor }, { "DELE", 4,
                                                                                                                                              func_dele },
                                    { "XRMD", 4, func_xrmd }, { "CWD", 3, func_cwd }, { "XPWD", 4, func_xpwd }, { "RETR", 4, func_retr },
                                    { "FEAT", 4, func_feat }, { "LIST", 4, func_list }, { "PORT", 4, func_port }, { "NLST", 4, func_nlst }, { "QUIT", 4,
                                                                                                                                              func_quit },
                                    { "PWD", 3, func_xpwd }, { "MKD", 3, func_mkdir }, { "XMKD", 4, func_mkdir }, { "RMD", 3, func_rmdir }, { "REBOOT", 6, func_reboot }, { NULL, 0, NULL } };

/**
 * @brief  Return second argument from input string.
 */
static void argument_collector(char *_buffer, char *_nameBuffer, uint16_t maxsize)
{
    char sep[2] = " ";
    char *word;
    uint8_t wcount = 0;

    // Check if the input parameters are valid
    if (_buffer == NULL || _nameBuffer == NULL)
    {
        return;
    }

    // Duplicate the _buffer string using strdup
    char *bf = strdup(_buffer);
    if (bf == NULL)
    {
        return;
    }

    for (word = strtok(bf, sep); word; word = strtok(NULL, sep))
    {
        wcount++;
        if (wcount == 2)
        {
            if (strlen(word) >= maxsize)
            {
                word[maxsize - 1] = 0;
            }

            // Copy the name to _nameBuffer
            strcpy(_nameBuffer, word);

            break;
        }
    }

    // Free the bf memory
    free(bf);
}

/**
 * check_authentificated
 */
static boolean_t check_authentificated(io_handler_t *ioh)
{
    if (!ioh->authenticated)
    {
        sprintf(ioh->send_buffer, "530 Please log in with USER and PASS first. \r\n");
        send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);
        return FALSE;
    }

    return TRUE;
}

/**
 * func_opts
 */
static int8_t func_opts(io_handler_t *ioh)
{
    argument_collector(ioh->receive_buffer, ioh->temp, DATA_BUFFER_SIZE);

    sprintf(ioh->send_buffer, "530 Please log in with USER and PASS first.\r\n");
    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_user
 */
static int8_t func_user(io_handler_t *ioh)
{
    argument_collector(ioh->receive_buffer, ioh->user, NAME_AND_PASSWD_LEN);

    sprintf(ioh->send_buffer, "331 Password required \r\n");
    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_pass
 */
static int8_t func_pass(io_handler_t *ioh)
{
    int32_t bytes, pw = 0;
    dbt_user *user;

    argument_collector(ioh->receive_buffer, ioh->password, NAME_AND_PASSWD_LEN);

    user = find_element(&config_db.db_user, &config_db.db_user->name, false, USER, STRING, 0, 0, ioh->user,0);
    if (user)
    {
        pw = user->pw;
        mem_free(user, sizeof(dbt_user));
    }

    if (pw && check_pw(ioh->password, user->pw))
    //if (true) // allow any user for debugging
    {
        sprintf(ioh->send_buffer, "230 Public login successful \r\n");
        bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);
        if (bytes < 0)
            return -1;
        ioh->authenticated = true;
    }
    else
    {
        sprintf(ioh->send_buffer, "530 Invalid username or password \r\n");
        bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);
        if (bytes < 0)
            return -1;
        ioh->authenticated = false;
    }

    if (bytes > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_dele
 */
static int8_t func_dele(io_handler_t *ioh)
{
    char path[64];

    if (!check_authentificated(ioh))
        return -1;
    argument_collector(ioh->receive_buffer, path, DATA_BUFFER_SIZE);

    if (ftp_filerout_delete(ioh, path))
    {
        sprintf(ioh->send_buffer, "550 File not found\r\n");
    }
    else
    {
        sprintf(ioh->send_buffer, "200 Ok\r\n");
    }

    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_mkdir
 */
static int8_t func_mkdir(io_handler_t *ioh)
{
    char path[64];

    if (!check_authentificated(ioh))
        return -1;
    argument_collector(ioh->receive_buffer, path, DATA_BUFFER_SIZE);

    if (ftp_filerout_mkdir(ioh, path))
    {
        sprintf(ioh->send_buffer, "550 Error\r\n");
    }
    else
    {
        sprintf(ioh->send_buffer, "257 \"%s\" created\r\n", path);
    }

    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_rmdir
 */
static int8_t func_rmdir(io_handler_t *ioh)
{
    char path[64];

    if (!check_authentificated(ioh))
        return -1;
    argument_collector(ioh->receive_buffer, path, DATA_BUFFER_SIZE);

    if (ftp_filerout_rmdir(ioh, path))
    {
        sprintf(ioh->send_buffer, "550 Error\r\n");
    }
    else
    {
        sprintf(ioh->send_buffer, "250 \"%s\" removed\r\n", path);
    }

    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_type
 */
static int8_t func_feat(io_handler_t *ioh)
{
    if (!check_authentificated(ioh))
        return -1;

    sprintf(ioh->send_buffer, "OPTS\r\nUSER\r\nPASS\r\nSTOR\r\nDELE\r\nXRMD\r\nCWD\r\nXPWD\r\nRETR\r\nLIST\r\nPORT\r\nNLST\r\nQUIT\r\nPWD\r\nMKD\r\n");

    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_xrmd
 */
static int8_t func_xrmd(io_handler_t *ioh)
{
    char path[64];

    if (!check_authentificated(ioh))
        return -1;
    argument_collector(ioh->receive_buffer, path, DATA_BUFFER_SIZE);

    if (0)
    {
        sprintf(ioh->send_buffer, "550 Directory not found\r\n");
    }
    else
    {
        sprintf(ioh->send_buffer, "200 Ok\r\n");
    }

    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_stor
 */
static int8_t func_stor(io_handler_t *ioh)
{
    char filename[64];

    if (!check_authentificated(ioh))
        return -1;

    argument_collector(ioh->receive_buffer, filename, 64);

    sprintf(ioh->send_buffer, "150 OK\r\n");
    send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);

    if (ftp_filerout_write(ioh, filename) == 0)
    {
        // If config file has been transmitted, suggest reboot
        if (strstr(filename, "sw.cfg") && strstr(filename, "sw.cfg" == NULL))
        {
            sprintf(ioh->send_buffer, "226 File transfer completed\r\nNew configuration requires reboot.\r\nPlease send \"quote REBOOT\" at end of session.\r\n");
        }
        else
        {
            sprintf(ioh->send_buffer, "226 File transfer completed\r\n");
        }
    }
    else
    {
        sprintf(ioh->send_buffer, "450 Requested file action not taken\r\n");
    }

    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief Implement FTP command CWD
 *
 * Change working directory.
 */
static int8_t func_cwd(io_handler_t *ioh)
{
    char dirName[DATA_BUFFER_SIZE];

    if (!check_authentificated(ioh))
        return -1;
    argument_collector(ioh->receive_buffer, dirName, DATA_BUFFER_SIZE);

    if (ftp_filerout_changeDir(ioh, dirName) == CSL_SOK)
    {
        get_curdir(dirName, ioh);
        sprintf(ioh->send_buffer, "250 CWD successful. %s is current directory\r\n", dirName);
    }
    else
    {
        sprintf(ioh->send_buffer, "550 CWD to '%s' failed.\r\n", dirName);
    }

    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_xpwd
 */
static int8_t func_xpwd(io_handler_t *ioh)
{
    char *dir, d[3];
    if (!check_authentificated(ioh))
        return -1;

    if (ioh->current_working_dir_path[2] == 0)
    {
        sprintf(d, "\"/");
        dir = NULL;
    }
    else if (ioh->current_working_dir_path[0] == '0')
    {
        sprintf(d, "\"");
        dir = &ioh->current_working_dir_path[2];
    }
    else
    {
        sprintf(d, "\"/");
        dir = ioh->current_working_dir_path;
    }

    sprintf(ioh->send_buffer, "257 %s%s\" is current directory\r\n", d, dir ? dir : "");

    if (send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0) > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_retr
 */
static int8_t func_retr(io_handler_t *ioh)
{
    int32_t bytes;
    char filename[64];
    db_event_t e;

    if (!check_authentificated(ioh))
    {
        return -1;
    }

    argument_collector(ioh->receive_buffer, filename, 64);

    // If config file is requested, update first
    if (strstr(filename, "sw.cfg") && strstr(filename, "sw.cfg.bu") == NULL)
    {
        // Update configuration file
        e = DBE_EXPORT;
        Mailbox_post(m_db, &e, BIOS_WAIT_FOREVER);

        // Wait for configuration update ready
        Semaphore_pend(sem_db_updated, BIOS_WAIT_FOREVER);

        sprintf(ioh->send_buffer, "150 Configuration updated. Transferring... \r\n");
    }
    else
    {
        sprintf(ioh->send_buffer, "150 Transferring... \r\n");
    }

    bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);

    if (ftp_filerout_read(ioh, filename) == CSL_SOK)
    {
        sprintf(ioh->send_buffer, "226 File transfer completed... \r\n");
    }
    else
    {
        sprintf(ioh->send_buffer, "450 Requested file action not taken\r\n");
    }
    bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);

    fdClose(ioh->data_socket);

    if (bytes > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_list
 */
static int8_t func_list(io_handler_t *ioh)
{
    int32_t res;
    if (!check_authentificated(ioh))
        return -1;

    sprintf(ioh->send_buffer, "150 Transferring... \r\n");
    send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);

    res = ftp_filerout_list(ioh, true);

    sprintf(ioh->send_buffer, "226 File transfer completed... \r\n");
    send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);

    fdClose(ioh->data_socket);

    if (res == CSL_SOK)
    {
        return ERROR_SUCCESS;
    }
    return -1;
}

/**
 * func_port
 */
static int8_t func_port(io_handler_t *ioh)
{
    int act_port[2];
    int act_ip[4], port_dec;
    char ip_decimal[40];
    struct sockaddr_in remoteaddr_act;
    int32_t bytes;

    if (!check_authentificated(ioh))
        return -1;

    if (ioh->data_socket)
        fdClose(ioh->data_socket);
    ioh->data_socket = socket(AF_INET, SOCK_STREAMNC, IPPROTO_TCP);

    /* local variables */
    sscanf(ioh->receive_buffer, "PORT %d,%d,%d,%d,%d,%d", &act_ip[0], &act_ip[1], &act_ip[2], &act_ip[3], &act_port[0], &act_port[1]);
    sprintf(ip_decimal, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2], act_ip[3]);
    port_dec = (act_port[0] * 256) + act_port[1];

    /* Prepare address for connect */
    memset(&remoteaddr_act, 0, sizeof(struct sockaddr_in));
    remoteaddr_act.sin_family = AF_INET;
    remoteaddr_act.sin_addr.s_addr = inet_addr(ip_decimal);
    remoteaddr_act.sin_port = NDK_htons(port_dec);

    if (connect(ioh->data_socket, (struct sockaddr*) &remoteaddr_act, (int32_t) sizeof(struct sockaddr_in)) == SOCKET_ERROR)
    {
        sprintf(ioh->send_buffer, "425 Something is wrong, can't start the active connection... \r\n");
        bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);
        fdClose(ioh->data_socket);
    }
    else
    {
        sprintf(ioh->send_buffer, "200 Ok\r\n");
        bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);
    }

    if (bytes > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_nlst
 */
static int8_t func_nlst(io_handler_t *ioh)
{
    int8_t res;

    if (!check_authentificated(ioh))
        return -1;

    sprintf(ioh->send_buffer, "150 Transferring... \r\n");
    send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);

    res = ftp_filerout_list(ioh, false);

    sprintf(ioh->send_buffer, "226 File transfer completed... \r\n");
    send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);

    fdClose(ioh->data_socket);

    if (res == CSL_SOK)
    {
        return ERROR_SUCCESS;
    }
    return -1;
}

/**
 * func_quit
 */
static int8_t func_quit(io_handler_t *ioh)
{
    int32_t bytes;
    sprintf(ioh->send_buffer, "221 Connection closed by the FTP client\r\n");
    bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);
    ioh->running = FALSE;

    if (bytes > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

/**
 * func_reboot, custom function: "quote REBOOT"
 */
static int8_t func_reboot(io_handler_t *ioh)
{
    int32_t bytes;
    sprintf(ioh->send_buffer, "System will reboot in 5 seconds.\r\n");
    bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);
    ioh->running = FALSE;

    // Request system reboot
    Semaphore_post(sem_reboot);

    if (bytes > 0)
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return -1;
    }
}

static void get_curdir(char *r, io_handler_t *ioh)
{
    char *dir, d[3];

    if (ioh->current_working_dir_path[2] == 0)
    {
        sprintf(d, "\"/");
        dir = NULL;
    }
    else if (ioh->current_working_dir_path[0] == '0')
    {
        sprintf(d, "\"");
        dir = &ioh->current_working_dir_path[2];
    }
    else
    {
        sprintf(d, "\"/");
        dir = ioh->current_working_dir_path;
    }

    sprintf(r, "%s%s\"", d, dir ? dir : "");
}
