/*
 * driver-8io.c
 *
 *  Created on: 6 nov 2023
 */

#include "common.h"
#include "driver-8io.h"

Semaphore_Handle sem_gpio;
Mailbox_Handle m_gpio;

bool iobuf[19] = { 0 };
/*
 uint8_t set_8io(uint8_t id, bool state)
 {
 uint8_t io, led;

 switch (id)
 {
 case 11:
 io = OUT_1;
 led = LED_OUT_1_GPIO;
 break;
 case 12:
 io = OUT_2;
 led = LED_OUT_2_GPIO;
 break;
 case 13:
 io = OUT_3;
 led = LED_OUT_3_GPIO;
 break;
 case 14:
 io = OUT_4;
 led = LED_OUT_4_GPIO;
 break;
 case 15:
 io = OUT_5;
 led = LED_OUT_5_GPIO;
 break;
 case 16:
 io = OUT_6;
 led = LED_OUT_6_GPIO;
 break;
 case 17:
 io = OUT_7;
 led = LED_OUT_7_GPIO;
 break;
 case 18:
 io = OUT_8;
 led = LED_OUT_8_GPIO;
 break;
 default:
 return 0;
 }

 if (iobuf[id] == state)
 {
 // No state change, do nothing
 return 2;
 }
 else
 {
 // State change
 iobuf[id] = state;
 GPIO_write(io, state);
 GPIO_write(led, state);
 event_msg(id, state);
 return 1;
 }
 }*/

void get_8io(uint_least8_t index)
{
    UInt Hkey;

    // disable interrupt, pause_gpio will switch back on
    GPIO_disableInt(index);

    // Ensure atomic behavior: other HWI events are put to queue by TI-RTOS
    Hkey = Hwi_disable();

    // Send index to gpio_task
    Mailbox_post(m_gpio, &index, BIOS_NO_WAIT);

    // Back to normal
    Hwi_restore(Hkey);
}
/*
 void gpio_task(UArg arg0, UArg arg1)
 {
 uint8_t id, led, index;
 bool s;
 Task_Params taskParams;

 while (1)
 {
 // Pend the semaphore to wait for the Hwi
 Mailbox_pend(m_gpio, &index, BIOS_WAIT_FOREVER);

 switch (index)
 {
 case IN_1:
 id = 1;
 led = LED_IN_1_GPIO;
 break;
 case IN_2:
 id = 2;
 led = LED_IN_2_GPIO;
 break;
 case IN_3:
 id = 3;
 led = LED_IN_3_GPIO;
 break;
 case IN_4:
 id = 4;
 led = LED_IN_4_GPIO;
 break;
 case IN_5:
 id = 5;
 led = LED_IN_5_GPIO;
 break;
 case IN_6:
 id = 6;
 led = LED_IN_6_GPIO;
 break;
 case IN_7:
 id = 7;
 led = LED_IN_7_GPIO;
 break;
 case IN_8:
 id = 8;
 led = LED_IN_8_GPIO;
 break;
 }

 Task_Params_init(&taskParams);
 taskParams.priority = TASK_PRIO_MEDIUM;
 taskParams.stackSize = TASK_LIGHT;
 taskParams.arg0 = (UArg) index;
 taskParams.instance->name = "pause_gpio";
 Task_create(pause_gpio, &taskParams, NULL);

 s = !GPIO_read(index);
 if (iobuf[id] != s)
 {
 // state change
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);
 }
 }
 }
 */
static void event_msg(uint8_t port, bool state)
{
    msg_event_t msg;

    msg.unit = UNIT_ID;
    msg.port = port;
    msg.state = state;
    if (Mailbox_post(m_event_in, &msg, BIOS_NO_WAIT) == false)
    {
        err_mailbox_full(__func__, m_event_in);
    }
}

void pause_gpio(UArg arg0, UArg arg1)
{
    Task_sleep(50); // Wait 50 ms before accepting new events for GPIO IRQ
    GPIO_enableInt((uint8_t) arg0);
    Task_exit();
}
/*
 void get_io_status()
 {
 uint8_t id, io, led;
 bool s;

 io = IN_1;
 id = 1;
 led = LED_IN_1_GPIO;
 s = !GPIO_read(io);
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);

 io = IN_2;
 id = 2;
 led = LED_IN_2_GPIO;
 s = !GPIO_read(io);
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);

 io = IN_3;
 id = 3;
 led = LED_IN_3_GPIO;
 s = !GPIO_read(io);
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);

 io = IN_4;
 id = 4;
 led = LED_IN_4_GPIO;
 s = !GPIO_read(io);
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);

 io = IN_5;
 id = 5;
 led = LED_IN_5_GPIO;
 s = !GPIO_read(io);
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);

 io = IN_6;
 id = 6;
 led = LED_IN_6_GPIO;
 s = !GPIO_read(io);
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);

 io = IN_7;
 id = 7;
 led = LED_IN_7_GPIO;
 s = !GPIO_read(io);
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);

 io = IN_8;
 id = 8;
 led = LED_IN_8_GPIO;
 s = !GPIO_read(io);
 iobuf[id] = s;
 event_msg(id, s);
 GPIO_write(led, s);
 }*/
