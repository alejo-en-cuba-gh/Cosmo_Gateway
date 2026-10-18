/*
 * json-db.c
 *
 *  Created on: 21 ott 2023
 */

#include "common.h"
#include "config-db.h"

// DB semaphores
Semaphore_Handle sem_db_rw, sem_db_ro, sem_db_ready, sem_db_updated;
Mailbox_Handle m_db;
uint8_t sem_db_ro_count = 0;
bool db_ready = false, db_changed = false, app_connected = false;

// Database structure
dbt_db config_db;

// DB table versions
uint16_t tversion[N_DBTABLES + 1] = { 0 }; // Update when adding new tables!!!

const uint16_t vdb = 5, vfw = 19;
uint16_t fv;
extern uint8_t get_appcount();
extern bool db_get_db_ready();

// Define a function to create a JSON object from the data and template
void db_init(UArg arg0, UArg arg1)
{
    uint8_t i;
    bool r = false;
    db_event_t e;
    char lm[150];

    // Create DB semaphores
    Semaphore_Params params;
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_db_ro";
    sem_db_ro = Semaphore_create(1, &params, NULL);

    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    params.instance->name = "sem_db_rw";
    sem_db_rw = Semaphore_create(1, &params, NULL);

    // Initialize flash memory
    // For simplicity, use DB semaphores also for Flash
    flash_init();

    // Initialize database
    config_db.db_item = NULL;
    config_db.db_item_last = NULL;
    config_db.db_unit = NULL;
    config_db.db_unit_last = NULL;
    config_db.db_rule = NULL;
    config_db.db_rule_last = NULL;
    config_db.db_rulepart = NULL;
    config_db.db_rulepart_last = NULL;
    config_db.db_ruletimer = NULL;
    config_db.db_ruletimer_last = NULL;
    config_db.db_action = NULL;
    config_db.db_action_last = NULL;
    config_db.db_timer = NULL;
    config_db.db_timer_last = NULL;
    config_db.db_user = NULL;
    config_db.db_user_last = NULL;
    config_db.db_group = NULL;
    config_db.db_group_last = NULL;
    config_db.db_groupitem = NULL;
    config_db.db_groupitem_last = NULL;
    config_db.db_device = NULL;
    config_db.db_device_last = NULL;
    config_db.db_zdevice = NULL;
    config_db.db_zdevice_last = NULL;
    config_db.db_zendpoint = NULL;
    config_db.db_zendpoint_last = NULL;
    /*
     Semaphore_pend(sem_sd, BIOS_WAIT_FOREVER);
     Semaphore_post(sem_sd);
     */
    Semaphore_pend(sem_spiffs_mount, BIOS_WAIT_FOREVER);
    Semaphore_post(sem_spiffs_mount);

     if (check_cfg_file("sw.cfg"))
     {
         // Import and apply configuration file
         r = import_config(&config_db, "sw.cfg");
         sprintf(lm, "db_init - get configuration from sw.cfg");
         logevent(L_INFO, lm);
     }
     else if (check_cfg_file("sw.cfg.bu"))
     {
         // Import and apply backup configuration
         r = import_config(&config_db, "sw.cfg.bu");
         if (r)
         {
             export_config(&config_db, "sw.cfg");
         }
         sprintf(lm, "db_init - get configuration from sw.cfg.bu");
          logevent(L_INFO, lm);
     }

    if (r == false)
    {
        // No valid configuration available, start with default settings
        db_add_item_raw(sizeof(dbt_device), (dbt_generic**) &config_db.db_device, (dbt_generic**) &config_db.db_device_last);
        config_db.db_device->mac[0] = 0;
        config_db.db_device->idate = 0;
        config_db.db_device->name[0] = 0;
        config_db.db_device->location[0] = 0;
        config_db.db_device->lan_dhcp = true;
        config_db.db_device->lan_ip = 0;
        config_db.db_device->lan_sn = 0;
        config_db.db_device->lan_gw = 0;
        config_db.db_device->lan_dns = 0;
        sprintf(config_db.db_device->time, "time.windows.com");
        config_db.db_device->dst = 1;
        config_db.db_device->timezone = -4;
        config_db.db_device->vhw = 1;
        config_db.db_device->loglev = 2;
        config_db.db_device->logudp = 20000;
        config_db.db_device->acip = 0;
        config_db.db_device->acp = 0;

        // Create first unit
        if (oem == M_SCANVEST)
        {
            db_add_item_raw(sizeof(dbt_unit), (dbt_generic**) &config_db.db_unit, (dbt_generic**) &config_db.db_unit_last);
            config_db.db_unit->hwid[0] = 0;
            config_db.db_unit->type = UT_MU_8IO_S;
            config_db.db_unit->vhw = 0;
            config_db.db_unit->vfw = 0;
            config_db.db_unit->acid = 1;
            config_db.db_unit->online = 1;
            sprintf(config_db.db_unit->name, "UNIT %d", config_db.db_unit->id);

            // Create 8 IO items in unit 1
            for (i = 1; i < 9; i++)
            {
                db_add_item_raw(sizeof(dbt_item), (dbt_generic**) &config_db.db_item, (dbt_generic**) &config_db.db_item_last);
                config_db.db_item_last->unit = 1;
                config_db.db_item_last->component = 0;
                config_db.db_item_last->port = i;
                config_db.db_item_last->type = IT_IN;
                config_db.db_item_last->icon = 0;
                config_db.db_item_last->state = 0;
                sprintf(config_db.db_item_last->name, "IN %d/%d",
                        config_db.db_item_last->unit,
                        i);

                db_add_item_raw(sizeof(dbt_item), (dbt_generic**) &config_db.db_item, (dbt_generic**) &config_db.db_item_last);
                config_db.db_item_last->unit = 1;
                config_db.db_item_last->component = 0;
                config_db.db_item_last->port = i + 10;
                config_db.db_item_last->type = IT_OUT;
                config_db.db_item_last->icon = 0;
                config_db.db_item_last->state = 0;
                sprintf(config_db.db_item_last->name, "OUT %d/%d",
                        config_db.db_item_last->unit,
                        i);
            }
        }

        // Create admin user
        db_add_item_raw(sizeof(dbt_user), (dbt_generic**) &config_db.db_user, (dbt_generic**) &config_db.db_user_last);
        sprintf(config_db.db_user->name, "sw");
        config_db.db_user->pw = hash_pw("sw");
        sprintf(config_db.db_user->email, "sw");
        config_db.db_user->level = 3;

        // Save configuration
        export_config(&config_db, "sw.cfg");
        export_config(&config_db, "sw.cfg.bu");
        //Factory
        i = 0;

        flash_write_val(FL_STATUS_1, &i, 1);

        sprintf(lm, "db_init - reset configuration to default");
        logevent(L_INFO, lm);
    }

    // Set versions
    config_db.db_device->vfw = vfw;
    config_db.db_device->vdb = vdb;

    //config_db.db_unit->vfw = config_db.db_device->vfw;
    //config_db.db_unit->vhw = 1;

    sprintf(lm, "%s, firmware version %d, database version %d", lm, vfw, vdb);
    logevent(L_INFO, lm);

    // Check if versions need flash update
    flash_read_val(FL_FW_2, &fv, 2);
    if (fv != vfw)
    {
        // Flash version != firmware version means flash not initialized
        // Reset all parameters
        // Set firmware
        fv = vfw;
        flash_write_val(FL_FW_2, &fv, 2);

        if (fv == 0xFFFF)
        {
            // Empty flash
            // Set OEM
            uint8_t u8 = oem;
            flash_write_val(FL_OEM_1, &u8, 1);
            // Reset installation date
            uint32_t u32 = 0;
            flash_write_val(FL_INS_DATE_4, &u32, 4);
        }
    }

    // Set DB ready
    db_ready = true;
    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    // Update status of all IO ports
    if (mu == MU_8IO)
    {
        //get_io_status();
        //restore_items(UT_8IO, 1, alphacom_set());
    }

    Task_sleep(5000);
    logevent(L_INFO, lm);

    while (1)
    {
        Mailbox_pend(m_db, &e, BIOS_WAIT_FOREVER);
        // Do not backup configuration while ftp session is active
        if (e == DBE_EXPORT && db_changed)
        {
            // Update database copy on SD card
            if (check_cfg_file("sw.cfg"))
            {
                copy_file("sw.cfg", "sw.cfg.bu");
            }

            // Check if backup file is OK and overwrite current config
            if (check_cfg_file("sw.cfg.bu"))
            {
                export_config(&config_db, "sw.cfg");
            }

            // Get semaphore for writing to DB
            Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);
            db_changed = false;
            Semaphore_post(sem_db_rw);
        }
    }
}

static uint16_t get_offset(char **item, char **element)
{
    return ((char*) element) - (char*) *item;
}

dbt_generic* db_add_item(db_type type, dbt_generic **db_item, dbt_generic **last_item)
{

    dbt_generic *element;

    element = calloc(1, get_el_size(type));
    if (element == NULL)
    {
        // printf("Memory allocation failed\n");
        return NULL;
    }

    Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);

    if (*db_item)
    {
        element->previous = *last_item;
        element->id = (*last_item)->id + 1;
        (*last_item)->next = element;
    }
    else
    {
        *db_item = element;
        element->id = 1;
    }
    *last_item = element;

    // Increase table version identifier by 1
    tversion[type]++;

    // Release semaphore for writing to DB
    Semaphore_post(sem_db_rw);

    return get_element(&element, type);
}

bool db_add_item_raw(int item_size, dbt_generic **db_item, dbt_generic **last_item)
{
    // For direct write to DB, use for import only
    dbt_generic *element;

    element = calloc(1, item_size);
    if (element == NULL)
    {
        // printf("Memory allocation failed\n");
        return false;
    }

    if (*db_item)
    {
        element->previous = *last_item;
        element->id = (*last_item)->id + 1;
        (*last_item)->next = element;
    }
    else
    {
        *db_item = element;
        element->id = 1;
    }
    *last_item = element;

    return true;
}

bool db_delete_item(dbt_generic **user_item, dbt_generic **first_item, dbt_generic **last_item, db_type type)
{
    /*
     //Send to all users the deleted item
     msg_dbev_t msg;
     msg.s = NULL;
     msg.esize = 30;
     msg.entry = mem_alloc(30);
     msg.rec_id = (*user_item)->id;
     msg.tab_id = type;

     sprintf(msg.entry, "%d,%d,%d###", AM_DBDELETE, msg.tab_id, msg.rec_id);
     Mailbox_post(m_lappsend, &msg, BIOS_WAIT_FOREVER);
     */
    // Get semaphore for writing to DB
    Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);

    dbt_generic *item = (*user_item)->element;

    if (item == NULL)
    {
        // printf("Invalid element\n");

        // Release semaphore for writing to DB
        Semaphore_post(sem_db_rw);
        return false;
    }

    if (item->previous)
    {
        // Item to be deleted is not the first in the list
        if (item->next)
        {
            item->previous->next = item->next;
            item->next->previous = item->previous;
        }
        else
        {
            // Item to be deleted is last in the list
            item->previous->next = NULL;
            *last_item = item->previous;
        }
    }
    else
    {
        if (item->next)
        {
            // Make second item become first item
            item->next->previous = NULL;
            *first_item = item->next;
        }
        else
        {
            // Empty list
            *first_item = NULL;
            *last_item = NULL;
        }
    }

    // Increase table version identifier by 1
    tversion[type]++;
    db_changed = true;

    // Release semaphore for writing to DB
    Semaphore_post(sem_db_rw);

    free_element(item, type);
    free_element(*user_item, type);
    return true;
}

dbt_generic* find_element(dbt_generic **base, char **search_item, bool next,
                          db_type type,
                          item_type s_type, bool s_bool,
                          uint64_t s_number,
                          char *s_string,
                          number_type num_type)
{
    bool found = true;
    void *char_ptr = NULL;
    unsigned short *test = NULL;
    uint16_t s_item_offset;
    dbt_generic *item;

    if (*base == NULL)
    {
        return NULL;
    }

    // Get semaphore for reading from from DB
    sem_db_start_read();

    s_item_offset = get_offset(base, search_item);
    item = *base;

    // if next is true, start searching from the record after item (if exists)
    if (item->next && next)
    {
        item = item->next;
    }
    else if (item->next == NULL && next)
    {
        sem_db_end_read();
        return NULL;
    }

    switch ((int) s_type)
    {
    case BOOL:
        while (((bool) *((char*) item + s_item_offset)) != s_bool)
        {
            if (item->next)
            {
                item = item->next;
            }
            else
            {
                found = false;
                break;
            }
        }
        break;
    case NUMBER:
        switch (num_type)
        {
        case N_INT:
            while (*(int*) ((char*) item + s_item_offset) != s_number)
            {

                if (item->next)
                {
                    item = item->next;
                }
                else
                {
                    found = false;
                    break;
                }
            }
            break;
        case N_UINT8:
            while (*(uint8_t*) ((char*) item + s_item_offset) != s_number)
            {

                if (item->next)
                {
                    item = item->next;
                }
                else
                {
                    found = false;
                    break;
                }
            }
            break;
        case N_UINT16:
            while (*(uint16_t*) ((char*) item + s_item_offset) != s_number)
            {

                if (item->next)
                {
                    item = item->next;
                }
                else
                {
                    found = false;
                    break;
                }
            }
            break;
        case N_UINT64:
            while (*(uint64_t*) ((char*) item + s_item_offset) != s_number)
            {

                if (item->next)
                {
                    item = item->next;
                }
                else
                {
                    found = false;
                    break;
                }
            }
            break;
        case N_INT8:
            while (*(int8_t*) ((char*) item + s_item_offset) != s_number)
            {

                if (item->next)
                {
                    item = item->next;
                }
                else
                {
                    found = false;
                    break;
                }
            }
            break;
        case N_INT16:
            while (*(int16_t*) ((char*) item + s_item_offset) != s_number)
            {

                if (item->next)
                {
                    item = item->next;
                }
                else
                {
                    found = false;
                    break;
                }
            }
            break;
        }

        break;
    case STRING:
        while (strcmp(((char*) item + s_item_offset), s_string))
        {
            if (item->next)
            {
                item = item->next;
            }
            else
            {
                found = false;
                break;
            }
        }
        break;
    }

    // Release semaphore for reading from DB
    sem_db_end_read();

    if (found)
    {
        return get_element(&item, type);
    }
    else
    {
        return NULL;
    }
}

dbt_generic* find_element_and(dbt_generic **base, bool next, db_type type, char **search_item1, item_type s_type1, bool s_bool1, int s_number1, char *s_string1,
                              char **search_item2,
                              item_type s_type2, bool s_bool2, int s_number2, char *s_string2)
{
    bool found = false;
    uint16_t s_item_offset1, s_item_offset2;
    dbt_generic *item;

    if (*base == NULL)
    {
        return NULL;
    }

    // Get semaphore for reading from from DB
    sem_db_start_read();

    s_item_offset1 = get_offset(base, search_item1);
    s_item_offset2 = get_offset(base, search_item2);
    item = *base;

    // if next is true, start searching from the record after item (if exists)
    if (item->next && next)
    {
        item = item->next;
    }
    else if (item->next == NULL && next)
    {
        sem_db_end_read();
        return NULL;
    }

    while (true)
    {
        switch ((int) s_type1)
        {
        case BOOL:
            if (((bool) *((char*) item + s_item_offset1)) == s_bool1)
            {
                switch ((int) s_type2)
                {
                case BOOL:
                    found = ((bool) *((char*) item + s_item_offset2))
                    == s_bool2;
                    break;
                case NUMBER:
                    found = ((int) *((char*) item + s_item_offset2))
                    == s_number2;
                    break;
                case STRING:
                    if (strcmp(((char*) item + s_item_offset2), s_string2) == 0)
                    {
                        found = true;
                    }
                    else
                    {
                        found = false;
                    }
                }
            }
            break;
        case NUMBER:
            if (((int) *((char*) item + s_item_offset1)) == s_number1)
            {
                switch ((int) s_type2)
                {
                case BOOL:
                    found = ((bool) *((char*) item + s_item_offset2))
                    == s_bool2;
                    break;
                case NUMBER:
                    found = ((int) *((char*) item + s_item_offset2))
                    == s_number2;
                    break;
                case STRING:
                    if (strcmp(((char*) item + s_item_offset2), s_string2) == 0)
                    {
                        found = true;
                    }
                    else
                    {
                        found = false;
                    }
                }
            }
            break;
        case STRING:
            if (strcmp(((char*) item + s_item_offset1), s_string1))
            {
                switch ((int) s_type2)
                {
                case BOOL:
                    found = ((bool) *((char*) item + s_item_offset2))
                    == s_bool2;
                    break;
                case NUMBER:
                    found = ((int) *((char*) item + s_item_offset2))
                    == s_number2;
                    break;
                case STRING:
                    if (strcmp(((char*) item + s_item_offset2), s_string2) == 0)
                    {
                        found = true;
                    }
                    else
                    {
                        found = false;
                    }
                }
            }
            break;
        }

        if (found || item->next == NULL)
        {
            break;
        }
        else
        {
            item = item->next;
        }
    }

    // Release semaphore for reading from DB
    sem_db_end_read();

    if (found)
    {
        return get_element(&item, type);
    }
    else
    {
        return NULL;
    }
}

void empty_table(dbt_generic **item, dbt_generic **last, db_type type,
bool handle_sem)
{
    dbt_generic *e, *f;

    if (handle_sem)
    {
        // Get semaphore for writing to DB
        Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);
    }

    e = *last;

    while (e != NULL)
    {
        f = e;
        if (e->previous)
        {
            e = e->previous;
        }
        else
        {
            e = NULL;
        }

        free_element(f, type);
    }

    *last = NULL;
    *item = NULL;

    if (handle_sem)
    {
        // Release semaphore for writing to DB
        Semaphore_post(sem_db_rw);
    }
}

static int free_element(dbt_generic **e, db_type type)
{
    switch ((int) type)
    {
    case ITEM:
        mem_free(e, sizeof(dbt_item));
        break;
    case COMPONENT:
        mem_free(e, sizeof(dbt_component));
        break;
    case UNIT:
        mem_free(e, sizeof(dbt_unit));
        break;
    case RULE:
        mem_free(e, sizeof(dbt_rule));
        break;
    case RULEPART:
        mem_free(e, sizeof(dbt_rulepart));
        break;
    case RULETIMER:
        mem_free(e, sizeof(dbt_ruletimer));
        break;
    case TIMER:
        mem_free(e, sizeof(dbt_timer));
        break;
    case DEVICE:
        mem_free(e, sizeof(dbt_device));
        break;
    case USER:
        mem_free(e, sizeof(dbt_user));
        break;
    case GROUP:
        mem_free(e, sizeof(dbt_group));
        break;
    case GROUPITEM:
        mem_free(e, sizeof(dbt_groupitem));
        break;
    case ZDEVICE:
        mem_free(e, sizeof(dbt_zdevice));
        break;
    case ZENDPOINT:
        mem_free(e, sizeof(dbt_zendpoint));
        break;
    default:
        return 0;
    }

    return 1;
}

dbt_generic* get_element(dbt_generic **e, db_type type)
{
    // Returns a copy of the requested element writing the item's original memory address in the base field
    dbt_generic *item;
    uint16_t s, o;

    if (*e == NULL)
    {
        return NULL;
    }

    // Get semaphore for reading from DB
    sem_db_start_read();

    o = get_offset(e, &((*e)->id));
    s = get_el_size(type);

    item = mem_alloc(s);
    memcpy(&(item->id), &((*e)->id), s - o);
    item->element = *e;

    // Release semaphore for reading from DB
    sem_db_end_read();

    return item;
}

int get_el_size(db_type type)
{
    switch ((int) type)
    {
    case ITEM:
        return sizeof(dbt_item);
    case COMPONENT:
        return sizeof(dbt_component);
    case UNIT:
        return sizeof(dbt_unit);
    case RULE:
        return sizeof(dbt_rule);
    case RULEPART:
        return sizeof(dbt_rulepart);
    case RULETIMER:
        return sizeof(dbt_ruletimer);
    case TIMER:
        return sizeof(dbt_timer);
    case ACTION:
        return sizeof(dbt_action);
    case DEVICE:
        return sizeof(dbt_device);
    case USER:
        return sizeof(dbt_user);
    case GROUP:
        return sizeof(dbt_group);
    case GROUPITEM:
        return sizeof(dbt_groupitem);
    case ZDEVICE:
        return sizeof(dbt_zdevice);
    case ZENDPOINT:
        return sizeof(dbt_zendpoint);
    default:
        return 0;
    }
}

bool set_element(dbt_generic **e, db_type type, bool freemem)
{
    // Returns a copy of the requested element writing the item's original memory address in the base field
    int s, o;
    msg_dbev_t msg;
    db_event_t d;

    s = get_el_size(type);

    // Get semaphore for writing to DB
    Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);

    o = get_offset((char**) e, (char**) &((*e)->id));
    memcpy(&((*e)->element->id), &((*e)->id), s - o);

    // Increase table version identifier by 1
    tversion[type]++;
    db_changed = true;

    // Release semaphore for writing to DB
    Semaphore_post(sem_db_rw);
    /*
     if (get_appcount())
     {
     // Local app connected
     msg.s = NULL;
     msg.tab_id = type;
     msg.rec_id = (*e)->id;
     parse_dbget(&msg);
     }*/

    if (freemem)
    {
        free_element((dbt_generic**) *e, type);
    }

    return 1;
}

void sem_db_start_read()
{
    // Get semaphore for reading from DB
    // Pend on the read semaphore
    Semaphore_pend(sem_db_ro, BIOS_WAIT_FOREVER);

    // Increment the reader count
    sem_db_ro_count++;

    // If this is the first reader, pend on the write semaphore
    if (sem_db_ro_count == 1)
    {
        Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);
    }

    // Release the read semaphore
    Semaphore_post(sem_db_ro);
}

void sem_db_end_read()
{
    // Release semaphore for reading from DB
    // Pend on the read semaphore
    Semaphore_pend(sem_db_ro, BIOS_WAIT_FOREVER);

    // Decrement the reader count
    sem_db_ro_count--;

    // If this is the last reader, release the write semaphore
    if (sem_db_ro_count == 0)
    {
        Semaphore_post(sem_db_rw);
    }

    // Release the read semaphore
    Semaphore_post(sem_db_ro);
}

int8_t get_loglevel()
{
    int8_t l;
    sem_db_start_read();
    l = config_db.db_device->loglev;
    sem_db_end_read();

    return l;
}

#ifdef _LOGUDP_
uint16_t get_logudp()
{
    uint16_t l;

    sem_db_start_read();
    l = config_db.db_device->logudp;
    sem_db_end_read();

    return l;
}
#endif

bool get_dhcp()
{
    bool d;

    sem_db_start_read();
    d = config_db.db_device->lan_dhcp;
    sem_db_end_read();

    return d;
}

bool alphacom_set()
{
    bool l;

    sem_db_start_read();
    if (config_db.db_device->acip && config_db.db_device->acp)
    {
        l = true;
    }
    else
    {
        l = false;
    }
    sem_db_end_read();

    return l;
}

int16_t get_item_state(uint16_t unit, uint8_t port)
{
    dbt_item *item;
    uint8_t s;

    item = find_element_and(&config_db.db_item, false, ITEM,
                            &config_db.db_item->port,
                            NUMBER, 0, port, "",
                            &config_db.db_item->unit,
                            NUMBER, 0, unit, "");

    if (item)
    {
        s = item->state;
        mem_free(item, sizeof(dbt_item));
        return s;
    }
    else
    {
        return -1;
    }
}

bool set_rule_state(uint16_t unit, uint8_t id)
{
    dbt_rule *rule;

    rule = find_element(&config_db.db_rule, &config_db.db_rule->id, false, RULE,
                        NUMBER,
                        0, id, "", N_UINT16);

    if (rule)
    {
        rule->enabled = !rule->enabled;
        set_element((dbt_generic**) &rule, RULE, true);
        return 1;
    }
    else
    {
        return 0;
    }
}

bool set_group_state(uint16_t group, bool state)
{
    dbt_groupitem *gitem, *g_el;
    dbt_item *item;
    msg_event_t ev_out;

    gitem = find_element(&config_db.db_groupitem,
                         &config_db.db_groupitem->group,
                         false, ITEM, NUMBER, 0,
                         group,
                         "", N_UINT16);

    while (gitem)
    {
        item = find_element(&config_db.db_item, &config_db.db_item->id, false,
                            ITEM,
                            NUMBER, 0, gitem->item, "", N_UINT16);
        if (item && item->type == IT_OUT)
        {
            ev_out.unit = item->unit;
            ev_out.port = item->port;
            ev_out.state = state;
            Mailbox_post(m_event_out, &ev_out, BIOS_WAIT_FOREVER);
            mem_free(item, sizeof(dbt_item));
        }
        g_el = gitem->element;
        mem_free(gitem, sizeof(dbt_groupitem));
        gitem = find_element(&g_el, &g_el->group, true, ITEM, NUMBER, 0, group,
                             "",
                             N_UINT16);
    }
}

uint8_t create_items(unit_t type, uint8_t uid)
{
    uint8_t i;
    dbt_item *item;
    dbt_unit *unit;

    switch (type)
    {
    case UT_8IO_S:
        // Check if acid has been assigned
        unit = find_element(&config_db.db_unit, &config_db.db_unit->id, false,
                            UNIT,
                            NUMBER, 0, uid, "", N_UINT16);
        if (unit->acid == 0)
        {
            unit->acid = get_next_acid(uid);
            set_element((dbt_generic**) &unit, UNIT, false);
        }
        mem_free(unit, sizeof(dbt_unit));
    case UT_8IO:
        for (i = 1; i < 9; i++)
        {
            item = db_add_item(ITEM, &config_db.db_item,
                               &config_db.db_item_last);
            item->unit = uid;
            item->component = 0;
            item->port = i;
            item->type = IT_IN;
            item->icon = 0;
            item->state = 0;
            item->name[0] = 0;
            sprintf(item->name, "IN %d/%d", item->unit, i);
            set_element((dbt_generic**) &item, ITEM, true);

            item = db_add_item(ITEM, &config_db.db_item,
                               &config_db.db_item_last);
            item->unit = uid;
            item->component = 0;
            item->port = i + 10;
            item->type = IT_OUT;
            item->icon = 0;
            item->state = 0;
            sprintf(item->name, "OUT %d/%d", item->unit, i);
            set_element((dbt_generic**) &item, ITEM, true);
        }
        break;
    }

    return 1;
}

uint8_t restore_items(unit_t type, uint8_t uid, bool all_off)
{
    // Restores the states of all outputs according to database
    uint8_t i;
    dbt_item *item;
    msg_event_t ev;

    ev.unit = uid;

    switch (type)
    {
    case UT_8IO_S:
        case UT_8IO:
        for (i = 11; i < 19; i++)
        {
            item = find_element_and(&config_db.db_item, false, ITEM,
                                    &config_db.db_item->port,
                                    NUMBER, 0, i, "",
                                    &config_db.db_item->unit,
                                    NUMBER, 0, uid,
                                    "");
            if (item)
            {
                ev.port = item->port;
                if (all_off)
                {
                    ev.state = false;
                }
                else
                {
                    ev.state = item->state;
                }

                if (Mailbox_post(m_event_out, &ev, BIOS_WAIT_FOREVER) == false)
                {
                    err_mailbox_full(__func__, m_event_out);
                }
                mem_free(item, sizeof(dbt_item));

            }

        }
        break;
    }

    return 1;
}

static uint8_t get_next_acid()
{
    // Gets next free acid for unit
    sem_db_start_read();

    uint8_t ai = 0;
    dbt_unit *unit;

    unit = config_db.db_unit;
    while (unit)
    {
        if (unit->acid > ai)
        {
            ai = unit->acid;
        }

        unit = unit->next;
    }

    sem_db_end_read();

    if (ai)
    {
        return ai + 1;
    }
    else
    {
        return 0;
    }
}

uint16_t get_tversion(db_type t)
{
    uint16_t v;

    sem_db_start_read();
    if (t)
    {
        v = tversion[t];
    }
    else
    {
        v = config_db.db_device->vdb;
    }

    sem_db_end_read();
    return v;
}

void get_tversion_all(char *s)
{
    uint8_t v;
    char b[7];

    sem_db_start_read();

    sprintf(s, "%d", config_db.db_device->vdb);

    for (v = 1; v <= N_DBTABLES; v++)
    {
        sprintf(b, ",%d", tversion[v]);
        strcat(s, b);
    }

    sem_db_end_read();
}

void inc_tversion(db_type t)
{
    Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);
    tversion[t]++;
    Semaphore_post(sem_db_rw);
}

bool parse_dbdelete(msg_dbev_t *msg)
{
    dbt_item *item;
    dbt_component *component;
    dbt_unit *unit;
    dbt_rule *rule;
    dbt_rulepart *rulepart;
    dbt_ruletimer *ruletimer;
    dbt_timer *timer;
    dbt_action *action;
    dbt_user *user;
    dbt_group *group;
    dbt_groupitem *groupitem;

    char lm[50];

    switch (msg->tab_id)
    {
    case ITEM:
        item = find_element(&config_db.db_item, &config_db.db_item->id, false,
                            ITEM,
                            NUMBER, 0, msg->rec_id, "", N_UINT16);
        if (item)
        {
            do
            {
                action = find_element_and(&config_db.db_action, false, ACTION,
                                          &config_db.db_action->atype,
                                          NUMBER,
                                          0,
                                          A_ITEM, "",
                                          &config_db.db_action->aitem,
                                          NUMBER,
                                          0,
                                          msg->rec_id, "");
                if (action)
                {
                    db_delete_item(&action, &config_db.db_action,
                                   &config_db.db_action_last,
                                   ACTION);
                }
                rulepart = find_element(&config_db.db_rulepart,
                                        &config_db.db_rulepart->citem,
                                        false,
                                        RULEPART,
                                        NUMBER, 0, msg->rec_id, "",
                                        N_UINT16);
                if (rulepart)
                {
                    db_delete_item(&rulepart, &config_db.db_rulepart,
                                   &config_db.db_rulepart_last,
                                   RULEPART);
                }
                groupitem = find_element(&config_db.db_groupitem,
                                         &config_db.db_groupitem->item,
                                         false,
                                         GROUPITEM,
                                         NUMBER, 0, msg->rec_id, "",
                                         N_UINT16);
                if (groupitem)
                {
                    db_delete_item(&groupitem, &config_db.db_groupitem,
                                   &config_db.db_groupitem_last,
                                   GROUPITEM);
                }

            }
            while (action || rulepart || groupitem);

            return db_delete_item(&item, &config_db.db_item,
                                  &config_db.db_item_last,
                                  msg->tab_id);
        }
        break;
    case COMPONENT:
        component = find_element(&config_db.db_component,
                                 &config_db.db_component->id,
                                 false, COMPONENT,
                                 NUMBER,
                                 0, msg->rec_id, "", N_UINT16);
        if (component)
        {
            return db_delete_item(&component, &config_db.db_component,
                                  &config_db.db_component_last,
                                  msg->tab_id);
        }
        break;
    case UNIT:
        unit = find_element(&config_db.db_unit, &config_db.db_unit->id, false,
                            UNIT,
                            NUMBER, 0, msg->rec_id, "", N_UINT16);
        if (unit)
        {
            do
            {
                item = find_element(&config_db.db_item,
                                    &config_db.db_item->unit,
                                    false, ITEM,
                                    NUMBER,
                                    0, msg->rec_id, "", N_UINT8);
                if (item)
                {
                    do
                    {
                        action = find_element_and(&config_db.db_action, false,
                                                  ACTION,
                                                  &config_db.db_action->atype,
                                                  NUMBER,
                                                  0, A_ITEM, "",
                                                  &config_db.db_action->aitem,
                                                  NUMBER, 0, msg->rec_id, "");
                        if (action)
                        {
                            db_delete_item(&action, &config_db.db_action,
                                           &config_db.db_action_last,
                                           ACTION);
                        }
                        rulepart = find_element(&config_db.db_rulepart,
                                                &config_db.db_rulepart->citem,
                                                false,
                                                RULEPART,
                                                NUMBER, 0,
                                                msg->rec_id,
                                                "", N_UINT16);
                        if (rulepart)
                        {
                            db_delete_item(&rulepart, &config_db.db_rulepart,
                                           &config_db.db_rulepart_last,
                                           RULEPART);
                        }
                        groupitem = find_element(&config_db.db_groupitem,
                                                 &config_db.db_groupitem->item,
                                                 false,
                                                 GROUPITEM,
                                                 NUMBER, 0,
                                                 msg->rec_id,
                                                 "", N_UINT16);
                        if (groupitem)
                        {
                            db_delete_item(&groupitem, &config_db.db_groupitem,
                                           &config_db.db_groupitem_last,
                                           GROUPITEM);
                        }

                    }
                    while (action || rulepart || groupitem);

                    db_delete_item(&item, &config_db.db_item,
                                   &config_db.db_item_last,
                                   ITEM);
                }

            }
            while (item);
            return db_delete_item(&unit, &config_db.db_unit,
                                  &config_db.db_unit_last,
                                  msg->tab_id);
        }
        break;
    case RULE:
        rule = find_element(&config_db.db_rule, &config_db.db_rule->id, false,
                            RULE,
                            NUMBER, 0, msg->rec_id, "", N_UINT16);
        if (rule)
        {
            do
            {
                rulepart = find_element(&config_db.db_rulepart,
                                        &config_db.db_rulepart->rid,
                                        false,
                                        RULEPART,
                                        NUMBER, 0, msg->rec_id, "",
                                        N_UINT16);
                if (rulepart)
                {
                    db_delete_item(&rulepart, &config_db.db_rulepart,
                                   &config_db.db_rulepart_last,
                                   RULEPART);
                }

                ruletimer = find_element(&config_db.db_ruletimer,
                                         &config_db.db_ruletimer->rid,
                                         false,
                                         RULETIMER,
                                         NUMBER, 0, msg->rec_id, "",
                                         N_UINT16);
                if (ruletimer)
                {
                    db_delete_item(&ruletimer, &config_db.db_ruletimer,
                                   &config_db.db_ruletimer_last,
                                   RULETIMER);
                }
                action = find_element(&config_db.db_action,
                                      &config_db.db_action->rid,
                                      false, ACTION,
                                      NUMBER,
                                      0, msg->rec_id, "", N_UINT16);
                if (action)
                {
                    db_delete_item(&action, &config_db.db_action,
                                   &config_db.db_action_last,
                                   ACTION);
                }
                timer = find_element(&config_db.db_timer,
                                     &config_db.db_timer->rid,
                                     false, TIMER,
                                     NUMBER,
                                     0, msg->rec_id, "", N_UINT16);
                if (timer)
                {
                    db_delete_item(&timer, &config_db.db_timer,
                                   &config_db.db_timer_last,
                                   TIMER);
                }

            }
            while (rulepart || ruletimer || action || timer);

            sprintf(lm, "dbdelete: rule %d name %s", rule->id, rule->name);
            logevent(L_INFO, lm);

            return db_delete_item(&rule, &config_db.db_rule,
                                  &config_db.db_rule_last,
                                  msg->tab_id);
        }
        break;
    case RULEPART:
        rulepart = find_element(&config_db.db_rulepart,
                                &config_db.db_rulepart->id,
                                false, RULEPART,
                                NUMBER,
                                0, msg->rec_id, "", N_UINT16);
        if (rulepart)
        {
            sprintf(lm, "dbdelete: rulepart %d rule id %d scond %s",
                    rulepart->id,
                    rulepart->rid,
                    rulepart->scond ? "True" : "False");
            logevent(L_INFO, lm);

            return db_delete_item(&rulepart, &config_db.db_rulepart,
                                  &config_db.db_rulepart_last,
                                  msg->tab_id);
        }
        break;
    case RULETIMER:
        ruletimer = find_element(&config_db.db_ruletimer,
                                 &config_db.db_ruletimer->id,
                                 false, RULETIMER,
                                 NUMBER,
                                 0, msg->rec_id, "", N_UINT16);
        if (ruletimer)
        {
            sprintf(lm, "dbdelete: ruletimer %d rule id %d", ruletimer->id,
                    ruletimer->rid);
            logevent(L_INFO, lm);

            return db_delete_item(&ruletimer, &config_db.db_ruletimer,
                                  &config_db.db_ruletimer_last,
                                  msg->tab_id);
        }
        break;
    case ACTION:
        action = find_element(&config_db.db_action, &config_db.db_action->id,
        false,
                              ACTION, NUMBER, 0, msg->rec_id, "", N_UINT16);
        if (action)
        {
            sprintf(lm, "dbdelete: action %d rule id %d", action->id,
                    action->rid);
            logevent(L_INFO, lm);

            return db_delete_item(&action, &config_db.db_action,
                                  &config_db.db_action_last,
                                  msg->tab_id);
        }
        break;
    case TIMER:
        timer = find_element(&config_db.db_timer, &config_db.db_timer->id,
        false,
                             TIMER, NUMBER, 0, msg->rec_id, "", N_UINT16);
        if (timer)
        {
            sprintf(lm, "dbdelete: timer %d rule id %d", timer->id, timer->rid);
            logevent(L_INFO, lm);

            return db_delete_item(&timer, &config_db.db_timer,
                                  &config_db.db_timer_last,
                                  msg->tab_id);
        }
        break;
    case USER:
        user = find_element(&config_db.db_user, &config_db.db_user->id, false,
                            USER,
                            NUMBER, 0, msg->rec_id, "", N_UINT16);
        if (user)
        {
            return db_delete_item(&user, &config_db.db_user,
                                  &config_db.db_user_last,
                                  msg->tab_id);
        }
        break;
    case GROUP:
        group = find_element(&config_db.db_group, &config_db.db_group->id,
        false,
                             GROUP, NUMBER, 0, msg->rec_id, "", N_UINT16);
        if (group)
        {
            do
            {
                groupitem = find_element(&config_db.db_groupitem,
                                         &config_db.db_groupitem->group,
                                         false,
                                         GROUPITEM,
                                         NUMBER, 0, msg->rec_id, "",
                                         N_UINT16);
                if (groupitem)
                {
                    db_delete_item(&groupitem, &config_db.db_groupitem,
                                   &config_db.db_groupitem_last,
                                   GROUPITEM);
                }
                action = find_element_and(&config_db.db_action, false, ACTION,
                                          &config_db.db_action->atype,
                                          NUMBER,
                                          0,
                                          A_GROUP, "",
                                          &config_db.db_action->aitem,
                                          NUMBER,
                                          0,
                                          msg->rec_id, "");
                if (action)
                {
                    db_delete_item(&action, &config_db.db_action,
                                   &config_db.db_action_last,
                                   ACTION);
                }
            }
            while (groupitem || action);

            return db_delete_item(&group, &config_db.db_group,
                                  &config_db.db_group_last,
                                  msg->tab_id);
        }
        break;
    case GROUPITEM:
        groupitem = find_element(&config_db.db_groupitem,
                                 &config_db.db_groupitem->id,
                                 false, GROUPITEM,
                                 NUMBER,
                                 0, msg->rec_id, "", N_UINT16);
        if (groupitem)
        {
            return db_delete_item(&groupitem, &config_db.db_groupitem,
                                  &config_db.db_groupitem_last,
                                  msg->tab_id);
        }
        break;
    }

    return false;
}

void parse_dbset(msg_dbev_t *msg)
{
    int64_t i;
    uint16_t r;
    bool new = false;
    char *s = msg->entry;
    dbt_item *item = NULL;
    dbt_component *component = NULL;
    dbt_unit *unit = NULL;
    dbt_rule *rule = NULL;
    dbt_rulepart *rulepart = NULL;
    dbt_ruletimer *ruletimer = NULL;
    dbt_timer *timer = NULL;
    dbt_action *action = NULL;
    dbt_device *device = NULL;
    dbt_user *user = NULL;
    dbt_group *group = NULL;
    dbt_groupitem *groupitem = NULL;
    dbt_zdevice *zdevice = NULL;
    dbt_zendpoint *zendpoint = NULL;

    char lm[50];

    switch (msg->tab_id)
    {
    case ITEM:
        if (msg->rec_id < 65535)
        {
            item = find_element(&config_db.db_item, &config_db.db_item->id,
            false,
                                ITEM, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }

        if (item == NULL)
        {
            item = db_add_item(ITEM, &config_db.db_item,
                               &config_db.db_item_last);
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            item->unit = i;
            s += r;
        }
        else
        {
            item->unit = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            item->component = i;
            s += r;
        }
        else
        {
            item->component = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            item->port = i;
            s += r;
        }
        else
        {
            item->port = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            item->type = i;
            s += r;
        }
        else
        {
            item->type = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            item->icon = i;
            s += r;
        }
        else
        {
            item->icon = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            item->state = i;
            s += r;
        }
        else
        {
            item->state = 0;
        }

        r = get_next_dbset(s, &i, item->name);
        if (r)
        {
            s += r;
        }
        else
        {
            item->name[0] = 0;
        }
        set_element((dbt_generic**) &item, ITEM, true);
        break;
    case COMPONENT:
        if (msg->rec_id < 65535)
        {
            component = find_element((dbt_generic**) &config_db.db_component,
                                     (char**) &config_db.db_component->id,
                                     false,
                                     COMPONENT,
                                     NUMBER, 0, msg->rec_id, "",
                                     N_UINT16);
        }
        if (component == NULL)
        {
            component = db_add_item(COMPONENT, &config_db.db_component,
                                    &config_db.db_component_last);
        }

        r = get_next_dbset(s, &i, component->name);
        if (r)
        {
            s += r;
        }
        else
        {
            component->name[0] = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            component->type = i;
            s += r;
        }
        else
        {
            component->type = 0;
        }

        set_element((dbt_generic**) &component, COMPONENT, true);
        break;
    case UNIT:
        if (msg->rec_id < 65535)
        {
            unit = find_element((dbt_generic**) &config_db.db_unit, (char**) &config_db.db_unit->id, false, UNIT, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }
        if (unit == NULL)
        {
            unit = db_add_item(UNIT, (dbt_generic**) &config_db.db_unit, (dbt_generic**) &config_db.db_unit_last);
            new = true;
        }

        r = get_next_dbset(s, &i, unit->hwid);
        if (r)
        {
            s += r;
        }
        else
        {
            unit->hwid[0] = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            unit->type = i;
            s += r;
        }
        else
        {
            unit->type = 0;
        }

        // Skip VHW/VFW which are RO
        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            s += r;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            s += r;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            unit->acid = i;
            s += r;
        }
        else
        {
            unit->acid = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            unit->online = i;
            s += r;
        }
        else
        {
            unit->online = 0;
        }

        r = get_next_dbset(s, &i, unit->name);
        if (r)
        {
            s += r;
        }

        if (new)
        {
            sprintf(unit->name, "UNIT %d", unit->id);
        }

        set_element((dbt_generic**) &unit, UNIT, false);

        if (new)
        {
            create_items(unit->type, unit->id);

        }
        mem_free(unit, sizeof(dbt_unit));

        break;
    case RULE:
        if (msg->rec_id < 65535)
        {
            rule = find_element(&config_db.db_rule, &config_db.db_rule->id,
            false,
                                RULE, NUMBER, 0, msg->rec_id, "", N_UINT16);
            new = false;
        }
        if (rule == NULL)
        {
            rule = db_add_item(RULE, &config_db.db_rule,
                               &config_db.db_rule_last);
            new = true;
        }

        r = get_next_dbset(s, &i, rule->name);
        if (r)
        {
            s += r;
        }
        else
        {
            rule->name[0] = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            rule->enabled = i;
            s += r;
        }
        else
        {
            rule->enabled = 0;
        }

        if (new)
        {
            sprintf(lm, "dbset: new rule id %d name %s", rule->id, rule->name);
            logevent(L_INFO, lm);
        }
        else
        {
            sprintf(lm, "dbset: mod rule id %d name %s", rule->id, rule->name);
            logevent(L_INFO, lm);
        }

        set_element((dbt_generic**) &rule, RULE, true);
        break;
    case RULEPART:
        if (msg->rec_id < 65535)
        {
            rulepart = find_element(&config_db.db_rulepart,
                                    &config_db.db_rulepart->id,
                                    false, RULEPART,
                                    NUMBER,
                                    0, msg->rec_id, "", N_UINT16);
            new = false;
        }
        if (rulepart == NULL)
        {
            rulepart = db_add_item(RULEPART, &config_db.db_rulepart,
                                   &config_db.db_rulepart_last);
            new = true;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            rulepart->rid = i;
            s += r;
        }
        else
        {
            rulepart->rid = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            rulepart->scond = i;
            s += r;
        }
        else
        {
            rulepart->scond = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            rulepart->citem = i;
            s += r;
        }
        else
        {
            rulepart->citem = 0;
        }

        // Character
        if (s[0] == ',')
        {
            rulepart->cop = 0;
        }
        else
        {
            rulepart->cop = s[0];
            s += 2;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            rulepart->cvalue = i;
            s += r;
        }
        else
        {
            rulepart->cvalue = 0;
        }

        if (new)
        {
            sprintf(lm, "dbset: new rulepart id %d rule id %d scond %s",
                    rulepart->id,
                    rulepart->rid,
                    rulepart->scond ? "True" : "False");
            logevent(L_INFO, lm);
        }
        else
        {
            sprintf(lm, "dbset: mod rulepart id %d rule id %d scond %s",
                    rulepart->id,
                    rulepart->rid,
                    rulepart->scond ? "True" : "False");
            logevent(L_INFO, lm);
        }

        set_element((dbt_generic**) &rulepart, RULEPART, true);
        break;
    case RULETIMER:
        if (msg->rec_id < 65535)
        {
            ruletimer = find_element((dbt_generic**) &config_db.db_ruletimer,
                                     (char**) &config_db.db_ruletimer->id,
                                     false,
                                     RULETIMER,
                                     NUMBER, 0, msg->rec_id, "",
                                     N_UINT16);
            new = false;
        }
        if (ruletimer == NULL)
        {
            ruletimer = db_add_item(RULETIMER, &config_db.db_ruletimer,
                                    &config_db.db_ruletimer_last);
            new = true;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            ruletimer->rid = i;
            s += r;
        }
        else
        {
            ruletimer->rid = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            ruletimer->type = i;
            s += r;
        }
        else
        {
            ruletimer->type = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            ruletimer->day = i;
            s += r;
        }
        else
        {
            ruletimer->day = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            ruletimer->tstart = i;
            s += r;
        }
        else
        {
            ruletimer->tstart = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            ruletimer->tend = i;
            s += r;
        }
        else
        {
            ruletimer->tend = 0;
        }

        if (new)
        {
            sprintf(lm, "dbset: new ruletimer id %d rule id %d", ruletimer->id,
                    ruletimer->rid);
            logevent(L_INFO, lm);
        }
        else
        {
            sprintf(lm, "dbset: mod ruletimer id %d rule id %d", ruletimer->id,
                    ruletimer->rid);
            logevent(L_INFO, lm);
        }

        set_element((dbt_generic**) &ruletimer, RULETIMER, true);
        break;
    case ACTION:
        if (msg->rec_id < 65535)
        {
            action = find_element(&config_db.db_action,
                                  &config_db.db_action->id,
                                  false, ACTION,
                                  NUMBER,
                                  0, msg->rec_id, "", N_UINT16);
            new = false;
        }
        if (action == NULL)
        {
            action = db_add_item(ACTION, &config_db.db_action,
                                 &config_db.db_action_last);
            new = true;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            action->rid = i;
            s += r;
        }
        else
        {
            action->rid = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            action->atype = i;
            s += r;
        }
        else
        {
            action->atype = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            action->aitem = i;
            s += r;
        }
        else
        {
            action->aitem = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            action->aaction = i;
            s += r;
        }
        else
        {
            action->aaction = 0;
        }

        if (new)
        {
            sprintf(lm, "dbset: new action id %d rule id %d", action->id,
                    action->rid);
            logevent(L_INFO, lm);
        }
        else
        {
            sprintf(lm, "dbset: mod action id %d rule id %d", action->id,
                    action->rid);
            logevent(L_INFO, lm);
        }

        set_element((dbt_generic**) &action, ACTION, true);
        break;
    case TIMER:
        if (msg->rec_id < 65535)
        {
            timer = find_element(&config_db.db_timer, &config_db.db_timer->id,
            false,
                                 TIMER, NUMBER, 0, msg->rec_id, "", N_UINT16);
            new = false;
        }
        if (timer == NULL)
        {
            timer = db_add_item(TIMER, &config_db.db_timer,
                                &config_db.db_timer_last);
            new = true;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            timer->rid = i;
            s += r;
        }
        else
        {
            timer->rid = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            timer->type = i;
            s += r;
        }
        else
        {
            timer->type = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            timer->day = i;
            s += r;
        }
        else
        {
            timer->day = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            timer->tstart = i;
            s += r;
        }
        else
        {
            timer->tstart = 0;
        }

        if (new)
        {
            sprintf(lm, "dbset: new timer id %d rule id %d", timer->id,
                    timer->rid);
            logevent(L_INFO, lm);
        }
        else
        {
            sprintf(lm, "dbset: mod timer id %d rule id %d", timer->id,
                    timer->rid);
            logevent(L_INFO, lm);
        }

        set_element((dbt_generic**) &timer, TIMER, true);
        break;
    case DEVICE:
        device = get_element(&config_db.db_device, DEVICE);

        // MAC is RO
        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            s += r;
        }

        // idate is RO
        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            s += r;
        }

        r = get_next_dbset(s, &i, device->name);
        if (r)
        {
            s += r;
        }
        else
        {
            device->name[0] = 0;
        }

        r = get_next_dbset(s, &i, device->location);
        if (r)
        {
            s += r;
        }
        else
        {
            device->location[0] = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->lan_dhcp = i;
            s += r;
        }
        else
        {
            device->lan_dhcp = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->lan_ip = i;
            s += r;
        }
        else
        {
            device->lan_ip = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->lan_gw = i;
            s += r;
        }
        else
        {
            device->lan_gw = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->lan_dns = i;
            s += r;
        }
        else
        {
            device->lan_dns = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->lan_sn = i;
            s += r;
        }
        else
        {
            device->lan_sn = 0;
        }

        r = get_next_dbset(s, &i, device->time);
        if (r)
        {
            s += r;
        }
        else
        {
            sprintf(device->time, "0");
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->dst = i;
            s += r;
        }
        else
        {
            device->dst = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->timezone = i;
            s += r;
        }
        else
        {
            device->timezone = 0;
        }

        // VHW/VFW/VDB are RO
        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            s += r;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            s += r;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            s += r;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->loglev = i;
            s += r;
        }
        else
        {
            device->loglev = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->logudp = i;
            s += r;
        }
        else
        {
            device->logudp = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->acip = i;
            s += r;
        }
        else
        {
            device->acip = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            device->acp = i;
            s += r;
        }
        else
        {
            device->acp = 0;
        }

        set_element((dbt_generic**) &device, DEVICE, true);
        break;
    case USER:
        if (msg->rec_id < 65535)
        {
            user = find_element(&config_db.db_user, &config_db.db_user->id,
            false,
                                USER, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }

        if (user == NULL)
        {
            user = db_add_item(USER, &config_db.db_user,
                               &config_db.db_user_last);
        }

        r = get_next_dbset(s, &i, user->name);
        if (r)
        {
            s += r;
        }
        else
        {
            user->name[0] = 0;
        }

        r = get_next_dbset(s, &i, user->email);
        if (r)
        {
            s += r;
        }
        else
        {
            user->email[0] = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            user->pw = i;
            s += r;
        }
        else
        {
            user->pw = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            user->level = i;
            s += r;
        }
        else
        {
            user->level = 0;
        }

        set_element((dbt_generic**) &user, USER, true);
        break;
    case GROUP:
        if (msg->rec_id < 65535)
        {
            group = find_element(&config_db.db_group, &config_db.db_group->id,
            false,
                                 GROUP, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }
        if (group == NULL)
        {
            group = db_add_item(GROUP, &config_db.db_group,
                                &config_db.db_group_last);
        }

        r = get_next_dbset(s, &i, group->name);
        if (r)
        {
            s += r;
        }
        else
        {
            group->name[0] = 0;
        }
        set_element((dbt_generic**) &group, GROUP, true);
        break;
    case GROUPITEM:
        if (msg->rec_id < 65535)
        {
            groupitem = find_element(&config_db.db_groupitem,
                                     &config_db.db_groupitem->id,
                                     false,
                                     GROUPITEM,
                                     NUMBER, 0, msg->rec_id, "",
                                     N_UINT16);
        }
        if (groupitem == NULL)
        {
            groupitem = db_add_item(GROUPITEM, &config_db.db_groupitem,
                                    &config_db.db_groupitem_last);
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            groupitem->item = i;
            s += r;
        }
        else
        {
            groupitem->item = 0;
        }

        r = get_next_dbset(s, &i, NULL);
        if (r)
        {
            groupitem->group = i;
            s += r;
        }
        else
        {
            groupitem->group = 0;
        }

        set_element((dbt_generic**) &groupitem, GROUPITEM, true);
        break;
    }
}

void parse_dbget(msg_dbev_t *msg)
{
    uint16_t d;
    bool next = false;
    msg_dbev_t msend;
    msg_mqtt_t msg_mqtt;
    dbt_item *item = NULL;
    dbt_component *component = NULL;
    dbt_unit *unit = NULL;
    dbt_rule *rule = NULL;
    dbt_rulepart *rulepart = NULL;
    dbt_ruletimer *ruletimer = NULL;
    dbt_timer *timer = NULL;
    dbt_action *action = NULL;
    dbt_device *device = NULL;
    dbt_user *user = NULL;
    dbt_group *group = NULL;
    dbt_groupitem *groupitem = NULL;
    dbt_zdevice *zdevice = NULL;
    dbt_zendpoint *zendpoint = NULL;
    void *r;

    char mac[18];

    db_get_mac(mac);

    msend.tab_id = msg->tab_id;
    msend.rec_id = msg->rec_id;
    msend.s = msg->s;

    switch (msg->tab_id)
    {
    case ITEM:
        d = sizeof(dbt_item) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            item = get_element(&config_db.db_item, ITEM);
            next = true;
        }
        else
        {
            item = find_element(&config_db.db_item, &config_db.db_item->id,
            false,
                                ITEM, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }

        while (item)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry,
                    "%d,%d,%d,%hu,%hhu,%hu,%hhu,%hhu,%hhu,%hhu,%s###",
                    AM_DBGET,
                    ITEM,
                    get_tversion(ITEM), item->id, item->unit,
                    item->component,
                    item->port, item->type, item->icon,
                    item->state,
                    item->name);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = item->element->next;
                mem_free(item, sizeof(dbt_item));
                item = get_element(&r, ITEM);
            }
            else
            {
                mem_free(item, sizeof(dbt_item));
                item = NULL;
            }
        }
        break;
    case COMPONENT:
        d = sizeof(dbt_component) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            component = get_element(&config_db.db_component, COMPONENT);
            next = true;
        }
        else
        {
            component = find_element(&config_db.db_component,
                                     &config_db.db_component->id,
                                     false,
                                     COMPONENT,
                                     NUMBER, 0, msg->rec_id, "",
                                     N_UINT16);
        }

        while (component)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hu,%s,%hhu###", AM_DBGET, COMPONENT,
                    get_tversion(COMPONENT),
                    component->id, component->name,
                    component->type);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = component->element->next;
                mem_free(component, sizeof(dbt_component));
                component = get_element(&r, COMPONENT);
            }
            else
            {
                mem_free(component, sizeof(dbt_component));
                component = NULL;
            }
        }
        break;
    case UNIT:
        d = sizeof(dbt_unit) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            unit = get_element(&config_db.db_unit, UNIT);
            next = true;
        }
        else
        {
            unit = find_element(&config_db.db_unit, &config_db.db_unit->id,
            false,
                                UNIT, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }

        while (unit)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry,
                    "%d,%d,%d,%hhu,%s,%hhu,%hhu,%hhu,%hhu,%hhd,%s###",
                    AM_DBGET,
                    UNIT,
                    get_tversion(UNIT), unit->id, unit->hwid, unit->type,
                    unit->vhw,
                    unit->vfw, unit->acid, unit->online, unit->name);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = unit->element->next;
                mem_free(unit, sizeof(dbt_unit));
                unit = get_element(&r, UNIT);
            }
            else
            {
                mem_free(unit, sizeof(dbt_unit));
                unit = NULL;
            }
        }
        break;
    case RULE:
        d = sizeof(dbt_rule) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            rule = get_element(&config_db.db_rule, RULE);
            next = true;
        }
        else
        {
            rule = find_element(&config_db.db_rule, &config_db.db_rule->id,
            false,
                                RULE, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }
        while (rule)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hu,%s,%hhu###", AM_DBGET, RULE,
                    get_tversion(RULE),
                    rule->id, rule->name, rule->enabled);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);

            if (next)
            {
                r = rule->element->next;
                mem_free(rule, sizeof(dbt_rule));
                rule = get_element(&r, RULE);
            }
            else
            {
                mem_free(rule, sizeof(dbt_rule));
                rule = NULL;
            }
        }
        break;
    case RULEPART:
        d = sizeof(dbt_rulepart) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            rulepart = get_element(&config_db.db_rulepart, RULEPART);
            next = true;
        }
        else
        {
            rulepart = find_element(&config_db.db_rulepart,
                                    &config_db.db_rulepart->id,
                                    false, RULEPART,
                                    NUMBER,
                                    0, msg->rec_id, "", N_UINT16);
        }

        while (rulepart)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hu,%hu,%hhu,%hu,%c,%hu###",
                    AM_DBGET,
                    RULEPART, get_tversion(RULEPART), rulepart->id,
                    rulepart->rid,
                    rulepart->scond, rulepart->citem,
                    rulepart->cop,
                    rulepart->cvalue);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = rulepart->element->next;
                mem_free(rulepart, sizeof(dbt_rulepart));
                rulepart = get_element(&r, RULEPART);
            }
            else
            {
                mem_free(rulepart, sizeof(dbt_rulepart));
                rulepart = NULL;
            }
        }
        break;
    case RULETIMER:
        d = sizeof(dbt_ruletimer) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            ruletimer = get_element(&config_db.db_ruletimer, RULETIMER);
            next = true;
        }
        else
        {
            ruletimer = find_element(&config_db.db_ruletimer,
                                     &config_db.db_ruletimer->id,
                                     false,
                                     RULETIMER,
                                     NUMBER, 0, msg->rec_id, "",
                                     N_UINT16);
        }

        while (ruletimer)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hu,%hu,%hhu,%u,%hu,%hu###",
                    AM_DBGET,
                    RULETIMER, get_tversion(RULETIMER), ruletimer->id,
                    ruletimer->rid,
                    ruletimer->type, ruletimer->day,
                    ruletimer->tstart,
                    ruletimer->tend);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = ruletimer->element->next;
                mem_free(ruletimer, sizeof(dbt_ruletimer));
                ruletimer = get_element(&r, RULETIMER);
            }
            else
            {
                mem_free(ruletimer, sizeof(dbt_ruletimer));
                ruletimer = NULL;
            }
        }
        break;
    case ACTION:
        d = sizeof(dbt_action) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            action = get_element(&config_db.db_action, ACTION);
            next = true;
        }
        else
        {
            action = find_element(&config_db.db_action,
                                  &config_db.db_action->id,
                                  false, ACTION,
                                  NUMBER,
                                  0, msg->rec_id, "", N_UINT16);
        }

        while (action)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hu,%hu,%hhu,%hu,%hhu###", AM_DBGET,
                    ACTION,
                    get_tversion(ACTION), action->id, action->rid,
                    action->atype,
                    action->aitem, action->aaction);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = action->element->next;
                mem_free(action, sizeof(dbt_action));
                action = get_element(&r, ACTION);
            }
            else
            {
                mem_free(action, sizeof(dbt_action));
                action = NULL;
            }
        }
        break;
    case TIMER:
        d = sizeof(dbt_timer) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            timer = get_element(&config_db.db_timer, TIMER);
            next = true;
        }
        else
        {
            timer = find_element(&config_db.db_timer, &config_db.db_timer->id,
            false,
                                 TIMER, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }

        while (timer)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hu,%hu,%hhu,%u,%hu###", AM_DBGET,
                    TIMER,
                    get_tversion(TIMER), timer->id, timer->rid,
                    timer->type,
                    timer->day, timer->tstart);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = timer->element->next;
                mem_free(timer, sizeof(dbt_timer));
                timer = get_element(&r, TIMER);
            }
            else
            {
                mem_free(timer, sizeof(dbt_timer));
                timer = NULL;
            }
        }
        break;
    case DEVICE:
        d = sizeof(dbt_device) + DBEV_OVERHEAD;
        msend.esize = d;
        msend.entry = mem_alloc(d);
        device = get_element(&config_db.db_device, DEVICE);
        sprintf(msend.entry,
                "%d,%d,%d,%s,%u,%s,%s,%hhu,%u,%u,%u,%u,%u,%u,%u,%u,%s,%hhu,%hhd,%hu,%hu,%hu,%hhd,%hu,%u,%hu###",
                AM_DBGET, DEVICE, get_tversion(DEVICE), device->mac,
                device->idate,
                device->name, device->location, device->lan_dhcp,
                device->lan_ip,
                device->lan_gw, device->lan_dns, device->lan_sn,
                device->cur_ip,
                device->cur_gw, device->cur_dns, device->cur_sn,
                device->time,
                device->dst, device->timezone, device->vhw,
                device->vfw,
                device->vdb, device->loglev, device->logudp,
                device->acip,
                device->acp);
        //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
        mem_free(device, sizeof(dbt_device));
        break;
    case USER:
        d = sizeof(dbt_user) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            user = get_element(&config_db.db_user, USER);
            next = true;
        }
        else
        {
            user = find_element(&config_db.db_user, &config_db.db_user->id,
            false,
                                USER, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }

        while (user)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hu,%s,%s,%d,%hhu###", AM_DBGET,
                    USER,
                    get_tversion(USER), user->id, user->name, user->email,
                    user->pw,
                    user->level);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = user->element->next;
                mem_free(user, sizeof(dbt_user));
                user = get_element(&r, USER);
            }
            else
            {
                mem_free(user, sizeof(dbt_user));
                user = NULL;
            }
        }
        break;
    case GROUP:

        d = sizeof(dbt_group) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            group = get_element(&config_db.db_group, GROUP);
            next = true;
        }
        else
        {
            group = find_element(&config_db.db_group, &config_db.db_group->id, false, GROUP, NUMBER, 0, msg->rec_id, "", N_UINT16);
        }

        while (group)
        {

            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hd,%s###", AM_DBGET, GROUP,
                    get_tversion(GROUP),
                    group->id, group->name);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = group->element->next;
                mem_free(group, sizeof(dbt_group));
                group = get_element(&r, GROUP);
            }
            else
            {
                mem_free(group, sizeof(dbt_group));
                group = NULL;
            }
        }
        break;
    case GROUPITEM:
        d = sizeof(dbt_groupitem) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            groupitem = get_element(&config_db.db_groupitem, GROUPITEM);
            next = true;
        }
        else
        {
            groupitem = find_element(&config_db.db_groupitem,
                                     &config_db.db_groupitem->id,
                                     false,
                                     GROUPITEM,
                                     NUMBER, 0, msg->rec_id, "",
                                     N_UINT16);
        }

        while (groupitem)
        {
            msend.entry = mem_alloc(d);
            sprintf(msend.entry, "%d,%d,%d,%hu,%hu,%hu###", AM_DBGET, GROUPITEM,
                    get_tversion(GROUPITEM),
                    groupitem->id, groupitem->item,
                    groupitem->group);
            //Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
            if (next)
            {
                r = groupitem->element->next;
                mem_free(groupitem, sizeof(dbt_groupitem));
                groupitem = get_element(&r, GROUPITEM);
            }
            else
            {
                mem_free(groupitem, sizeof(dbt_groupitem));
                groupitem = NULL;
            }
        }
        break;
    case ZDEVICE:
        d = sizeof(dbt_zdevice) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            zdevice = get_element(&config_db.db_zdevice, ZDEVICE);
            next = true;
        }
        else
        {
            zdevice = find_element(&config_db.db_zdevice,
                                   &config_db.db_zdevice->id,
                                   false, ZDEVICE,
                                   NUMBER,
                                   0, msg->rec_id, "", N_UINT16);
        }

        while (zdevice)
        {
            /*
             msend.entry = mem_alloc(d);
             sprintf(msend.entry, "%d,%d,%d,%hu,%hu,%d,%hhu,%hhu,%s,%s,%hhu,%s###",
             AM_DBGET,
             ZDEVICE, get_tversion(ZDEVICE),
             zdevice->id,
             zdevice->nwk_address,
             zdevice->status,
             zdevice->mfg_id,
             zdevice->ep_count,
             zdevice->ieee_address,
             zdevice->parent_address,
             zdevice->cap_info,
             zdevice->model_id);
             Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
             */

            msg_mqtt.size_content = 150;
            msg_mqtt.content = mem_alloc(msg_mqtt.size_content);
            sprintf(msg_mqtt.content, "%d,%s,%s,%hu,%hu,%hhu,%hhu,%s,%hhu",
                    zdevice->id,
                    zdevice->ieee_address,
                    zdevice->model_id,
                    zdevice->nwk_address,
                    zdevice->status,
                    zdevice->mfg_id,
                    zdevice->ep_count,
                    zdevice->parent_address,
                    zdevice->cap_info
                    );

            msg_mqtt.size_topic = 50;
            msg_mqtt.topic = mem_alloc(msg_mqtt.size_topic);
            sprintf(msg_mqtt.topic, "smaway/%s/tocloud/config/ZDEVICES\0", mac);

            sendToMqttCheckingMailboxNoWait(&msg_mqtt);
            //Mailbox_post(m_mqtt_send, &msg_mqtt, BIOS_NO_WAIT);

            if (next)
            {
                r = zdevice->element->next;
                mem_free(zdevice, sizeof(dbt_zdevice));
                zdevice = get_element(&r, ZDEVICE);
                Task_sleep(500);
            }
            else
            {
                mem_free(zdevice, sizeof(dbt_zdevice));
                zdevice = NULL;
            }
        }
        break;
    case ZENDPOINT:
        d = sizeof(dbt_zendpoint) + DBEV_OVERHEAD;
        msend.esize = d;
        if (msg->rec_id == 65535)
        {
            zendpoint = get_element(&config_db.db_zendpoint, ZENDPOINT);
            next = true;
        }
        else
        {
            zendpoint = find_element(&config_db.db_zendpoint,
                                     &config_db.db_zendpoint->id,
                                     false,
                                     ZENDPOINT,
                                     NUMBER, 0, msg->rec_id, "",
                                     N_UINT16);
        }

        while (zendpoint)
        {
            /*
             msend.entry = mem_alloc(d);
             sprintf(msend.entry, "%d,%d,%d,%hu,%hu,%hhu,%hu,%hu,%hhu,%hhu###",
             AM_DBGET,
             ZENDPOINT, get_tversion(GROUPITEM), zendpoint->id,
             zendpoint->zdevice_id,
             zendpoint->zendpoint_id,
             zendpoint->profile_id,
             zendpoint->device_id,
             zendpoint->device_version,
             zendpoint->num_in_cluster);
             Mailbox_post(m_lappsend, &msend, BIOS_WAIT_FOREVER);
             */

            msg_mqtt.size_content = d;
            msg_mqtt.content = mem_alloc(msg_mqtt.size_content);
            sprintf(msg_mqtt.content, "%hu,%hu,%hhu,%hu,%hu,%hhu,%hhu",
                    zendpoint->id,
                    zendpoint->zdevice_id,
                    zendpoint->zendpoint_id,
                    zendpoint->profile_id,
                    zendpoint->device_id,
                    zendpoint->device_version,
                    zendpoint->num_in_cluster);
            uint8_t i = 0;
            for (i = 0; i < zendpoint->num_in_cluster; i++)
            {
                sprintf(msg_mqtt.content, "%s,%hhu", msg_mqtt.content, zendpoint->clusters[i]);
            }

            msg_mqtt.size_topic = 50;
            msg_mqtt.topic = mem_alloc(msg_mqtt.size_topic);
            sprintf(msg_mqtt.topic, "smaway/%s/tocloud/config/ZENDPOINTS\0", mac);

            sendToMqttCheckingMailboxNoWait(&msg_mqtt);
//            Mailbox_post(m_mqtt_send, &msg_mqtt, BIOS_NO_WAIT);

            if (next)
            {
                r = zendpoint->element->next;
                mem_free(zendpoint, sizeof(dbt_zendpoint));
                zendpoint = get_element(&r, ZENDPOINT);
            }
            else
            {
                mem_free(zendpoint, sizeof(dbt_zendpoint));
                zendpoint = NULL;
            }
        }
        break;
    }
}

uint16_t get_next_dbset(char *s, int64_t *i, char *d)
{
    char *p;
    uint16_t a;

    if (s[0] == ',')
    {
        // Empty value
        if (d)
        {
            d[0] = 0;
        }
        else
        {
            *i = 0;
        }
        return 1;
    }
    else
    {
        p = strchr(s, ',');
        if (p == NULL)
        {
            // Reached end?
            p = strchr(s, '#');
            if (p == NULL)
            {
                p = &(s[strlen(s)]);
            }
        }
        if (p == NULL)
        {
            return 0;
        }

        else
        {
            a = p - s;
            if (d)
            {
                strncpy(d, s, a);
                d[a] = 0;
            }
            else
            {
                *i = atoi(s);
            }
            return a + 1;
        }
    }
}

void set_app_connected(bool y)
{
    Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);
    app_connected = y;
    Semaphore_post(sem_db_rw);
}

void db_get_mac(char *mac)
{
    dbt_device *dbd;
    dbd = get_element(&config_db.db_device, DEVICE);
    sprintf(mac, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c", dbd->mac[0], dbd->mac[1], dbd->mac[2], dbd->mac[3], dbd->mac[4], dbd->mac[5], dbd->mac[6], dbd->mac[7], dbd->mac[8], dbd->mac[9], dbd->mac[10],
            dbd->mac[11]);
    mem_free(dbd, sizeof(dbt_device));
}

bool db_get_db_ready()
{
    return db_ready;
}
