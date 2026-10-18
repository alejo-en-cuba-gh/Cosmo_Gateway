/**
 * @file ftpserver.c
 *
 * @brief FTP Server and client implementation.
 *
 */

/* Include files  */
#include "modules.h"
#include "ftpserver.h"

/*
 * Defines
 */
#define LISTEN_TIMEOUT                        (10000)
#define FTPSERVER_PORT                        (21)

/* Ftpserver task definitions */
#define TASK_FTP_SERVER_STACK_SIZE            (1024)
#define TASK_FTP_SERVER_PRIO                  (5)
#define TASK_FTP_SERVER_NAME                  "FTP Task"

/* Ftpserver client connection task definitions */
#define TASK_FTP_SERVER_CLIENT_STACK_SIZE     (4096)
#define TASK_FTP_SERVER_CLIENT_PRIO           (5)
#define TASK_FTP_SERVER_CLIENT_NAME           "FTP-Client Task"

/*
 * Local functions
 */
static void ftpserver_thread_entry(UArg a0, UArg a1);
static void ftpserver_client_thread_entry(UArg a0, UArg a1);
static int8_t ftpserver_parser(io_handler_t *ioh);
static uint16_t port;
static uint32_t listen_timeout;

extern ftp_cmd_handler_t cmd_handler[];

Semaphore_Handle sem_ftp_connected, sem_ftp_state;
bool ftp_active = false;

/**
 * @brief ftpserver_init
 */
int8_t ftpserver_init(void)
{
    Task_Handle task;
    Task_Params params;
    Error_Block eb;

    Task_Params_init(&params);
    Error_init(&eb);

    Semaphore_Params sp;
    Semaphore_Params_init(&sp);
    sp.mode = Semaphore_Mode_BINARY;
    sp.instance->name = "sem_ftp_connected";
    sem_ftp_connected = Semaphore_create(0, &sp, NULL);

    Semaphore_Params_init(&sp);
    sp.mode = Semaphore_Mode_BINARY;
    sp.instance->name = "sem_ftp_state";
    sem_ftp_state = Semaphore_create(1, &sp, NULL);

    params.priority = TASK_FTP_SERVER_PRIO;
    params.stackSize = TASK_FTP_SERVER_STACK_SIZE;
    params.instance->name = "ftpserver_thread";
    params.arg0 = 0;
    params.arg1 = 0;

    /* Read data from config (or default values) */
    port = FTPSERVER_PORT;
    listen_timeout = LISTEN_TIMEOUT;

    task = Task_create(ftpserver_thread_entry, &params, &eb);
    if (task == NULL)
    {
        //Display_printf(display, 0, 0, "Task '%s', creation failed for ftpserver_thread!, Error %s\n", Error_getMsg(&eb));

        return ERROR_TASK_CREATE;
    }

    return ERROR_SUCCESS;
}

/**
 * @brief     FTP server main thread. Listen on new connections and spawns of client connection tasks.
 *
 * @param a0    Not used
 * @param a1    Not used
 *
 * @return        -
 */
void ftpserver_thread_entry(UArg a0, UArg a1)
{

    Task_Params params;
    Error_Block eb;
    msg_led_t l;

    struct sockaddr_in server_addr, client_addr;
    SOCKET listen_sock, client_sock;
    int len = sizeof(struct sockaddr_in);
    struct timeval timeout = { 10, 0 };

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = NDK_htonl(INADDR_ANY);
    server_addr.sin_port = NDK_htons(port);

    fdOpenSession(TaskSelf());

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        return;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    server_addr.sin_port = NDK_htons(port);

    if (bind(listen_sock, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
    {
        fdClose(listen_sock);
        return;
    }

    if (listen(listen_sock, 5) == SOCKET_ERROR)
    {
        fdClose(listen_sock);
        return;
    }

    /* Set listen timeout to make watchdog work */
    timeout.tv_sec = listen_timeout;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == SOCKET_ERROR)
    {
        fdClose(listen_sock);
        return;
    }

    for (;;)
    {
        Task_Handle task;

        if ((client_sock = accept(listen_sock, (struct sockaddr*) &client_addr, &len)) == INVALID_SOCKET)
        {
            // called when socket closed - TODO improve error handling
            fdClose(listen_sock);
            Task_sleep(100);
            break;
        }

        Task_Params_init(&params);
        Error_init(&eb);

        params.priority = TASK_FTP_SERVER_CLIENT_PRIO;
        params.stackSize = TASK_FTP_SERVER_CLIENT_STACK_SIZE;
        params.instance->name = "ftpserver_client_thread";
        params.arg0 = (UArg) client_sock;
        params.arg1 = 0;

        task = Task_create(ftpserver_client_thread_entry, &params, &eb);
        if (task)
        {
            // Set ftp-connected LED
            l.led = LS_FTP;
            l.state = true;
            if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false)
            {
                err_mailbox_full(__func__, m_led);
            }

            // Set FTP state
            set_ftp_connect(true);

            // Stop waiting for new connection until current session is closed
            Semaphore_pend(sem_ftp_connected, BIOS_WAIT_FOREVER);

            // Unset ftp-connected LED
            l.led = LS_FTP;
            l.state = false;
            if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false)
            {
                err_mailbox_full(__func__, m_led);
            }

            // Set FTP state
            set_ftp_connect(false);
        }
        else
        {
            fdClose(client_sock);
            Task_sleep(100);
        }
    }

    // Close the file session
    fdCloseSession(TaskSelf());

    // End task
    Task_exit();
}

/**
 * @brief     FTP server client connection task. Spawned of from the ftp server task
 *          when a new connection is established
 *
 * @param a0    client socket
 * @param a1    Not used
 *
 */
static void ftpserver_client_thread_entry(UArg a0, UArg a1)
{
    io_handler_t ftpserver;
    SOCKET socket = (SOCKET) a0;
    int32_t opt = 1;

    // Open the file session
    fdOpenSession(TaskSelf());

    memset(&ftpserver, 0, sizeof(ftpserver));

    /* Disables the Nagle algorithm for TCP sockets */
    if (setsockopt(socket, IPPROTO_TCP, NDK_TCP_NODELAY, &opt, sizeof(opt)) != SOCKET_ERROR)
    {
        ftpserver.authenticated = false;
        ftpserver.socket = (void*) socket;
        ftpserver.send_buffer = mem_alloc(BUFFER_SIZES);
        ftpserver.receive_buffer = mem_alloc(BUFFER_SIZES);
        ftpserver.current_working_dir_path = mem_alloc(DATA_BUFFER_SIZE);
        ftpserver.DataBuf = mem_alloc(DATA_BUFFER_SIZE);
        ftpserver.temp = mem_alloc(DATA_BUFFER_SIZE);

        sprintf(ftpserver.current_working_dir_path, "0:");

        if (ftpserver.send_buffer && ftpserver.receive_buffer && ftpserver.current_working_dir_path && ftpserver.DataBuf && ftpserver.temp)
        {
            ftpserver_parser(&ftpserver);

            if (ftpserver.send_buffer)
                mem_free(ftpserver.send_buffer, BUFFER_SIZES);
            if (ftpserver.receive_buffer)
                mem_free(ftpserver.receive_buffer, BUFFER_SIZES);
            if (ftpserver.current_working_dir_path)
                mem_free(ftpserver.current_working_dir_path, DATA_BUFFER_SIZE);
            if (ftpserver.temp)
                mem_free(ftpserver.temp, DATA_BUFFER_SIZE);
            if (ftpserver.DataBuf)
                mem_free(ftpserver.DataBuf, DATA_BUFFER_SIZE);
        }
    }

    // New clients can connect
    Semaphore_post(sem_ftp_connected);

    /* Close the client socket */
    fdClose(socket);

    /* Close the file session */
    fdCloseSession(TaskSelf());

    // End task
    Task_exit();
}

/**
 * @brief FTP Server comand parser
 *
 * Look up the received ftp command name string in cmd_handler, and call the associated function.
 *
 * @param ioh    Pointer to io-structure that contains buffers, socket etc, see
 *               io_handler_t structure definition.
 *
 * @return ERROR_SUCCESS
 */
static int8_t ftpserver_parser(io_handler_t *ioh)
{
    int32_t n, bytes;
    ftp_cmd_handler_t *p;
    char pn[10];

    /* Respond with welcome message */
    if (oem == M_SCANVEST)
    {
        sprintf(pn, "ScanWay");
    }
    else
    {
        sprintf(pn, "SMaWay");

    }

    sprintf(ioh->send_buffer, "220 Welcome to the %s FTP server.\r\nPlease use this service ONLY with the Windows command line FTP client.\r\n", pn);

    bytes = send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);

    ioh->running = TRUE;
    while (ioh->running)
    {
        n = 0;
        while (1)
        {
            if (n > (BUFFER_SIZES - 2))
                break;

            bytes = recv(ioh->socket, &ioh->receive_buffer[n], 1, 0); // receive byte by byte...

            if (bytes <= 0)
                break;
            if (ioh->receive_buffer[n] == '\n')
            {
                ioh->receive_buffer[n] = '\0';
                break;
            }
            if (ioh->receive_buffer[n] != '\r')
                n++; /*ignore CRs*/
        }
        if (bytes <= 0)
            break;

        /* Process requested command */
        for (p = &cmd_handler[0]; p->func != NULL; p++)
        {
            if (strncmp(ioh->receive_buffer, p->cmd, p->len) == 0)
            {
                if (p->func)
                    p->func(ioh);
                break;
            }
        }

        /* Command NOT found! */
        if (p->func == NULL)
        {
            sprintf(ioh->send_buffer, "202 Command not implemented (%s)\r\n", ioh->receive_buffer);
            send(ioh->socket, ioh->send_buffer, strlen(ioh->send_buffer), 0);
        }
    }

    return ERROR_SUCCESS;
}

static void set_ftp_connect(bool s)
{
    Semaphore_pend(sem_ftp_state, BIOS_WAIT_FOREVER);
    ftp_active = s;
    Semaphore_post(sem_ftp_state);
}

bool get_ftp_connect()
{
    bool s;
    Semaphore_pend(sem_ftp_state, BIOS_WAIT_FOREVER);
    s = ftp_active;
    Semaphore_post(sem_ftp_state);
    return s;
}
