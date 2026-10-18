/*
 * event.c
 *
 *  Created on: 6 nov 2023
 */

#include "common.h"
#include "event.h"
/*
void event_in(UArg arg0, UArg arg1)
{
    msg_event_t msg;
    msg_sll_t ac;
    dbt_item *item;
    uint8_t d;
    uint16_t item_id, rule_id;
    bool r;
    char lm[50];

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);

    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    while (1)
    {
        // Receive the message from the mailbox
        Mailbox_pend(m_event_in, &msg, BIOS_WAIT_FOREVER);

        sprintf(lm, "event_in: unit %d port %d state %d", msg.unit, msg.port, msg.state);
        logevent(L_EVENT, lm);

        // Find item ID in database
        if (msg.unit)
        {
            // Message with unit and port
            item = (dbt_item*) find_element_and((dbt_generic**) &config_db.db_item, false, ITEM, (char**) &config_db.db_item->port, NUMBER, 0, msg.port, "", (char**) &config_db.db_item->unit, NUMBER, 0, msg.unit, "");
        }
        else
        {
            // Message with unit = 0, so port is item id
            item = (dbt_item*) find_element((dbt_generic**) &config_db.db_item, (char**) &config_db.db_item->id, false, ITEM, NUMBER, 0, msg.port, "", N_UINT16);
        }

        if (item)
        {
            if (item->state != msg.state)
            {
                item_id = item->id;
                // Update item state in database
                item->state = msg.state;
                // As IT_OUT == 1, use this value to set significant flag, i.e. only output states are significant

                 if (item->type == IT_IN && oem == M_SCANVEST && get_sll_ready())
                 {
                 d = get_ac_device(item->unit);
                 if (d)
                 {
                 ac.unit = d;
                 ac.port = msg.port;
                 ac.state = msg.state;
                 ac.recv[0] = 0;
                 if (Mailbox_post(m_sll_in, &ac, BIOS_NO_WAIT) == false)
                 {
                 err_mailbox_full((char*) __func__, m_sll_in);
                 }
                 }
                 }
                set_element((dbt_generic**) &item, ITEM, false);
            }
            mem_free(item, sizeof(dbt_item));
        }
    }
}

void event_out(UArg arg0, UArg arg1)
{
    msg_event_t msg;
    dbt_item *item;
    CAN_Frame can;
    char lm[50];

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);

    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    while (1)
    {
        // Receive the message from the mailbox
        Mailbox_pend(m_event_out, &msg, BIOS_WAIT_FOREVER);

        if (msg.unit == 0)
        {
            // Translate item id into unit/port
            item = find_element(&config_db.db_item, &config_db.db_item->id, false, ITEM, NUMBER, 0, msg.port, "", N_UINT16);
            if (item)
            {
                msg.unit = item->unit;
                msg.port = item->port;
                mem_free(item, sizeof(dbt_item));
            }
            else
            {
                // Invalid item
                continue;
            }
        }

        if (msg.unit > 1)
        {
            // Event for external unit, send to CAN
            can.can_id = C_EVENT_OUT;
            can.data[0] = msg.unit;
            can.data[1] = msg.port;
            can.data[2] = msg.state;
            can.dlc = 3;
            Mailbox_post(m_can_out, &can, BIOS_WAIT_FOREVER);
            sprintf(lm, "event_out: CAN unit %d port %d state %d", msg.unit, msg.port, msg.state);
        }
        else
        {
            // Local execution
            //set_8io(msg.port, msg.state);
            //sprintf(lm, "event_out: local port %d state %d", msg.port, msg.state);
        }
        logevent(L_EVENT, lm);
    }
}*/

