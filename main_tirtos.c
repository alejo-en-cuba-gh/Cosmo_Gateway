/*
 * Copyright (c) 2016-2020, Texas Instruments Incorporated
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
 */

/*
 *  ======== main_tirtos.c ========
 */

/* RTOS header files */
#include "common.h"
#include "main_tirtos.h"

extern void ti_ndk_config_Global_startupFxn();

/* Tasks */
Task_Handle t_main = NULL, t_config_db = NULL, t_net = NULL, t_net_update = NULL, t_logger = NULL,
        t_net_upstate = NULL, t_tick = NULL, t_led = NULL, t_reboot = NULL,
        t_finder = NULL, t_mqtt_client = NULL, t_appsvr_connection_mqtt = NULL, t_rpc_reciever = NULL, t_rpc_send = NULL,
        t_create_com_task = NULL, t_flash = NULL, t_Wifi_command = NULL, t_wifi_connect = NULL;
Mailbox_Handle m_main, m_evlog, m_led, m_log_udp, m_frame_proc_mqtt;
Semaphore_Handle sem_reboot, sem_systemprintf;

//CAN_Handle can;
UART_Handle uart;
Display_Handle display = NULL;
const oem_t oem = M_SMABIT;
spiffs fs_spiffs;
const mu_t mu = MU_BASE;

/*
 *  ======== main ========
 */

int main(void)
{
    Task_Params taskParams;
    Mailbox_Params mbxParams;
    Semaphore_Params params;
    Timer_Handle timer0;
    Timer_Params timerParams;

    Board_init();
    GPIO_unlock(GPIOMSP432E4_PD7);
    GPIO_init();
    UART_init();
    Button_init();
    LED_init();
    CAN_init();

//    SDFatFS_init();
    NVS_init();

    ledbutton();

    // Start NDK stack
    set_ndk_prio();
    ti_ndk_config_Global_startupFxn();

    // Create DB ready semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_db_ready";
    sem_db_ready = Semaphore_create(0, &params, NULL);

    // Create DB update needed semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_db_updated";
    sem_db_updated = Semaphore_create(0, &params, NULL);

    // Create network ready semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_nw_ready";
    sem_nw_ready = Semaphore_create(0, &params, NULL);

    // Create network ready semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_online";
    sem_online = Semaphore_create(0, &params, NULL);

    // Create network connected semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_nw_connect";
    sem_nw_connect = Semaphore_create(1, &params, NULL);

    // Create IP update semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_ip_update";
    sem_ip_update = Semaphore_create(0, &params, NULL);

    // Create localtime semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_ltime";
    sem_ltime = Semaphore_create(1, &params, NULL);

    // Create logger ready semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_logready";
    sem_logready = Semaphore_create(1, &params, NULL);
    /*
     // Create sd ready semaphore
     Semaphore_Params_init(&params);
     params.mode = Semaphore_Mode_BINARY;
     params.instance->name = "sem_sd";
     sem_sd = Semaphore_create(0, &params, NULL);

     // Create sd GPIO semaphore
     Semaphore_Params_init(&params);
     params.mode = Semaphore_Mode_BINARY;
     params.instance->name = "sem_sd_gpio";
     sem_sd_gpio = Semaphore_create(0, &params, NULL);
     */

    // Create timer tick semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_ttick";
    sem_ttick = Semaphore_create(0, &params, NULL);

    // Create reboot semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_reboot";
    sem_reboot = Semaphore_create(0, &params, NULL);

    // Create MQTT disconnect semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_mq_disc";
    sem_mq_disc = Semaphore_create(0, &params, NULL);

    // Create SNTP semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_sntp";
    sem_sntp = Semaphore_create(0, &params, NULL);

    // Create MQTT connected semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_mq_connected";
    sem_mq_connected = Semaphore_create(1, &params, NULL);

    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_uart";
    sem_uart = Semaphore_create(0, &params, NULL);

    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_rpc";
    sem_rpc = Semaphore_create(1, &params, NULL);

    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_srsp";
    sem_srsp = Semaphore_create(0, &params, NULL);

    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_mtprocess";
    sem_mtprocess = Semaphore_create(1, &params, NULL);

    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_mtaf_response";
    sem_mtaf_response = Semaphore_create(0, &params, NULL);


    // Create MQTT_INIT semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_mqtt_init";
    sem_mqtt_init = Semaphore_create(0, &params, NULL);
/*
    // Create local apps connected semaphore
    Semaphore_Params sp;
    Semaphore_Params_init(&sp);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_appcon";
    sem_appcon = Semaphore_create(1, &params, NULL);
*/
    // Create spiffs mount semaphore
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_spiffs_mount";
    sem_spiffs_mount = Semaphore_create(0, &params, NULL);

    // Create wifi ready
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_wifi_ready";
    sem_wifi_ready = Semaphore_create(0, &params, NULL);

    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_role_wifi";
    sem_role_wifi = Semaphore_create(1, &params, NULL);

    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_sta_ap_ready";
    sem_sta_ap_ready = Semaphore_create(1, &params, NULL);

    /*
     if (oem == M_SCANVEST)
     {
     // Create the sll_ready sempaphore
     Semaphore_Params_init(&params);
     params.mode = Semaphore_Mode_BINARY;
     params.instance->name = "sem_sll_ready";
     sem_sll_ready = Semaphore_create(1, &params, NULL);
     }
     */
    // Create the event logger mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_evlog";
    m_evlog = Mailbox_create(sizeof(msg_log_t), MBOXSIZE * 2, &mbxParams, NULL);

    #ifdef _LOGUDP_
    // Create the event UDP logger mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_log_udp";
    m_log_udp = Mailbox_create(sizeof(msg_log_t), MBOXSIZE * 2, &mbxParams,
    NULL);
    #endif

/*
    // Create the events-in mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_event_in";
    m_event_in = Mailbox_create(sizeof(msg_event_t), MBOXSIZE, &mbxParams,
    NULL);

    // Create the events-out mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_event_out";
    m_event_out = Mailbox_create(sizeof(msg_event_t), MBOXSIZE, &mbxParams,
    NULL);

    // Create can_out mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_can_out";
    m_can_out = Mailbox_create(sizeof(CAN_Frame), MBOXSIZE, &mbxParams, NULL);
*/
    // Create gpio mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_gpio";
    m_gpio = Mailbox_create(sizeof(uint8_t), MBOXSIZE, &mbxParams, NULL);

    // Create led mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_led";
    m_led = Mailbox_create(sizeof(msg_led_t), MBOXSIZE, &mbxParams, NULL);

    // Create database mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_db";
    m_db = Mailbox_create(sizeof(db_event_t), MBOXSIZE / 2, &mbxParams, NULL);

    // Create MQTT send mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_mqtt_send";
    m_mqtt_send = Mailbox_create(sizeof(msg_mqtt_t), MBOXSIZE * 2, &mbxParams, NULL);

    // Create SMaWay mqtt process send mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_frame_proc_mqtt";
    m_frame_proc_mqtt = Mailbox_create(sizeof(msg_mqtt_t), MBOXSIZE, &mbxParams, NULL);

    // Create MQTT receive mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_mqtt_recv";
    m_mqtt_recv = Mailbox_create(sizeof(msg_mqtt_t), MBOXSIZE, &mbxParams, NULL);

    // Create Remote Procedure Call send mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_rpc_send";
    m_rpc_send = Mailbox_create(sizeof(msg_rpc_t), MBOXSIZE, &mbxParams, NULL);

    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_mtSys";
    m_mtSys = Mailbox_create(sizeof(msg_mqtt_t), MBOXSIZE, &mbxParams, NULL);

    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_rpc_srsp";
    m_rpc_srsp = Mailbox_create(sizeof(msg_rpc_t), MBOXSIZE, &mbxParams, NULL);

    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_rpc_arsp";
    m_rpc_arsp = Mailbox_create(sizeof(msg_rpc_t), MBOXSIZE, &mbxParams, NULL);

    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_add_device";
    m_add_device = Mailbox_create(sizeof(msg_rpc_t), MBOXSIZE, &mbxParams, NULL);

    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_wifi_send";
    m_wifi_send = Mailbox_create(sizeof(msg_rpc_t), MBOXSIZE, &mbxParams, NULL);

    /*
     if (oem == M_SCANVEST)
     {
     // Create the SLL mailbox
     Mailbox_Params_init(&mbxParams);
     mbxParams.instance->name = "m_sll_in";
     m_sll_in = Mailbox_create(sizeof(msg_sll_t), MBOXSIZE, &mbxParams,
     NULL);
     }
     */

    /*
     * Open CAN instance.
     * CAN_Params filterID and filterMask determine what messages to receive.
     */
/*
    CAN_Params canParams;
    CAN_Params_init(&canParams);
    can = CAN_open(CONFIG_CAN0, &canParams);
*/
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readEcho = UART_ECHO_OFF;
    uart = UART_open(ZNP_UART, &uartParams);

    // Create the event log task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_LOW;
    taskParams.instance->name = "t_logger";
    t_logger = Task_create(logger, &taskParams, NULL);

    // Create the network task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_HUNGRY;
    taskParams.priority = TASK_PRIO_LOW;
    taskParams.instance->name = "t_net";
    t_net = Task_create(net_init, &taskParams, NULL);

    // Create the network update task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_HUNGRY;
    taskParams.priority = TASK_PRIO_LOW;
    taskParams.instance->name = "t_net_update";
    t_net_update = Task_create(net_update, &taskParams, NULL);

    // Create the network upstate task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_LOW;
    taskParams.instance->name = "t_net_upstate";
    t_net_upstate = Task_create(net_upstate, &taskParams, NULL);

#if defined(_USELOGS_) && defined(_LOGUDP_)
    // Create the UDP log task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_LIGHT;
    taskParams.priority = TASK_PRIO_LOW;
    taskParams.instance->name = "t_logger_udp";
    t_logger_udp = Task_create(logger_udp, &taskParams, NULL);
#endif
    // Create the database task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_HUNGRY;
    taskParams.priority = TASK_PRIO_HIGH;
    taskParams.instance->name = "t_config_db";
    t_config_db = Task_create(db_init, &taskParams, NULL);
/*
   // Create the event_in task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_HIGH;
    taskParams.instance->name = "t_event_in";
    t_event_in = Task_create(event_in, &taskParams, NULL);

    // Create the event_out task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_HIGH;
    taskParams.instance->name = "t_event_out";
    t_event_out = Task_create(event_out, &taskParams, NULL);
    */
    /*
     // Create the SD init task
     Task_Params_init(&taskParams);
     taskParams.stackSize = TASK_LIGHT;
     taskParams.priority = TASK_PRIO_MEDIUM;
     taskParams.instance->name = "t_sd";
     t_sd = Task_create(init_sd, &taskParams, NULL);
     */
    // Create the timer tick task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_MEDIUM;
    taskParams.instance->name = "t_tick";
    t_tick = Task_create(timer_tick, &taskParams, NULL);

    // Create the status led task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_LIGHT;
    taskParams.priority = TASK_PRIO_LOW;
    taskParams.instance->name = "t_led";
    t_led = Task_create(ledstate, &taskParams, NULL);

    // Create the reboot task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_LIGHT;
    taskParams.priority = TASK_PRIO_LOW;
    taskParams.instance->name = "t_reboot";
    t_reboot = Task_create(reboot_system, &taskParams, NULL);

    // Create MQTT server task
    Task_Params_init(&taskParams);
    taskParams.stackSize = 1.5*TASK_HUNGRY;
    taskParams.priority = TASK_PRIO_MEDIUM;
    taskParams.instance->name = "t_mqtt_client";
    t_mqtt_client = Task_create(mqtt_client, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = 1.5*TASK_NORMAL;
    taskParams.priority = TASK_PRIO_MEDIUM;
    taskParams.instance->name = "t_appsvr_connection_mqtt";
    t_appsvr_connection_mqtt = Task_create(appsvr_connection_mqtt, &taskParams, NULL);

    // Create flash task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_LIGHT;
    taskParams.priority = TASK_PRIO_MEDIUM;
    taskParams.instance->name = "t_flash";
    t_flash = Task_create(init_spiffs_external, &taskParams, NULL);

    // Create wifi connect task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_MEDIUM;
    taskParams.instance->name = "t_wifi_connect";
    t_wifi_connect = Task_create(wifi_connect, &taskParams, NULL);

    /*
     if (oem == M_SCANVEST)
     {
     // Create SLL task
     Task_Params_init(&taskParams);
     taskParams.stackSize = TASK_NORMAL;
     taskParams.priority = TASK_PRIO_HIGH;
     taskParams.instance->name = "t_sll";
     t_sll = Task_create(sll, &taskParams, NULL);
     }
     */
    // Create timer for event handling
    Timer_init();
    Timer_Params_init(&timerParams);
    timerParams.period = 30000000;
    timerParams.periodUnits = Timer_PERIOD_US;
    timerParams.timerMode = Timer_CONTINUOUS_CALLBACK;
    timerParams.timerCallback = (Timer_CallBackFxn) timer30;

    timer0 = Timer_open(CONFIG_TIMER_0, &timerParams);
    Timer_start(timer0);

    struct timespec ts;
    ts.tv_sec = 1672531200;
    ts.tv_nsec = 0;
    clock_settime(CLOCK_REALTIME, &ts);

    BIOS_start();

    return (0);
}

void delete_com_task()
{
    //Task_delete(&t_rpc_send);
    //Task_delete(&t_rpc_reciever);
    Task_delete(&t_Wifi_command);
}

void create_com_task()
{
    Task_Params taskParams;

    /*
     if (!t_finder)
     {
     // Create finderr task
     Task_Params_init(&taskParams);
     taskParams.stackSize = TASK_NORMAL;
     taskParams.priority = TASK_PRIO_LOW;
     taskParams.instance->name = "t_finder";
     t_finder = Task_create(finder_service, &taskParams, NULL);
     }
     */
    if (!t_rpc_send)
    {
        // Create the rpc_send task
        Task_Params_init(&taskParams);
        taskParams.stackSize = TASK_HUNGRY;
        taskParams.priority = TASK_PRIO_MEDIUM;
        taskParams.instance->name = "t_rpc_send";
        t_rpc_send = Task_create(rpc_send, &taskParams, NULL);
    }
    if (!t_rpc_reciever)
    {
        // Create the rpc_recv task
        Task_Params_init(&taskParams);
        taskParams.stackSize = TASK_NORMAL;
        taskParams.priority = TASK_PRIO_HIGH;
        taskParams.instance->name = "t_rpc_reciever";
        t_rpc_reciever = Task_create(rpc_reciever, &taskParams, NULL);
    }
    /*
    if (!t_can_recv)
    {
        // Create the CAN receiver task
        Task_Params_init(&taskParams);
        taskParams.stackSize = TASK_NORMAL;
        taskParams.priority = TASK_PRIO_HIGH;
        taskParams.instance->name = "t_can_recv";
        t_can_recv = Task_create(can_receiver, &taskParams, NULL);
    }
    if (!t_can_send)
    {
        // Create the CAN sender task
        Task_Params_init(&taskParams);
        taskParams.stackSize = TASK_NORMAL;
        taskParams.priority = TASK_PRIO_MEDIUM;
        taskParams.instance->name = "t_can_send";
        t_can_send = Task_create(can_sender, &taskParams, NULL);
    }*/
    // Create wifi_command task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_HUNGRY;
    taskParams.priority = TASK_PRIO_MEDIUM;
    taskParams.instance->name = "t_Wifi_command";
    t_Wifi_command = Task_create(Wifi_command, &taskParams, NULL);

}

static void GPIO_unlock(GPIO_PinConfig pinConfig)
{
    // Required to enable pin 128 (PD7)
    uint_fast8_t pinMask = pinConfig & 0xFF;
    uint8_t port = (pinConfig >> 8) & 0xFF;
    Power_setDependency(getPowerResource(port));
    uint32_t portBase = GPIOMSP432E4_getGpioBaseAddr(port);
    // write magic value to lock register
    HWREG(portBase + GPIO_O_LOCK) = 0x4C4F434B;
    HWREG(portBase + GPIO_O_CR) = pinMask;
}

void err_mailbox_full(char *fct, Mailbox_Handle mb)
{
    char lm[100];

    sprintf(lm, "MAILBOX FULL: %p in function %s", mb, fct);
    logevent(L_ERROR, lm);
}

static void reboot_system(UArg arg0, UArg arg1)
{
    // Reboot task
    // TODO: add here all processes which need to be closed correctly
    Semaphore_pend(sem_reboot, BIOS_WAIT_FOREVER);

    //Wait 5 seconds, then reboot
    Task_sleep(5000);
    SysCtlReset();
}

void err_sys_reboot()
{
#ifdef _DEBUG_
    // Loop on error in DEBUG mode
    #ifdef _LOGTASK_
    char lm[70*TASK_HISTORY_SIZE];

    printTaskHistory(lm);
    printf("%s",lm);
    #endif
    while (1)
    {
        ;
    }
#else
    // Restart on error in RELEASE mode
    SysCtlReset();
#endif
}

void err_sys_reboot_int(int status)
{
    err_sys_reboot();
}

void err_sys_reboot_str(char *status)
{
    err_sys_reboot();
}

// Task management utility function
bool safeTaskDelete(Task_Handle *taskHandle) {
    if (taskHandle == NULL || *taskHandle == NULL) {
        return false;
    }

    // Get task mode using available function
    Task_Mode mode = Task_getMode(*taskHandle);

    if ((mode != Task_Mode_RUNNING)) {
        if(mode != Task_Mode_TERMINATED)
            Task_delete(taskHandle);

        *taskHandle = NULL;  // Prevent double deletion
        return true;
    }
    else
        return false;
}


// Add this function anywhere in your file (before main)
void taskSwitchHook(Task_Handle prevTask, Task_Handle nextTask) {
#ifdef _LOGTASK_
    String prevName;
    String nextName;
    uint32_t index;

    if(db_get_db_ready() == false)
        return;

    updateSwitchTaskName(prevTask, "PREVIOUS");

    // Critical section for shared data
    Hwi_disable();
    // Add to history
    if(updateSwitchTaskName(nextTask, "NEXT"))
    {
        index = g_taskMonitor.historyIndex % TASK_HISTORY_SIZE;
        strncpy(g_taskMonitor.history[index].taskName, g_taskMonitor.currentTask, MAX_TASK_NAME_LEN-1);
        g_taskMonitor.history[index].taskName[MAX_TASK_NAME_LEN-1] = '\0';
        g_taskMonitor.history[index].timestamp = ClockP_getSystemTicks();

        g_taskMonitor.historyIndex++;
        g_taskMonitor.totalSwitches++;
    }
    Hwi_enable();
#endif
}
/*
void taskCreateHook(Task_Handle task, Error_Block *eb)
{
#ifdef _LOGTASK_

    String taskName;
    uint16_t taskNameStrLen;

    state_log_info_t logInfo;

//    Hwi_disable();  // Critical section

    if (task != NULL){
        taskName = Task_Handle_name(task);

        taskNameStrLen = strlen(taskName);

        if(taskNameStrLen)
        {
            logInfo.taskName = mem_alloc(taskNameStrLen+1);
            memcpy(logInfo.taskName, taskName, taskNameStrLen);
            *(logInfo.taskName+taskNameStrLen) = '\0';

            if(!logState(STARTING_TASK, logInfo, LOGDST_DEFAULT))
                mem_free(logInfo.taskName, taskNameStrLen+1);
        }
    }

//    Hwi_enable();
#endif
}

void taskDeleteHook(Task_Handle task)
{
#ifdef _LOGTASK_

    String taskName;
    uint16_t taskNameStrLen;

    state_log_info_t logInfo;

//    Hwi_disable();  // Critical section

    if (task != NULL){
        taskName = Task_Handle_name(task);

        taskNameStrLen = strlen(taskName);

        if(taskNameStrLen)
        {
            logInfo.taskName = mem_alloc(taskNameStrLen+1);
            memcpy(logInfo.taskName, taskName, taskNameStrLen);
            *(logInfo.taskName+taskNameStrLen) = '\0';

            if(!logState(ENDING_TASK, logInfo, LOGDST_DEFAULT))
                mem_free(logInfo.taskName, taskNameStrLen+1);
        }
    }

//    Hwi_enable();
#endif
}

bool logTaskState(char* taskName, gw_status_t status)
{
    msg_tasklog_t taskNameSend;
    uint16_t taskNameStrLen;

    taskNameStrLen = strlen(taskName);
    if(taskNameStrLen)
    {
        taskNameSend.taskName = taskName;
        taskNameSend.status = status;
        return checkTaskToLog(taskName) ? (Mailbox_post(m_task_log_send, &taskNameSend, BIOS_NO_WAIT) == true) : false;
    }
    else
        return false;
}
*/
bool checkTaskToLog(char* taskNameToCheck)
{
    if(strstr(taskNameToCheck, "static-instance-name") != NULL)
        return false;
    else if(strstr(taskNameToCheck, "ndkStackThread") != NULL)
        return false;
    else
        return true;
}

#ifdef _LOGTASK_
#ifdef _DEBUG_
// Capture and store problematic task handle
void captureProblematicTask(Task_Handle task, const char* context) {
    Hwi_disable();  // Critical section
    uint32_t i;
    // Check if we already have this task
    for (i = 0; i < g_problematicTaskCount; i++) {
        if (g_problematicTasks[i].handle == task) {
            g_problematicTasks[i].occurrenceCount++;
            Hwi_enable();
            return;
        }
    }

    // Add new problematic task
    if (g_problematicTaskCount < MAX_PROBLEMATIC_TASKS) {
        g_problematicTasks[g_problematicTaskCount].handle = task;
        g_problematicTasks[g_problematicTaskCount].firstSeen = ClockP_getSystemTicks();
        g_problematicTasks[g_problematicTaskCount].occurrenceCount = 1;
        g_problematicTasks[g_problematicTaskCount].identifiedName = "Unknown";//identifyTaskNameFromHandle(task);

        printf("  CAPTURED NEW: %p at tick %u, context: %s\n",
                     task, g_problematicTasks[g_problematicTaskCount].firstSeen, context);
        printf("  Identified as: %s\n", g_problematicTasks[g_problematicTaskCount].identifiedName);

        g_problematicTaskCount++;
    } else {
        printf("  WARNING: Max problematic tasks reached, cannot capture %p\n", task);
    }

    Hwi_enable();
}
#endif

/* Get the last running task name */
String getLastTaskName(void)
{
    return (String)g_taskMonitor.lastTask;
}

/* Get the current running task name */
String getCurrentTaskName(void)
{
    return (String)g_taskMonitor.currentTask;
}

/* Get total number of context switches */
UInt32 getTotalTaskSwitches(void)
{
    return g_taskMonitor.totalSwitches;
}

/* Print detailed task history */
void printTaskHistory(char* taskHistoryMsg)
{
    uint8_t i;
    uint8_t currLineLen = 0;
    uint8_t currHistoryIdx = 0;

    sprintf(taskHistoryMsg,"\n");
    currHistoryIdx++;

    for (i = 0; i < TASK_HISTORY_SIZE; i++) {
        currLineLen = printTaskHistoryEntryAtIndex(taskHistoryMsg + currHistoryIdx, i);
        if(currLineLen)
            currHistoryIdx += currLineLen;
        else
            continue;
    }
}

uint8_t printTaskHistoryEntryAtIndex(char* taskHistoryMsg, uint8_t index)
{
    uint8_t idx, start;
    start = (g_taskMonitor.historyIndex < TASK_HISTORY_SIZE) ?
            0 : g_taskMonitor.historyIndex % TASK_HISTORY_SIZE;

    idx = (start + index) % TASK_HISTORY_SIZE;
    return (g_taskMonitor.history[idx].taskName[0] != '\0') ?
            sprintf(taskHistoryMsg, "\t[%d] %-20s at tick %u\n",
                    index,
                    g_taskMonitor.history[idx].taskName,
                    g_taskMonitor.history[idx].timestamp)
            : 0;
}

bool updateSwitchTaskName(Task_Handle taskHandle,  const char* context)
{
    String taskName;
    char* taskNamePtr;
    if (taskHandle != NULL){
        taskName = Task_Handle_name(taskHandle);
        if (taskName == NULL) taskName = "Unknown";

        if (strstr(taskName, "static-instance-name") != NULL) {
            #ifdef _DEBUG_
            captureProblematicTask(taskHandle, context);
            #endif
            taskName = NULL;
            return false;
        }

        if (strstr(taskName, "ndkStackThread") != NULL) {
            taskName = NULL;
            return false;
        }

        if(taskName != NULL)
        {
            taskNamePtr = (strstr(context, "NEXT")!=NULL) ? (char*)g_taskMonitor.currentTask:(char*)g_taskMonitor.lastTask;

            // Update last task
            strncpy(taskNamePtr, taskName, MAX_TASK_NAME_LEN-1);
            *(taskNamePtr + MAX_TASK_NAME_LEN-1) = '\0';
            return true;
        }
    }

    return false;

}
#endif
