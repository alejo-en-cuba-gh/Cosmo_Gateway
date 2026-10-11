/*
 * timer_s.c
 *
 *  Created on: 7 nov 2023
 */

#include "common.h"
#include "timer_s.h"

Semaphore_Handle sem_ltime, sem_ttick;

void timer30(UArg arg)
{
    Semaphore_post(sem_ttick);
}

void timer_tick(UArg arg0, UArg arg1)
{
    Task_Params taskParams;
    CAN_Frame can;
    db_event_t e;
    uint8_t tcount = 1;
    uint8_t status = 0;

    while (1)
    {
        Semaphore_pend(sem_ttick, BIOS_WAIT_FOREVER);
        //logevent(L_DEBUG, "timer_tick");
        tcount++;
/*
        // Every 30 seconds
        can.can_id = C_ALIVE;
        can.can_dlc = 0;
        if (Mailbox_post(m_can_out, &can, BIOS_NO_WAIT) == false)
        {
            err_mailbox_full(__func__, m_can_out);
        }*/

        if (tcount % 2 == 0)
        {
            // Every minute
        }

        if (tcount % 10 == 0)
        {
            // Every 5 minutes
            flash_read_val(FL_STATUS_1, &status, 1);
            command_101(status);
        }

        if (tcount % 20 == 0)
        {
            // Every 10 minutes
            e = DBE_EXPORT;
            if (Mailbox_post(m_db, &e, BIOS_NO_WAIT) == false)
            {
                err_mailbox_full(__func__, m_db);
            }

        }

        if (tcount % 60 == 0)
        {
            // Every 30 minutes
            get_free_ram();

            if (tcount == 120)
            {
                // Every hour

                Task_Params_init(&taskParams);
                taskParams.priority = TASK_PRIO_MEDIUM;
                taskParams.stackSize = TASK_HUNGRY;
                taskParams.instance->name = "cron_run";
                Task_create(cron_run, &taskParams, NULL);

                tcount = 0;
            }
        }
    }
}

uint16_t to_timestamp(uint8_t hour, uint8_t minute)
{
    // Shift the hour to the left by 8 bits and use bitwise OR to combine it with the minute
    uint16_t time = (hour << 8) | minute;
    // Return the time value
    return time;
}

void from_timestamp(uint16_t time, uint8_t *hour, uint8_t *minute)
{
    // Use bitwise AND and right shift to extract the hour from the upper 8 bits
    *hour = (time & 0xFF00) >> 8;
    // Use bitwise AND to extract the minute from the lower 8 bits
    *minute = time & 0x00FF;
}

void set_day(uint32_t *month, uint8_t day, bool value)
{
    // set or unset specific days in the month by passing the number of the day and a bool value
    if (value)
    {
        SET_BIT(*month, day - 1); // set the bit to 1
    }
    else
    {
        CLEAR_BIT(*month, day - 1); // clear the bit to 0
    }
}

bool get_day(uint32_t month, int day)
{
    // get a bool return when indicating a specific day if set or not
    return GET_BIT(month, day); // return the value of the bit
}

bool* get_days(uint32_t month)
{
    bool *days = mem_alloc(31 * sizeof(bool));

    uint8_t i;
    for (i = 0; i < 31; i++)
    {
        days[i] = GET_BIT(month, i);
    }
    return days;
}

void get_ltime(struct tm *mt)
{
    // A function that returns a pointer to a tm structure containing the local time
    // Get the current time value in seconds since 1970
    // Attention: calling function must free time pointer!!!
    time_t now = time(NULL);
    dbt_device *dbd;
    struct tm *lt;

    dbd = get_element(&config_db.db_device, DEVICE);

    // Get semaphore for localtime
    Semaphore_pend(sem_ltime, BIOS_WAIT_FOREVER);

    // Get local time and copy to function's memory
    lt = localtime(&now);
    memcpy(mt, lt, sizeof(struct tm));

    // Release semaphore for localtime
    Semaphore_post(sem_ltime);

    // Apply the DST correction if needed
    switch (dbd->dst)
    {
    case 1:
        // USA an Cuba DST correction
        // DST starts on the second Sunday of March at 2 AM local time
        // DST ends on the first Sunday of November at 2 AM local time
        // The time zone shifts by one hour during DST
        if ((mt->tm_mon == 2 && mt->tm_mday - mt->tm_wday >= 8
        && mt->tm_hour >= 2) || // after second Sunday 2 AM in March
        (mt->tm_mon == 10 && mt->tm_mday - mt->tm_wday < 1
        && mt->tm_hour < 1) || // before first Sunday 1 AM in November
        (mt->tm_mon > 2 && mt->tm_mon < 10))
        { // between March and November
          // add one hour for DST
            mt->tm_hour++;
        }
        break;
    case 2:
        // AUST DST correction
        // DST starts on the first Sunday of October at 2 AM local time
        // DST ends on the first Sunday of April at 2 AM local time
        // The time zone shifts by one hour during DST
        if ((mt->tm_mon == 9 && mt->tm_mday - mt->tm_wday >= 1
        && mt->tm_hour >= 2) || // after first Sunday 2 AM in October
        (mt->tm_mon == 3 && mt->tm_mday - mt->tm_wday < 1
        && mt->tm_hour < 2) || // before first Sunday 2 AM in April
        (mt->tm_mon > 9 || mt->tm_mon < 3))
        { // between October and April
          // add one hour for DST
            mt->tm_hour++;
        }
        break;
    case 3:
        // WEST DST correction
        // DST starts on the last Sunday of March at 1 AM UTC
        // DST ends on the last Sunday of October at 1 AM UTC
        // The time zone shifts by one hour during DST
        if ((mt->tm_mon == 2 && mt->tm_mday - mt->tm_wday >= 25
        && mt->tm_hour >= 1) || // after last Sunday 1 AM in March
        (mt->tm_mon == 9 && mt->tm_mday - mt->tm_wday < 25
        && mt->tm_hour < 1) || // before last Sunday 1 AM in October
        (mt->tm_mon > 2 && mt->tm_mon < 9))
        { // between March and October
          // add one hour for DST
            mt->tm_hour++;
        }
        break;
    case 4:
        // MIDEAST DST correction
        // DST starts on the last Friday of March at 12 AM local time
        // DST ends on the last Friday of September at 12 AM local time
        // The time zone shifts by one hour during DST
        if ((mt->tm_mon == 2 && mt->tm_mday - mt->tm_wday >= 24
        && mt->tm_wday == 5 && mt->tm_hour >= 0) || // after last Friday 12 AM in March
        (mt->tm_mon == 8 && mt->tm_mday - mt->tm_wday < 24
        && mt->tm_wday == 5 && mt->tm_hour < 0) || // before last Friday 12 AM in September
        (mt->tm_mon > 2 && mt->tm_mon < 8))
        { // between March and September
          // add one hour for DST
            mt->tm_hour++;
        }
        break;
    default:
        // no DST correction
        break;
    }

    // Apply the timezone offset in hours
    mt->tm_hour += dbd->timezone;

    // Adjust the time components if needed
    mktime(mt);

    mem_free(dbd, sizeof(dbt_device));
}

void cron_run(UArg arg0, UArg arg1)
{
    struct tm now;
    char lm[50];

    // Wait for DB ready
    Semaphore_pend(sem_db_ready, BIOS_WAIT_FOREVER);

    // Release the read semaphore
    Semaphore_post(sem_db_ready);

    get_ltime(&now);
    sprintf(lm, "cron_run: %02d:%02d:%02d", now.tm_hour, now.tm_min,
            now.tm_sec);
    logevent(L_DEBUG, lm);

    // Wait until the next hour has been reached
    Task_sleep((3600 - (now.tm_min * 60 + now.tm_sec)) * 1000);

    switch (now.tm_hour)
    {
    case 2:
        // 02:00
        delete_old_files_by_size_spiffs(LOG_PATH, (float)1.0, (float)0.5);
        break;
    case 3:
        // 03:00
        update_sntp_time();
        break;
    }

    Task_exit();
}
