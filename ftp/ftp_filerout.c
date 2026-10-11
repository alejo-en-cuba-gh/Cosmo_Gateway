/**
 * @file ftp_filerout.c
 *
 * @brief FTP Server and client implementation.
 *
 */

/*
 * Include files
 */
#include "modules.h"
#include "ftpserver.h"

int8_t ftp_filerout_read(io_handler_t *ioh, char *name)
{
    char filepath[256];
    sprintf(filepath, "fat:%s/%s", ioh->current_working_dir_path, name);

    if (sd_ready(true) == FALSE)
    {
        return CSL_ESYS_FAIL;
    }

    FILE *src = fopen(filepath, "r");
    if (!src)
    {
        return CSL_ESYS_FAIL;
    }

    /*  Copy the contents from the src to the dst */
    while (1)
    {
        /*  Read from source file */
        unsigned int bytesRead = fread(ioh->DataBuf, 1, DATA_BUFFER_SIZE, src);
        if (bytesRead == 0)
        {
            break; /* Error or EOF */
        }
        send(ioh->data_socket, ioh->DataBuf, bytesRead, 0);
        Task_yield();
    }
    return CSL_SOK;
}

int8_t ftp_filerout_write(io_handler_t *ioh, char *name)
{
    int32_t bytesRead = 0, bytesWritten;
    char *pBuf;
    void *hBuffer;
    char filepath[256];
    sprintf(filepath, "fat:%s/%s", ioh->current_working_dir_path, name);

    if (sd_ready(true) == FALSE)
    {
        return CSL_ESYS_FAIL;
    }

    /* Open the file for the received data */
    FILE *dst = fopen(filepath, "w");
    if (!dst)
    {
        return -1;
    }
    int result = setvbuf(dst, NULL, _IONBF, 0);

    do
    {
        bytesRead = (int) recvnc(ioh->data_socket, (void**) &pBuf, 0, &hBuffer);

        if (bytesRead > 0)
        {
            bytesWritten = fwrite(pBuf, 1, bytesRead, dst);
            if (bytesWritten < bytesRead)
            {
                return -1;
            }
            recvncfree(hBuffer);
        }
    }
    while (bytesRead > 0);

    fflush(dst);
    fclose(dst);

    return 0;
}

/** @brief Write out a director listing
 *
 */
int8_t ftp_filerout_list(io_handler_t *ioh, bool dir_mode)
{
    DIR dir;
    char buffer[128];

    if (sd_ready(true) == FALSE)
    {
        return CSL_ESYS_FAIL;
    }

    FRESULT fRes = f_opendir(&dir, ioh->current_working_dir_path);
    if (fRes != 0)
    {
        return -1;
    }
    else
    {
        FILINFO filedat;
        fRes = f_readdir(&dir, &filedat);

        // For each file (until a file has an empty name)
        while (strlen(filedat.fname) > 0)
        {
            if (dir_mode)
            {
                uint16_t years = (filedat.fdate >> 9) & 0x7F;
                uint16_t months = (filedat.fdate >> 5) & 0x0F;
                uint16_t days = (filedat.fdate >> 0) & 0x1F;
                uint16_t hours = (filedat.ftime >> 11) & 0x1F;
                uint16_t mins = (filedat.ftime >> 5) & 0x3F;
                uint16_t secs = (filedat.ftime << 1) & 0x3F;

                years += 1980;

                // Generate a Unix like file entry - requirement is not well specified but WinSCP seems able to read this.
                sprintf(buffer, "%cr%c-------  1 sw           %d %02d/%02d/%04d %02d:%02d:%02d %s\r\n", (filedat.fattrib & AM_DIR) ? 'd' : '-', // First char is 'd' for a directory, '-' for a file
                        (filedat.fattrib & AM_RDO) ? '-' : 'w',  // Third char is 'w' if file is writeable.
                        filedat.fsize, days, months, years, hours, mins, secs, filedat.fname);
            }
            else
            {
                sprintf(buffer, "%s\r\n", filedat.fname);
            }
            // Write each file entry to the data socket.
            send(ioh->data_socket, buffer, strlen(buffer), 0);

            // read the next file info.
            fRes = f_readdir(&dir, &filedat);
        }

        f_closedir(&dir);
    }

    return CSL_SOK;
}

/** @brief Change current working directory
 *
 * @param ioh    Pointer to connection control struct
 * @param path   Pointer to the proposed new directory string
 *
 */
int8_t ftp_filerout_changeDir(io_handler_t *ioh, char *path)
{
    int8_t retval = CSL_SOK;
    char newPath[DATA_BUFFER_SIZE], *a;

    if (strcmp(path, "..") == 0)
    {
        if (strlen(ioh->current_working_dir_path) > 3)
        {
            a = strrchr(&ioh->current_working_dir_path[3], '/');
            if (a)
            {
                // Terminate directory at last '/'
                *a = 0;
                strcpy(newPath, ioh->current_working_dir_path);
            }
            else
            {
                // we are in first level folder, change to root
                sprintf(newPath, "%s", "0:");
            }
        }
        else
        {
            // Already in root
            retval = -1;
        }
    }
    else if (path[0] == '/')
    {
        // The path starts with '/', so it is a full path
        if (path[1] == 0)
        {
            // No other chars in name, so use the root.
            sprintf(newPath, "%s", "0:");
        }
        else
        {
            // Set new path
            sprintf(newPath, "0:%s", path);
        }
    }
    else
    {
        // A directory name - append it to the current path.
        sprintf(newPath, "%s/%s", ioh->current_working_dir_path, path);
    }

    if (sd_ready(true) == FALSE)
    {
        return CSL_ESYS_FAIL;
    }

    /* Check if the new directory name exists. */
    DIR dir;
    FRESULT fRes = f_opendir(&dir, newPath);

    if (fRes == 0)
    {
        f_closedir(&dir);
        strncpy(ioh->current_working_dir_path, newPath, DATA_BUFFER_SIZE);
    }
    else
    {
        retval = -1;
    }

    return retval;
}

int8_t ftp_filerout_delete(io_handler_t *ioh, char *name)
{
    FRESULT fresult;
    char filepath[256];

    if (name[0] == '/')
    {
        // Full path transmitted
        sprintf(filepath, "0:%s", name);
    }
    else
    {
        // Only file transmitted
        sprintf(filepath, "%s/%s", ioh->current_working_dir_path, name);
    }

    if (sd_ready(true) == FALSE)
    {
        return CSL_ESYS_FAIL;
    }

    fresult = f_unlink(filepath);
    return fresult;
}

int8_t ftp_filerout_mkdir(io_handler_t *ioh, char *name)
{
    FRESULT fresult;
    char filepath[256];

    if (name[0] == '/')
    {
        // Full path transmitted
        sprintf(filepath, "0:%s", name);
    }
    else
    {
        // Only file transmitted
        sprintf(filepath, "%s/%s", ioh->current_working_dir_path, name);
    }

    if (sd_ready(true) == FALSE)
    {
        return CSL_ESYS_FAIL;
    }

    fresult = f_mkdir(filepath);
    return fresult;
}

int8_t ftp_filerout_rmdir(io_handler_t *ioh, char *name)
{
    FRESULT fresult;
    char filepath[256];

    if (name[0] == '/')
    {
        // Full path transmitted
        sprintf(filepath, "0:%s", name);
    }
    else
    {
        // Only file transmitted
        sprintf(filepath, "%s/%s", ioh->current_working_dir_path, name);
    }

    if (sd_ready(true) == FALSE)
    {
        return CSL_ESYS_FAIL;
    }

    fresult = f_unlink(filepath);
    return fresult;
}
