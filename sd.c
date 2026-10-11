/*
 * sd.c
 *
 *  Created on: 15 dic 2023
 */

#include <file.h>

#include <ti/display/Display.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SDFatFS.h>

#include "common.h"
#include "sd.h"

#include <third_party/fatfs/ffcio.h>

Semaphore_Handle sem_sd, sem_sd_gpio;
SDFatFS_Handle sdfatfsHandle = NULL;
bool nosd = true;

void init_sd(UArg arg0, UArg arg1)
{
    msg_led_t l;

    while (1)
    {
        if (GPIO_read(SD_DETECT))
        {
            // No SD
            sdfatfsHandle = NULL;
            nosd = true;

            l.led = LS_SD;
            l.state = true;
            if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false)
            {
                err_mailbox_full(__func__, m_led);
            }
        }
        else
        {
            // Mount and register the SD Card
            //sdfatfsHandle = SDFatFS_open(CONFIG_SD_0, DRIVE_NUM);
            if (sdfatfsHandle)
            {
                add_device(fatfsPrefix, _MSA, ffcio_open, ffcio_close, ffcio_read, ffcio_write, ffcio_lseek, ffcio_unlink, ffcio_rename);
            }
            nosd = false;

            check_configdir();

            l.led = LS_SD;
            l.state = false;
            if (Mailbox_post(m_led, &l, BIOS_NO_WAIT) == false)
            {
                err_mailbox_full(__func__, m_led);
            }
        }

        Semaphore_post(sem_sd);
        Semaphore_pend(sem_sd_gpio, BIOS_WAIT_FOREVER);
    }
}

bool sd_ready(bool set_sem)
{
    bool r;
    Semaphore_pend(sem_sd, BIOS_WAIT_FOREVER);

    if (!GPIO_read(SD_DETECT) && sdfatfsHandle)
    {
        r = true;
    }
    else
    {
        r = false;
    }

    Semaphore_post(sem_sd);

    return r;
}

bool log2sd(bool event, char *msg)
{
    const char folder[] = "0:/log";
    FRESULT fresult; // FATFS result variable
    FIL fil; // File object
    DIR dir; // Directory object
    UINT bw; // Bytes written
    char p, file[60];
    struct tm now;

    if (event)
    {
        p = 'e';
    }
    else
    {
        p = 's';
    }

    get_ltime(&now);
    sprintf(file, "%s/%cl-%02d%02d%02d.log", folder, p, now.tm_mday, now.tm_mon + 1, now.tm_year - 100);

    // Start file operations
    if (sd_ready == false)
    {
        return 0;
    }

    // Check if the folder exists
    fresult = f_opendir(&dir, folder);
    if (fresult == FR_NO_PATH)
    {
        // Folder does not exist, create it
        fresult = f_mkdir(folder);
        if (fresult != FR_OK)
        {
            // Failed to create folder, return error
            return false;
        }
    }
    else if (fresult == FR_OK)
    {
        // Close the directory object
        f_closedir(&dir);
    }
    else
    {
        // Failed to open folder, return error
        return false;
    }

    // Check if the file exists
    fresult = f_open(&fil, file, FA_OPEN_ALWAYS | FA_WRITE);
    if (fresult != FR_OK)
    {
        // Failed to create file, return error
        return false;
    }

    // Move to the end of the file
    fresult = f_lseek(&fil, f_size(&fil));
    if (fresult != FR_OK)
    {
        // Failed to move to the end of the file, return error
        f_close(&fil);
        return false;
    }

    // Write the message to the file
    fresult = f_write(&fil, msg, strlen(msg), &bw);
    if (fresult != FR_OK || bw != strlen(msg))
    {
        // Failed to write the message, return error
        f_close(&fil);
        return false;
    }

    // Close the file object
    f_close(&fil);

    // Return success
    return true;
}

/*
 *  ======== fatfs_getFatTime ========
 */
int32_t fatfs_getFatTime(void)
{
    uint32_t fatTime;
    struct tm pTime;

    get_ltime(&pTime);

    fatTime = ((uint32_t) (pTime.tm_year - 80) << 25) | ((uint32_t) (pTime.tm_mon + 1) << 21) | ((uint32_t) (pTime.tm_mday) << 16) | ((uint32_t) (pTime.tm_hour) << 11) | ((uint32_t) (pTime.tm_min) << 5) | ((uint32_t) (pTime.tm_sec) >> 1);
    return ((int32_t) fatTime);
}

void sd_pinevent(uint_least8_t index)
{
    if (!nosd)
    {
        Semaphore_pend(sem_sd, BIOS_WAIT_FOREVER);
    }
    Semaphore_post(sem_sd_gpio);
}

static bool check_configdir()
{
    const char folder[] = "0:/config";
    FRESULT fresult; // FATFS result variable
    DIR dir; // Directory object

    // Check if the folder exists
    fresult = f_opendir(&dir, folder);
    if (fresult == FR_NO_PATH)
    {
        // Folder does not exist, create it
        fresult = f_mkdir(folder);
        if (fresult != FR_OK)
        {
            // Failed to create folder, return error
            f_closedir(&dir);
            return false;
        }
    }
    else if (fresult != FR_OK)
    {
        // Failed to open folder, return error
        return false;
    }

    // Close the directory object
    f_closedir(&dir);
    return true;
}

bool untar(char *filename)
{
    // Extract a tar file using FatFS
    char lm[50], *name, type, buffer[512];
    FIL fd, fd_out;
    FRESULT res;
    struct tar_header header;
    UINT bytes_read, bytes_written;
    int size;

    // Open the tar file for reading
    res = f_open(&fd, filename, FA_READ);
    if (res != FR_OK)
    {
        sprintf(lm, "untar: file open error %s", filename);
        logevent(L_ERROR, lm);
        return false;
    }

    // Read the tar file header by header

    while (f_read(&fd, &header, sizeof(header), &bytes_read) == FR_OK && bytes_read == sizeof(header))
    {
        // Check the magic and version fields
        if (strncmp(header.magic, "ustar", 5) != 0 || strncmp(header.version, "00", 2) != 0)
        {
            break;
        }

        // Get the file name, size, and type
        name = header.name;
        size = oct2dec(header.size, 11);
        type = header.typeflag[0];

        // Skip the padding blocks at the end
        if (strcmp(name, "") == 0)
        {
            break;
        }

        // Print the file information
        sprintf(lm, "untar; %s (%d bytes, type %c)", name, size, type);
        logevent(L_DEBUG, lm);

        // Handle different file types
        switch (type)
        {
            case '0': // regular file
            case '\0': // regular file
                // Create the file for writing
                res = f_open(&fd_out, name, FA_CREATE_ALWAYS | FA_WRITE);
                if (res != FR_OK)
                {
                    sprintf(lm, "untar: write error", filename);
                    logevent(L_ERROR, lm);
                    return false;
                }

                // Copy the file data from the tar file
                while (size > 0)
                {
                    res = f_read(&fd, buffer, 512, &bytes_read);
                    if (res != FR_OK)
                    {
                        sprintf(lm, "untar: read error", filename);
                        logevent(L_ERROR, lm);
                        return false;
                    }
                    res = f_write(&fd_out, buffer, bytes_read, &bytes_written);
                    if (res != FR_OK)
                    {
                        sprintf(lm, "untar: write error", filename);
                        logevent(L_ERROR, lm);
                        return false;
                    }
                    size -= bytes_read;
                }

                // Close the file
                f_close(&fd_out);
                break;
            case '5': // directory
                // Create the directory
                f_mkdir(name);
                break;
            default: // unsupported type
                sprintf(lm, "untar: unsupported file type: %c", type);
                logevent(L_ERROR, lm);
                return false;
        }
    }

    // Close the tar file
    f_close(&fd);
    return true;
}

static int oct2dec(char *oct, int size)
{
    // Convert an octal string to a decimal number
    int result = 0, i;
    for (i = 0; i < size; i++)
    {
        result = result * 8 + (oct[i] - '0');
    }
    return result;
}

void delete_old_files_by_size(char *folder, uint16_t mb_high, uint16_t mb_low)
{
    // A function that checks if a folder is larger than mb_high and deletes the oldest files until reaching less than mb_low
    DIR dir;
    FILINFO fno;
    FRESULT res;
    char path[50], lm[70];
    DWORD size = 0;
    int count = 0;
    int i, j;
    char **files;
    DWORD *times;
    DWORD temp;
    char *tempstr;

    if (sd_ready(true) == FALSE)
    {
        sprintf(lm, "delete_old_files_by_size: SD not ready", mb_high, mb_low);
        logevent(L_DEBUG, lm);
        return;
    }
    else
    {
        sprintf(lm, "delete_old_files_by_size: high %d low %d", mb_high, mb_low);
        logevent(L_DEBUG, lm);
    }

    // Open the directory
    res = f_opendir(&dir, folder);
    if (res == FR_OK)
    {
        // Loop through the directory to get the size and count
        while (1)
        {
            // Read a directory item
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0)
            {
                // Break on error or end of dir
                break;
            }
            if (fno.fattrib & AM_DIR)
            {
                // Skip sub directories
                continue;
            }

            // Add the file size to the total size
            size += fno.fsize;

            // Increment the file count
            count++;
        }
        // Close the directory
        f_closedir(&dir);
    }
    else
    {
        return;
    }

    // Check if the size is larger than mb_high
    if (size > mb_high * 1024 * 1024)
    {
        // Allocate memory for the file names and times arrays
        files = (char**) malloc(count * sizeof(char*));
        times = (DWORD*) malloc(count * sizeof(DWORD));

        // Reopen the directory
        res = f_opendir(&dir, folder);
        if (res == FR_OK)
        {
            // Loop through the directory to get the file names and times
            i = 0; // Index for the arrays
            while (1)
            {
                // Read a directory item
                res = f_readdir(&dir, &fno);
                if (res != FR_OK || fno.fname[0] == 0)
                {
                    // Break on error or end of dir
                    break;
                }
                if (fno.fattrib & AM_DIR)
                {
                    // Skip sub directories
                    continue;
                }

                // Get the file path
                sprintf(path, "%s/%s", folder, fno.fname);

                // Allocate memory for the file name and copy it
                files[i] = (char*) malloc(50 * sizeof(char));
                strcpy(files[i], path);

                // Get the file status
                res = f_stat(path, &fno);
                if (res != FR_OK)
                {
                    continue; // Skip on error
                }

                // Convert the file modification date and time to DWORD
                times[i] = ((DWORD) fno.fdate << 16) | fno.ftime;

                // Increment the index
                i++;
            }
            // Close the directory
            f_closedir(&dir);
        }

        // Sort the files by modification time using bubble sort
        // Note: you can use other sorting algorithms if you prefer
        for (i = 0; i < count - 1; i++)
        {
            for (j = 0; j < count - i - 1; j++)
            {
                if (times[j] > times[j + 1])
                {
                    // Swap the times
                    temp = times[j];
                    times[j] = times[j + 1];
                    times[j + 1] = temp;

                    // Swap the file names
                    tempstr = files[j];
                    files[j] = files[j + 1];
                    files[j + 1] = tempstr;
                }
            }
        }

        // Loop through the sorted files and delete the oldest ones until the size is below 40 MB
        i = 0; // Index for the arrays
        while (i < count && size > mb_low * 1024 * 1024)
        {
            // Subtract the file size from the total size
            res = f_stat(files[i], &fno);
            if (res == FR_OK)
            {
                size -= fno.fsize;
            }

            // Delete the file
            res = f_unlink(files[i]);
            if (res == FR_OK)
            {
                // Print a message
                sprintf(lm, "delete_old_files_by_size: delete %s", files[i]);
                logevent(L_DEBUG, lm);
            }

            // Increment the index
            i++;
        }

        // Free the memory for the file names and times arrays
        for (i = 0; i < count; i++)
        {
            free(files[i]);
        }
        free(files);
        free(times);
    }
}
