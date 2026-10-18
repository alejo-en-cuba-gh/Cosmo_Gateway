/*
 * can.c
 *
 *  Created on: 4 dic 2023
 */

#include <ti/sysbios/knl/Clock.h>

#include "common.h"
#include "can.h"
/*
Task_Handle t_can_r = NULL, t_can_alive = NULL;
Mailbox_Handle m_can_msg_r, m_can_alive;

void can_receiver(UArg arg0, UArg arg1)
{
    Task_Params taskParams;
    Mailbox_Params mbxParams;
    CAN_Frame cf;
    int32_t r;
    char lm[30];

    // Create can_msg_r mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_can_msg_r";
    m_can_msg_r = Mailbox_create(sizeof(CAN_Frame), MBOXSIZE, &mbxParams, NULL);

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);
    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    // Create the can_new_msg task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_HIGH;
    taskParams.instance->name = "t_can_r";
    t_can_r = Task_create(can_recv, &taskParams, NULL);

    while (1)
    {
        r = CAN_read(can, &cf, sizeof(cf));
        if (r > 0)
        {
            Mailbox_post(m_can_msg_r, &cf, BIOS_WAIT_FOREVER);
        }
        else
        {
            sprintf(lm, "can_receiver: error %d", r);
            logevent(L_ERROR, lm);
        }
    }
}

void can_recv(UArg arg0, UArg arg1)
{
    CAN_Frame msg;
    msg_event_t ev_in;
    msg_sll_t sm;
    char mac[13], lm[70];
    dbt_unit *un;
    uint8_t u;

    while (1)
    {
        // Receive the message from the mailbox
        Mailbox_pend(m_can_msg_r, &msg, BIOS_WAIT_FOREVER);

        switch (msg.can_id)
        {
        case C_EVENT_IN:
            if (msg.can_dlc == 3 && msg.data[0] > 1)
            {
                ev_in.unit = msg.data[0];
                ev_in.port = msg.data[1];
                ev_in.state = msg.data[2];
                if (Mailbox_post(m_event_in, &ev_in, BIOS_NO_WAIT) == false)
                {
                    err_mailbox_full(__func__, m_event_in);
                }
                sprintf(lm, "can_recv - event in: unit %d port %d state %d", msg.data[0], msg.data[1], msg.data[2]);
                logevent(L_DEBUG, lm);
            }
            break;
        case C_REGISTRATION:
            Task_sleep(1000);
            if (msg.can_dlc == 7)
            {
                // New units transmits MAC in 0-5, type in 6 and receives id in 7
                sprintf(mac, "%02X%02X%02X%02X%02X%02X", msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5]);
                un = find_element(&config_db.db_unit, &config_db.db_unit->hwid, false, UNIT, STRING, 0, 0, mac, 0);

                if (un == NULL)
                {
                    // Create new unit
                    un = db_add_item(UNIT, &config_db.db_unit, &config_db.db_unit_last);
                    if (un->id == 1)
                    {
                        // If main unit is basic there is no unit 1 so start with 2
                        un->id = 2;
                    }
                    strcpy(un->hwid, mac);
                    un->type = msg.data[6];
                    un->online = 1;
                    sprintf(un->name, "UNIT %d", un->id);
                    set_element((dbt_generic**) &un, UNIT, false);
                    create_items(un->type, un->id);
                    sprintf(lm, "can_recv - new unit created: unit %d type %d mac %s", un->id, un->type, mac);
                    logevent(L_INFO, lm);

                    msg.dlc = 8;
                    msg.data[7] = un->id;
                    if (Mailbox_post(m_can_out, &msg, BIOS_NO_WAIT) == false)
                    {
                        err_mailbox_full(__func__, m_can_out);
                    }
                    mem_free(un, sizeof(dbt_unit));
                }
                else
                {
                    msg.dlc = 8;
                    msg.data[7] = un->id;
                    if (Mailbox_post(m_can_out, &msg, BIOS_NO_WAIT) == false)
                    {
                        err_mailbox_full(__func__, m_can_out);
                    }
                    if (un->online == 0)
                    {
                        un->online = 1;
                        set_element((dbt_generic**) &un, UNIT, false);
                    }
                    // Unit already in database, assign ID to unit
                    sprintf(lm, "can_recv - new unit associated: unit %d type %d mac %s", un->id, un->type, mac);
                    logevent(L_INFO, lm);
                    restore_items(un->type, un->id, alphacom_set());

                    mem_free(un, sizeof(dbt_unit));
                }


                 if (oem == M_SCANVEST && un->acid && get_sll_ready())
                 {
                 sm.unit = un->acid;
                 sm.recv[0] = 0;
                 sm.recv[2] = 27;
                 if (Mailbox_post(m_sll_in, &sm, BIOS_NO_WAIT) == false)
                 {
                 err_mailbox_full(__func__, m_sll_in);
                 }
                 }

                if (t_can_alive == NULL)
                {
                    // Create the can alive mailbox
                    start_can_alive();
                }
            }
            break;
        case C_VERSIONS:
            if (msg.can_dlc == 3)
            {
                un = find_element(&config_db.db_unit, &config_db.db_unit->id, false, UNIT, NUMBER, 0, msg.data[0], "", N_UINT16);
                if (un)
                {
                    // Update unit versions in database
                    un->vhw = msg.data[1];
                    un->vfw = msg.data[2];
                    set_element((dbt_generic**) &un, UNIT, true);

                    sprintf(lm, "can_recv - unit version update: unit %d vhw %d vfw %d", msg.data[0], msg.data[1], msg.data[2]);
                    logevent(L_DEBUG, lm);
                }
            }
            break;
        case C_PONG:
            if (msg.can_dlc == 1)
            {
                u = msg.data[0];
                Mailbox_post(m_can_alive, &u, BIOS_NO_WAIT);
            }
            break;
        }
    }
}

void can_sender(UArg arg0, UArg arg1)
{
    CAN_Frame msg;
    char lm[100];
    uint8_t i;

    // Send reset message to the bus before starting
    msg.can_id = C_RESET;
    msg.can_dlc = 0;
    CAN_write(can, &msg, sizeof(msg));

    while (1)
    {
        // Receive the message from the mailbox
        Mailbox_pend(m_can_out, &msg, BIOS_WAIT_FOREVER);
        Task_sleep(2);
        CAN_write(can, &msg, sizeof(msg));
        sprintf(lm, "can_sender - id %d data", msg.can_id);
        for (i = 0; i < msg.can_dlc; i++)
        {
            sprintf(lm, "%s %d", lm, msg.data[i]);
        }
        logevent(L_DEBUG, lm);
    }
}

static void units_alive(UArg arg0, UArg arg1)
{
    uint8_t u;
    int8_t alive;
    dbt_unit *unit, *next;
    CAN_Frame msg;
    msg_sll_t sm;
    char lm[50];

    // Wait 2 minutes before starting
    Task_sleep(120000);

    while (1)
    {
        unit = get_element(&config_db.db_unit, UNIT);
        while (unit)
        {
            if (unit->id > 1 && unit->online > -1)
            {
                msg.can_id = C_PING;
                msg.can_dlc = 1;
                msg.data[0] = unit->id;
                if (Mailbox_post(m_can_out, &msg, BIOS_NO_WAIT) == false)
                {
                    err_mailbox_full(__func__, m_can_out);
                }

                // Wait 10 seconds for answer from unit
                if ( Mailbox_pend(m_can_alive, &u, 10000 * 1000 / Clock_tickPeriod) && (u == unit->id))
                {
                    // Unit alive
                    alive = 1;
                    sprintf(lm, "units_alive: unit %d alive", unit->id);
                    logevent(L_DEBUG, lm);
                }
                else
                {
                    alive = 0;
                    sprintf(lm, "units_alive: unit %d does not reply", unit->id);
                    logevent(L_ERROR, lm);
                }

                if (unit->online != alive)
                {
                    unit->online = alive;
                    set_element((dbt_generic**) &unit, UNIT, false);

                     if (oem == M_SCANVEST && unit->acid && get_sll_ready())
                     {
                     sm.unit = unit->acid;
                     sm.recv[0] = 0;
                     sm.recv[2] = 27;
                     if (Mailbox_post(m_sll_in, &sm, BIOS_NO_WAIT) == false)
                     {
                     err_mailbox_full(__func__, m_sll_in);
                     }
                     }

                }
            }

            if (unit->element->next)
            {
                // More units to check
                next = unit->element->next;
                mem_free(unit, sizeof(dbt_unit));
                unit = get_element(&next, UNIT);
            }
            else
            {
                // All units done
                mem_free(unit, sizeof(dbt_unit));
                unit = NULL;
            }
        }

        // Sleep for 90 seconds
        Task_sleep(90000);
    }
}

static void start_can_alive()
{
    Task_Params taskParams;
    Mailbox_Params mbxParams;

    // Create the can alive mailbox
    Mailbox_Params_init(&mbxParams);
    mbxParams.instance->name = "m_can_alive";
    m_can_alive = Mailbox_create(sizeof(uint8_t), 5, &mbxParams, NULL);

    // Create the can_new_msg task
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASK_NORMAL;
    taskParams.priority = TASK_PRIO_LOW;
    taskParams.instance->name = "t_can_alive";
    t_can_alive = Task_create(units_alive, &taskParams, NULL);
}
*/
