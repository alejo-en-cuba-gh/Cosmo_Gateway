/*
 * config-fileops.c
 *
 *  Created on: 17 dic 2023
 */

#include "common.h"
#include "config-fileops.h"
#include "config-db.h"
#include <xdc/runtime/System.h>

uint8_t get_next_value(char *s, long long *i, char *d, char delimiter);
uint32_t size_file(const char *file_name);
uint32_t db_migration_2to3(char **buffer, uint32_t file_size);
uint32_t db_migration_3to4(char **buffer, uint32_t file_size);

// A function that takes a pointer to a dbt_db structure and a file name as parameters
// and writes the contents of each substructure in one line of a text file
uint8_t export_config(dbt_db *db, char *file_name)
{
    // Declare pointers to each substructure type
    dbt_item *item;
    dbt_component *component;
    dbt_unit *unit;
    dbt_rule *rule;
    dbt_rulepart *rulepart;
    dbt_ruletimer *ruletimer;
    dbt_timer *timer;
    dbt_action *action;
    dbt_device *device;
    dbt_user *user;
    dbt_group *group;
    dbt_groupitem *groupitem;
    dbt_zdevice *zdevice;
    dbt_zendpoint *zendpoint;
    uint8_t i = 0;

    spiffs_file fd;
    size_t size_needed = 1;
    char *str = malloc(size_needed);
    str[0] = '\0';

    if (SPIFFS_mounted(&fs_spiffs) == FALSE)
    {
        free(str);
        return 0;
    }

    SPIFFS_remove(&fs_spiffs, file_name);

    // Open the file in write mode
    fd = SPIFFS_open(&fs_spiffs, file_name, SPIFFS_CREAT | SPIFFS_WRONLY, 0);
    if (fd < 0)
    {
        //printf("Error: cannot open file %s\n", file_name);
        free(str);
        return 0;
    }

    // Write the contents of each substructure in one line of the file
    // Use the substructure name followed by ':' as the prefix
    // Use ',' to separate the fields and ';' to separate the elements of the linked list
    // Use '\n' to end each line

    // Get semaphore for reading from DB
    sem_db_start_read();

    // Write the DB version first
    format_and_resize_string(fd, &str,&size_needed, "v%hu;\n", vdb);

    // Write the contents of db_item
    format_and_resize_string(fd, &str,&size_needed, "db_item:");
    item = db->db_item; // Get the first element of the linked list
    while (item != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current item
        format_and_resize_string(fd, &str,&size_needed, "%hu,%hhu,%hu,%hhu,%hhu,%hhu,%hhu,!%s;", item->id,
                                 item->unit,
                                 item->component, item->port, item->type, item->icon,
                                 item->state,
                                 item->name);
        item = item->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");
    // Write the contents of db_component
    format_and_resize_string(fd, &str,&size_needed, "db_component:");
    component = db->db_component; // Get the first element of the linked list
    while (component != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current component
        format_and_resize_string(fd, &str,&size_needed, "%hu,!%s,%hhu;", component->id, component->name,
                                 component->type);
        component = component->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");
    // Write the contents of db_unit
    format_and_resize_string(fd, &str,&size_needed, "db_unit:");
    unit = db->db_unit; // Get the first element of the linked list
    while (unit != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current unit
        format_and_resize_string(fd, &str,&size_needed, "%hhu,!%s,%hhu,%hhu,%hhu,%hhu,!%s;", unit->id, unit->hwid,
                                 unit->type,
                                 unit->vhw, unit->vfw, unit->acid, unit->name);
        unit = unit->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");
    // Write the contents of db_rule
    format_and_resize_string(fd, &str,&size_needed, "db_rule:");
    rule = db->db_rule; // Get the first element of the linked list
    while (rule != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current rule
        format_and_resize_string(fd, &str,&size_needed, "%hu,!%s,%hhu;", rule->id, rule->name, rule->enabled);
        rule = rule->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");
    // Write the contents of db_rulepart
    format_and_resize_string(fd, &str,&size_needed, "db_rulepart:");
    rulepart = db->db_rulepart; // Get the first element of the linked list
    while (rulepart != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current rulepart
        format_and_resize_string(fd, &str,&size_needed, "%hu,%hu,%hhu,%hu,%c,%hu;", rulepart->id, rulepart->rid,
                                 rulepart->scond,
                                 rulepart->citem, rulepart->cop,
                                 rulepart->cvalue);
        rulepart = rulepart->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");
    // Write the contents of db_ruletimer
    format_and_resize_string(fd, &str,&size_needed, "db_ruletimer:");
    ruletimer = db->db_ruletimer; // Get the first element of the linked list
    while (ruletimer != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current ruletimer
        format_and_resize_string(fd, &str,&size_needed, "%hu,%hu,%hhu,%u,%hu,%hu;", ruletimer->id, ruletimer->rid,
                                 ruletimer->type,
                                 ruletimer->day, ruletimer->tstart,
                                 ruletimer->tend);
        ruletimer = ruletimer->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");
    // Write the contents of db_timer
    format_and_resize_string(fd, &str,&size_needed, "db_timer:");
    timer = db->db_timer; // Get the first element of the linked list
    while (timer != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current timer
        format_and_resize_string(fd, &str,&size_needed, "%hu,%hu,%hhu,%u,%hu;", timer->id, timer->rid, timer->type,
                                 timer->day,
                                 timer->tstart);
        timer = timer->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");
    // Write the contents of db_action
    format_and_resize_string(fd, &str,&size_needed, "db_action:");
    action = db->db_action; // Get the first element of the linked list
    while (action != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current timer
        format_and_resize_string(fd, &str,&size_needed, "%hu,%hu,%hhu,%hu,%hhu;", action->id, action->rid,
                                 action->atype,
                                 action->aitem, action->aaction);
        action = action->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");

    // Write the contents of db_device
    format_and_resize_string(fd, &str,&size_needed, "db_device:");
    device = db->db_device; // Get the first element of the linked list
    format_and_resize_string(fd, &str,&size_needed, "!%s,%u,!%s,!%s,%hhu,%u,%u,%u,%u,%u,%u,%u,%u,!%s,%hhu,%hhd,%hu,%hu,%hu,%hhd,%hu,%u,%hu;",
                             device->mac,
                             device->idate, device->name, device->location,
                             device->lan_dhcp,
                             device->lan_ip, device->lan_gw, device->lan_dns,
                             device->lan_sn,
                             device->cur_ip, device->cur_gw, device->cur_dns,
                             device->cur_sn,
                             device->time, device->dst, device->timezone,
                             device->vhw,
                             device->vfw, device->vdb, device->loglev,
                             device->logudp,
                             device->acip, device->acp);
    format_and_resize_string(fd, &str,&size_needed, "\n");

    // Write the contents of db_user
    format_and_resize_string(fd, &str,&size_needed, "db_user:");
    user = db->db_user; // Get the first element of the linked list
    while (user != NULL)
    { // Loop until the end of the list
      // Write the fields of the current user
        format_and_resize_string(fd, &str,&size_needed, "%hu,!%s,!%s,%d,%hhu;", user->id, user->name, user->email,
                                 user->pw,
                                 user->level);
        user = user->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");

    // Write the contents of db_group
    format_and_resize_string(fd, &str,&size_needed, "db_group:");
    group = db->db_group; // Get the first element of the linked list
    while (group != NULL)
    { // Loop until the end of the list
      // Write the fields of the current group
        format_and_resize_string(fd, &str,&size_needed, "%hd,!%s;", group->id, group->name);
        group = group->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");

    // Write the contents of db_groupitem
    format_and_resize_string(fd, &str,&size_needed, "db_groupitem:");
    groupitem = db->db_groupitem; // Get the first element of the linked list
    while (groupitem != NULL)
    { // Loop until the end of the list
      // Write the fields of the current groupitem
        format_and_resize_string(fd, &str,&size_needed, "%hu,%hu,%hu;", groupitem->id, groupitem->group,
                                 groupitem->item);
        groupitem = groupitem->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");
    // Write the contents of db_unit
    format_and_resize_string(fd, &str,&size_needed, "db_zdevice:");
    zdevice = db->db_zdevice; // Get the first element of the linked list
    char ieee[24];
    while (zdevice != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current unit
        format_and_resize_string(fd, &str,&size_needed, "%hu,!%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X,!%s,%hu,%hhu,%hhu,!%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X,%u,%hu;",
                                    zdevice->id,
                                    BREAK_UINT64(zdevice->ieee_address, 7),
                                    BREAK_UINT64(zdevice->ieee_address, 6),
                                    BREAK_UINT64(zdevice->ieee_address, 5),
                                    BREAK_UINT64(zdevice->ieee_address, 4),
                                    BREAK_UINT64(zdevice->ieee_address, 3),
                                    BREAK_UINT64(zdevice->ieee_address, 2),
                                    BREAK_UINT64(zdevice->ieee_address, 1),
                                    BREAK_UINT64(zdevice->ieee_address, 0),
                                    zdevice->model_id, zdevice->nwk_address, zdevice->mfg_id,
                                    zdevice->ep_count,
                                    BREAK_UINT64(zdevice->parent_address, 7),
                                    BREAK_UINT64(zdevice->parent_address, 6),
                                    BREAK_UINT64(zdevice->parent_address, 5),
                                    BREAK_UINT64(zdevice->parent_address, 4),
                                    BREAK_UINT64(zdevice->parent_address, 3),
                                    BREAK_UINT64(zdevice->parent_address, 2),
                                    BREAK_UINT64(zdevice->parent_address, 1),
                                    BREAK_UINT64(zdevice->parent_address, 0),
                                    zdevice->cap_info, zdevice->firmware_version);
        zdevice = zdevice->next; // Move to the next element
    }
    format_and_resize_string(fd, &str,&size_needed, "\n");

    // Write the contents of db_unit
    format_and_resize_string(fd, &str,&size_needed, "db_zendpoint:");
    zendpoint = db->db_zendpoint; // Get the first element of the linked list
    while (zendpoint != NULL)
    {   // Loop until the end of the list
        // Write the fields of the current unit
        format_and_resize_string(fd, &str,&size_needed, "%hu,%hu,%hu,%hu,%hu,%hhu,%hhu,",
                                 zendpoint->id,
                                 zendpoint->zdevice_id, zendpoint->zendpoint_id,
                                 zendpoint->profile_id,
                                 zendpoint->device_id, zendpoint->device_version,
                                 zendpoint->num_in_cluster);
        for (i = 0; i < zendpoint->num_in_cluster; i++)
        {
            format_and_resize_string(fd, &str,&size_needed, "%hu_", zendpoint->clusters[i]);
        }
        format_and_resize_string(fd, &str,&size_needed, ";");
        zendpoint = zendpoint->next; // Move to the next element
    }

// Release semaphore for reading from DB
    sem_db_end_read();

    format_and_resize_string(fd, &str,&size_needed, "\nEND_CONFIG");

    if (SPIFFS_write(&fs_spiffs, fd, str, strlen(str)) < 0)
    {
        System_printf("Error writing to the file.\n");
        System_flush();
        return 0;
    }
    free(str);

// Close the file
    SPIFFS_close(&fs_spiffs, fd);
    return 1;
}

// Declare a buffer to store each line of the file
uint8_t import_config(dbt_db *db, char *file_name)
{
// Declare a buffer to store each line of the file
    char ieee_address[24];
    uint32_t bufsize = 0;
    int64_t i;
    uint16_t r = 0;
    uint8_t vdb_file = 0, j = 0;
    char *buffer, *type, *buffer_ptr;

    spiffs_file fd;

    if (SPIFFS_mounted(&fs_spiffs) == FALSE)
    {
        return 0;
    }

    // Open the file in read mode
    fd = SPIFFS_open(&fs_spiffs, file_name, SPIFFS_RDONLY, 0);
    // Check if the file is opened successfully
    if (fd < 0)
    {
        //printf("Error: cannot open file %s\n", file_name);
        return 0;
    }

    bufsize = get_fileSize(file_name);

// Get memory for buffer
    buffer = mem_alloc(bufsize);
    type = mem_alloc(50);
    if (buffer == NULL)
    {
        return 0;
    }

//copy all data in file
    if (SPIFFS_read(&fs_spiffs, fd, buffer, bufsize) < 0)
    {
        SPIFFS_close(&fs_spiffs, fd);
        return 0;
    }

    buffer[bufsize] = '\0';
    buffer_ptr = buffer;
    SPIFFS_close(&fs_spiffs, fd);

// Check if it is 'v'
    if (buffer_ptr[0] == 'v')
    {
        r = get_next_value(++buffer_ptr, &i, NULL, ';');
        if (r)
        {
            vdb_file = i;
            buffer_ptr += r;
        }
        buffer_ptr++; //REMOVING '\n'

        // Get semaphore for writing to DB
        Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);

        if ((vdb_file == 0) || (vdb_file > vdb))
        {
            // Invalid config version
            Semaphore_post(sem_db_rw);
            return 0;
        }

        // Erase current database
        empty_table((dbt_generic**) &db->db_groupitem, (dbt_generic**) &db->db_groupitem_last, GROUPITEM, false);
        empty_table((dbt_generic**) &db->db_group, (dbt_generic**) &db->db_group_last, GROUP, false);
        empty_table((dbt_generic**) &db->db_item, (dbt_generic**) &db->db_item_last, ITEM, false);
        empty_table((dbt_generic**) &db->db_rulepart, (dbt_generic**) &db->db_rulepart_last, RULEPART, false);
        empty_table((dbt_generic**) &db->db_ruletimer, (dbt_generic**) &db->db_ruletimer_last, RULETIMER, false);
        empty_table((dbt_generic**) &db->db_rule, (dbt_generic**) &db->db_rule_last, RULE, false);
        empty_table((dbt_generic**) &db->db_timer, (dbt_generic**) &db->db_timer_last, TIMER, false);
        empty_table((dbt_generic**) &db->db_action, (dbt_generic**) &db->db_action_last, ACTION, false);
        empty_table((dbt_generic**) &db->db_unit, (dbt_generic**) &db->db_unit_last, UNIT, false);
        empty_table((dbt_generic**) &db->db_user, (dbt_generic**) &db->db_user_last, USER, false);

        // In second run only receive for remaining space of buffer
        if (vdb_file < vdb)
        {

            if (vdb_file < 3)
            {
                bufsize = db_migration_2to3(&buffer, bufsize);
                buffer_ptr = buffer;
                r = get_next_value(++buffer_ptr, &i, NULL, ';');
                if (r)
                {
                    vdb_file = i;
                    buffer_ptr += r;
                }
                buffer_ptr++;
            }
            if (vdb_file < 4)
            {
                bufsize = db_migration_3to4(&buffer, bufsize);
                buffer_ptr = buffer;
                r = get_next_value(++buffer_ptr, &i, NULL, ';');
                if (r)
                {
                    vdb_file = i;
                    buffer_ptr += r;
                }
                buffer_ptr++;
            }
        }
        while (1)
        {
            r = get_next_value(buffer_ptr, &i, type, ':');
            if (r)
            {
                buffer_ptr += r;
            }

            if (strcmp(type, "db_item") == 0)
            {

                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_item), (dbt_generic**) &db->db_item, (dbt_generic**) &db->db_item_last);

                        //ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_item_last->id = i;
                            buffer_ptr += r;
                        }

                        //unit
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_item_last->unit = i;
                            buffer_ptr += r;
                        }

                        //component
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_item_last->component = i;
                            buffer_ptr += r;
                        }

                        //port
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_item_last->port = i;
                            buffer_ptr += r;
                        }

                        //type
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_item_last->type = i;
                            buffer_ptr += r;
                        }

                        //icon
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_item_last->icon = i;
                            buffer_ptr += r;
                        }

                        //state
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_item_last->state = i;
                            buffer_ptr += r;
                        }

                        //name
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_item_last->name,
                                           ';');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_component") == 0)
            {

                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_component), (dbt_generic**) &db->db_component, (dbt_generic**) &db->db_component_last);

                        //ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_component_last->id = i;
                            buffer_ptr += r;
                        }

                        //NAME
                        r = get_next_value(buffer_ptr, &i,
                                           db->db_component_last->name,
                                           ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }

                        //TYPE
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_component_last->type = i;
                            buffer_ptr += r;
                        }

                    }
                    else
                    {
                        break;
                    }
                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_unit") == 0)
            {
                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_unit), (dbt_generic**) &db->db_unit,
                                        (dbt_generic**) &db->db_unit_last);
                        //ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_unit_last->id = i;

                            //ONLINE
                            if (i == 1)
                            {
                                // Set unit 1 online
                                db->db_unit_last->online = 1;
                            }
                            else if (i > 1)
                            {
                                // Set all units offline at start except unit 1
                                db->db_unit_last->online = 0;
                            }

                            buffer_ptr += r;
                        }

                        //HWID
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_unit_last->hwid,
                                           ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }

                        //TYPE
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_unit_last->type = i;
                            buffer_ptr += r;
                        }

                        //VHW
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_unit_last->vhw = i;
                            buffer_ptr += r;
                        }

                        //VFW
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_unit_last->vfw = i;
                            buffer_ptr += r;
                        }

                        //ACID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_unit_last->acid = i;
                            buffer_ptr += r;
                        }

                        //NAME
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_unit_last->name,
                                           ';');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                buffer_ptr += 1;
            }
            else if (strcmp(type, "db_rulepart") == 0)
            {
                while (buffer[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_rulepart), (dbt_generic**) &db->db_rulepart,
                                        (dbt_generic**) &db->db_rulepart_last);

                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_rulepart_last->id = i;
                            buffer_ptr += r;
                        }

                        // RID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_rulepart_last->rid = i;
                            buffer_ptr += r;
                        }

                        // SCOND
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_rulepart_last->scond = i;
                            buffer_ptr += r;
                        }

                        // CITEM
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_rulepart_last->citem = i;
                            buffer_ptr += r;
                        }

                        // COP
                        r = get_next_value(buffer_ptr, &i,
                                           &db->db_rulepart_last->cop,
                                           ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }

                        // CVALUE
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_rulepart_last->cvalue = i;
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }

                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_ruletimer") == 0)
            {
                while (buffer[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_ruletimer), (dbt_generic**) &db->db_ruletimer, (dbt_generic**) &db->db_ruletimer_last);

                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_ruletimer_last->id = i;
                            buffer_ptr += r;
                        }

                        // RID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_ruletimer_last->rid = i;
                            buffer_ptr += r;
                        }

                        // TYPE
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_ruletimer_last->type = i;
                            buffer_ptr += r;
                        }

                        // DAY
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_ruletimer_last->day = i;
                            buffer_ptr += r;
                        }

                        // TSTART
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_ruletimer_last->tstart = i;
                            buffer_ptr += r;
                        }

                        // TEND
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_ruletimer_last->tend = i;
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }

                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_rule") == 0)
            {
                while (buffer[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_rule), (dbt_generic**) &db->db_rule, (dbt_generic**) &db->db_rule_last);

                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_rule_last->id = i;
                            buffer_ptr += r;
                        }

                        // NAME
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_rule_last->name,
                                           ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }

                        // ENABLED
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_rule_last->enabled = i;
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_timer") == 0)
            {
                while (buffer[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_timer), (dbt_generic**) &db->db_timer,
                                        (dbt_generic**) &db->db_timer_last);
                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_timer_last->id = i;
                            buffer_ptr += r;
                        }

                        // RID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_timer_last->rid = i;
                            buffer_ptr += r;
                        }

                        // TYPE
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_timer_last->type = i;
                            buffer_ptr += r;
                        }

                        // DAY
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_timer_last->day = i;
                            buffer_ptr += r;
                        }

                        // TSTART
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_timer_last->tstart = i;
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_action") == 0)
            {
                while (buffer[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_action), (dbt_generic**) &db->db_action,
                                        (dbt_generic**) &db->db_action_last);
                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_action_last->id = i;
                            buffer_ptr += r;
                        }
                        // RID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_action_last->rid = i;
                            buffer_ptr += r;
                        }
                        // ATYPE
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_action_last->atype = i;
                            buffer_ptr += r;
                        }
                        // AITEM
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_action_last->aitem = i;
                            buffer_ptr += r;
                        }
                        // AACTION
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_action_last->aaction = i;
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }

                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_device") == 0)
            {
                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_device), (dbt_generic**) &db->db_device,
                                        (dbt_generic**) &db->db_device_last);
                        // MAC
                        r = get_next_value(++buffer_ptr, &i, type, ',');
                        if (r)
                        {
                            db->db_device_last->mac[0] = 0;
                            buffer_ptr += r;
                        }
                        // IDATE
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->idate = i;
                            buffer_ptr += r;
                        }
                        // NAME - add 1 to buffer to skip '!'
                        r = get_next_value(++buffer_ptr, &i, db->db_device_last->name, ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                        // LOCATION - add 1 to buffer to skip '!'
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_device_last->location,
                                           ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                        // LAN_DHCP
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->lan_dhcp = i;
                            buffer_ptr += r;
                        }
                        // LAN_IP
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->lan_ip = i;
                            buffer_ptr += r;
                        }
                        // LAN_GW
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->lan_gw = i;
                            buffer_ptr += r;
                        }
                        // LAN_DNS
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->lan_dns = i;
                            buffer_ptr += r;
                        }
                        // LAN_SN
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->lan_sn = i;
                            buffer_ptr += r;
                        }
                        //cur_ip
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->cur_ip = 0;
                            buffer_ptr += r;
                        }
                        //cur_gw
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->cur_gw = 0;
                            buffer_ptr += r;
                        }
                        //cur_dns
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->cur_dns = 0;
                            buffer_ptr += r;
                        }
                        //cur_sn
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->cur_sn = 0;
                            buffer_ptr += r;
                        }
                        // TIME - add 1 to buffer to skip '!'
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_device_last->time,
                                           ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                        // DST
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->dst = i;
                            buffer_ptr += r;
                        }
                        // TIMEZONE
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->timezone = i;
                            buffer_ptr += r;
                        }
                        // VHW
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->vhw = 0;
                            buffer_ptr += r;
                        }
                        // VFW
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->vfw = 0;
                            buffer_ptr += r;
                        }
                        // VDB
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->vdb = i;
                            buffer_ptr += r;
                        }
                        // LOGLEV
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->loglev = i;
                            buffer_ptr += r;
                        }
                        // LOGUDP
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->logudp = i;
                            buffer_ptr += r;
                        }
                        // ACIP
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_device_last->acip = i;
                            buffer_ptr += r;
                        }
                        // ACP
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_device_last->acp = i;
                            buffer_ptr += r;
                        }

                    }
                    else
                    {
                        break;
                    }

                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_user") == 0)
            {
                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_user), (dbt_generic**) &db->db_user,
                                        (dbt_generic**) &db->db_user_last);
                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_user_last->id = i;
                            buffer_ptr += r;
                        }
                        // NAME
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_user_last->name,
                                           ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                        // EMAIL
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_user_last->email,
                                           ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                        // PW
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_user_last->pw = i;
                            buffer_ptr += r;
                        }
                        // LEVEL
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_user_last->level = i;
                            buffer_ptr += r;
                        }

                    }
                    else
                    {
                        break;
                    }

                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_groupitem") == 0)
            {
                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_groupitem),
                                        (dbt_generic**) &db->db_groupitem,
                                        (dbt_generic**) &db->db_groupitem_last);
                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_groupitem_last->id = i;
                            buffer_ptr += r;
                        }
                        // ITEM
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_groupitem_last->group = i;
                            buffer_ptr += r;
                        }
                        // GROUP
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_groupitem_last->item = i;
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_group") == 0)
            {

                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_group), (dbt_generic**) &db->db_group,
                                        (dbt_generic**) &db->db_group_last);
                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_group_last->id = i;
                            buffer_ptr += r;
                        }
                        // NAME
                        r = get_next_value(++buffer_ptr, &i,
                                           db->db_group_last->name,
                                           ';');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_zendpoint") == 0)
            {

                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_zdevice), (dbt_generic**) &db->db_zendpoint, (dbt_generic**) &db->db_zendpoint_last);
                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zendpoint_last->id = i;
                            buffer_ptr += r;
                        }
                        // zdevice_id
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zendpoint_last->zdevice_id = i;
                            buffer_ptr += r;
                        }
                        // zendpoint_id
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zendpoint_last->zendpoint_id = i;
                            buffer_ptr += r;
                        }
                        // profile_id
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zendpoint_last->profile_id = i;
                            buffer_ptr += r;
                        }
                        // device_id
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zendpoint_last->device_id = i;
                            buffer_ptr += r;
                        }
                        // device_version
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zendpoint_last->device_version = i;
                            buffer_ptr += r;
                        }
                        // num_in_cluster
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zendpoint_last->num_in_cluster = i;
                            buffer_ptr += r;
                        }
                        //clusters
                        for (j = 0; j < db->db_zendpoint_last->num_in_cluster; j++)
                        {
                            r = get_next_value(buffer_ptr, &i, NULL, '_');
                            if (r)
                            {
                                db->db_zendpoint_last->clusters[j] = i;
                                buffer_ptr += r;
                            }
                        }
                        buffer_ptr++; //eliminate ';'

                    }
                    else
                    {
                        break;
                    }
                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "db_zdevice") == 0)
            {

                while (buffer_ptr[0] != '\n')
                {
                    if ((buffer_ptr[0] >= '0' && buffer_ptr[0] <= '9')
                    || (buffer_ptr[0] >= 'a' && buffer_ptr[0] <= 'z')
                    || (buffer_ptr[0] >= 'A' && buffer_ptr[0] <= 'Z')
                    || buffer_ptr[0] == '!')
                    {
                        db_add_item_raw(sizeof(dbt_zdevice), (dbt_generic**) &db->db_zdevice, (dbt_generic**) &db->db_zdevice_last);
                        // ID
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zdevice_last->id = i;
                            buffer_ptr += r;
                        }
                        // ieee_address
                        r = get_next_value(++buffer_ptr, &i, &ieee_address, ',');
                        if (r)
                        {
                            db->db_zdevice_last->ieee_address = (uint64_t)ieee_to_uint64(ieee_address);
                            buffer_ptr += r;
                        }
                        // model_id
                        r = get_next_value(++buffer_ptr, &i, db->db_zdevice_last->model_id, ',');
                        if (r)
                        {
                            buffer_ptr += r;
                        }
                        // nwk_address
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zdevice_last->nwk_address = i;
                            buffer_ptr += r;
                        }
                        // mfg_id
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zdevice_last->mfg_id = i;
                            buffer_ptr += r;
                        }
                        // ep_count
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zdevice_last->ep_count = i;
                            buffer_ptr += r;
                        }
                        // parent_address
                        r = get_next_value(++buffer_ptr, &i, &ieee_address, ',');
                        if (r)
                        {
                            db->db_zdevice_last->parent_address = ieee_to_uint64(ieee_address);
                            buffer_ptr += r;
                        }
                        // cap_info
                        r = get_next_value(buffer_ptr, &i, NULL, ',');
                        if (r)
                        {
                            db->db_zdevice_last->cap_info = i;
                            buffer_ptr += r;
                        }
                        // fimware_version
                        r = get_next_value(buffer_ptr, &i, NULL, ';');
                        if (r)
                        {
                            db->db_zdevice_last->firmware_version = i;
                            buffer_ptr += r;
                        }

                        db->db_zdevice_last->status = 0;

                    }
                    else
                    {
                        break;
                    }
                }
                ++buffer_ptr; //to eliminate '\n'
            }
            else if (strcmp(type, "END_CONFIG") == 0)
            {
                break;
            }
            else
            {
                // Unsupported element, move to next if available or interrupt parsing
                buffer_ptr = strchr(buffer_ptr, ';');
                if (buffer_ptr == NULL)
                {
                    buffer_ptr++;
                }
            }
        }
        Semaphore_post(sem_db_rw);

    }
    mem_free(type, 50);
    mem_free(buffer, bufsize);
    return 1;
}

static bool check_endline(char **pos, uint16_t c)
{
    bool b;

    if ((*pos)[c] == '\r')
    {
        *pos += c + 2;
        b = true;
    }
    else if ((*pos)[c] == '\n')
    {
        *pos += c + 1;
        b = true;
    }
    else
    {
        *pos += c;
        b = false;
    }

    return b;
}

bool check_cfg_file(char *file)
{
    char c, buffer[256]; // Buffer for reading
    bool ok = false;

    spiffs_file fd;

    if (SPIFFS_mounted(&fs_spiffs) == FALSE)
    {
        return 0;
    }

    // Open the file in read mode
    fd = SPIFFS_open(&fs_spiffs, file, SPIFFS_RDONLY, 0);
    // Check if the file is opened successfully
    if (fd < 0)
    {
        //printf("Error: cannot open file %s\n", file_name);
        return false;
    }

// Read the first character of the file
    if (SPIFFS_read(&fs_spiffs, fd, &c, 1) < 0)
    {
        SPIFFS_close(&fs_spiffs, fd);
        return false;
    }

    if (c == 'v')
    {
        // Move the file pointer to the end of the file minus 12 characters
        if (SPIFFS_lseek(&fs_spiffs, fd, -12, SPIFFS_SEEK_END) < 0)
        {
            SPIFFS_close(&fs_spiffs, fd);
            return false;
        }

        // Read the remaining bytes of the file into the buffer
        if (SPIFFS_read(&fs_spiffs, fd, buffer, sizeof(buffer)) < 0)
        {
            SPIFFS_close(&fs_spiffs, fd);
            return false;
        }

        // Compare the last_line variable with the string "END_CONFIG"
        if (strstr(buffer, "END_CONFIG"))
        {
            ok = true;
        }
    }

    SPIFFS_close(&fs_spiffs, fd);
    return ok;
}

bool copy_file(const char *src, const char *dst)
{
    char *buffer; // Buffer for copying
    int32_t br, bw; // Bytes read and written
    spiffs_file fs, fd;

    buffer = mem_alloc(4096);
    if (buffer == NULL || externalflash_ready() == false)
    {
        return false;
    }

    if (SPIFFS_mounted(&fs_spiffs) == FALSE)
    {
        goto err;
    }

    // Open the source file
    fs = SPIFFS_open(&fs_spiffs, src, SPIFFS_RDONLY, 0);
    // Check if the file is opened successfully
    if (fs < 0)
    {
        goto err;
    }

    SPIFFS_remove(&fs_spiffs, dst);

// Create the destination file
    fd = SPIFFS_open(&fs_spiffs, dst, SPIFFS_CREAT | SPIFFS_WRONLY, 0);
    // Check if the file is opened successfully
    if (fd < 0)
    {
        goto err;
    }

// Copy the file content
    for (;;)
    {
        // Read a chunk of source file
        br = SPIFFS_read(&fs_spiffs, fs, buffer, sizeof(buffer));
        if (br == 0)
        {
            break; // EOF
        }
        if (br < 0)
        {
            goto err;
            // Error
        }
        // Write it to the destination file
        bw = SPIFFS_write(&fs_spiffs, fd, buffer, br);
        if (bw < 0 || bw < br)
        {
            goto err;
            // Error
        }
    }

// Close the files
    SPIFFS_close(&fs_spiffs, fs);
    SPIFFS_close(&fs_spiffs, fd);

    mem_free(buffer, 4096);

// Return success
    return true;

    err:
    // Close the files
    SPIFFS_close(&fs_spiffs, fs);
    SPIFFS_close(&fs_spiffs, fd);

    mem_free(buffer, 4096);
    // End file operations
    return false;
}

uint8_t get_next_value(char *s, long long *i, char *d, char delimiter)
{
    char *p;
    int length = 0;

    p = strchr(s, delimiter);
    if (p == NULL)
    {
        length = strlen(s);
        strncpy(d, s, length);
        d[length] = '\0';
    }
    else
    {
        length = p - s;
        if (d)
        {
            strncpy(d, s, length);
            d[length] = '\0';
        }
        else
        {
            *i = atoi(s);
        }
    }
    p = strchr(s, delimiter);
    if (p == NULL)
    {
        length = strlen(s);
        strncpy(d, s, length);
        d[length] = '\0';
    }
    else
    {
        length = p - s;
        if (d)
        {
            strncpy(d, s, length);
            d[length] = '\0';
        }
        else
        {
            *i = atoi(s);
        }
    }

    return length + 1;
}

uint32_t size_file(const char *file_name)
{
    uint32_t size = 0;
    spiffs_file fp;
    fp = SPIFFS_open(&fs_spiffs, file_name, SPIFFS_RDONLY, 0);

    if (fp == NULL)
    {
        return 0;
    }

    size = SPIFFS_lseek(&fs_spiffs, fp, 0, SPIFFS_SEEK_END) - size;

    SPIFFS_close(&fs_spiffs, fp);

    return size;
}

uint32_t db_migration_2to3(char **buffer, uint32_t file_size)
{
    char *temp = malloc(file_size + 256);
    char insert[20];
    char search[] = "db_device:";
    char *line, *pos, *start, *end;
    int i = 0;
    uint32_t ip = 0;

    line = strstr((*buffer), search);       //Find the start line to modify
    end = strchr(line, '\n') - 1;           //Find the end of the line to modify

    start = strchr(line, ':') + 1; //Get the start and the end of the first object
    strncpy(temp, (*buffer), (start - (*buffer))); //Copy the file until the first object
    temp[(start - (*buffer))] = '\0';

// MAC
    sprintf(insert, "!0", ip);
    strcat(temp, insert);

    pos = strchr(line, ',');

    for (i = 0; start < end; i++)
    {
        if ((i > 3 && i < 12) || i == 20)
        {
            start = pos;
            pos = strchr(++pos, ',');
            get_next_value(++start, (long long*) &ip, NULL, ',');
            ip = ((((ip) & 0xff000000) >> 24) | (((ip) & 0x00ff0000) >> 8)
            | (((ip) & 0x0000ff00) << 8) | (((ip) & 0x000000ff) << 24));
            sprintf(insert, ",%u", ip);
            strcat(temp, insert);
        }
        else
        {
            start = pos;
            pos = strchr(++pos, ',');
            if (pos < end)
            {
                strncat(temp, start, pos - start);

            }
            else if (start < end)
            {
                strncat(temp, start, end - start);
            }
        }
    }
    start = end;

    strcat(temp, start); // copy remind parameters

    mem_free(*buffer, file_size);

    *buffer = temp;
    file_size = file_size + 256;

    return file_size;
}

uint32_t db_migration_3to4(char **buffer, uint32_t file_size)
{
    char *temp = mem_alloc(file_size + 256);
    char insert[20];
    char *line, *pos, *start, *end;
    int id = 0;

    line = strstr((*buffer), "db_unit:"); //Find the start line to modify
    end = strchr(line, '\n') - 1;           //Find the end of the line to modify

    pos = strchr(line, ':');     //Get the start and the end of the first object
    start = pos;
    pos = strchr(line, ';');

    strncpy(temp, (*buffer), (pos - (*buffer))); //Copy the file until the first object
    temp[(pos - (*buffer))] = '\0';

    while (start < end)
    {
        start++;                                //jump ";"

        get_next_value(start, (long long*) &id, NULL, ','); //get id and make a name string with it
        sprintf(insert, ",!UNIT %d", id);

        strcat(temp, insert);        //insert the name

        start = pos;                //save the position unit id

        pos = strchr(++pos, ';');    //get the next "";"

        if (pos <= end)
        {
            strncat(temp, start, pos - start);  //Copy units parameter
        }
    }
    strcat(temp, start); // copy remind parameters

    mem_free(*buffer, file_size);

    *buffer = temp;

    file_size = file_size + 256;

    return file_size;
}

void format_and_resize_string(spiffs_file fd, char **str, size_t *size_str, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    // Calculate the size needed for the new formatted content
    size_t size_needed = vsnprintf(NULL, 0, format, args) + 1;  // +1 for the null terminator
    va_end(args);

    if(size_str)
    {
        *size_str = strlen(*str) + size_needed;
    }

    // Attempt to resize the string
    char *new_str = realloc(*str, strlen(*str) + size_needed);

    if (new_str != NULL)
    {
        *str = new_str;  // Update the original string pointer
        // Reinitialize va_list to use it with vsnprintf
        va_start(args, format);
        vsnprintf(*str + strlen(*str), size_needed, format, args);  // Append to the end of the string
        va_end(args);
    }
    else if(fd != NULL)
    {
        // If memory allocation fails, write the current content to the file
        System_printf("Not enough memory available; saving to file.\n");
        System_flush();
        if (SPIFFS_write(&fs_spiffs, fd, *str, strlen(*str)) < 0)
        {
            System_printf("Error writing to the file.\n");
            System_flush();
        }
        // Free the string and reset it
        free(*str);
        *str = malloc(size_needed);  // Allocate space for the new content
        if (*str == NULL)
        {
            System_printf("Critical error: Memory allocation failed.\n");
            System_flush();
            return;  // Exit if memory allocation fails
        }
        // Format directly into the new empty buffer
        va_start(args, format);
        vsnprintf(*str, size_needed, format, args);
        va_end(args);
    }
}

