/*
 * json.c
 *
 *  Created on: 7 oct 2024
 */

#include "common.h"
#include "jsonc.h"

#include <xdc/runtime/System.h>
#include <ti/utils/json/json.h>

bool get_template(char *name, char *template, uint16_t *size_template)
{
    uint32_t file_size = 0;
    spiffs_file fd;
    file_size = get_fileSize(name);
    fd = SPIFFS_open(&fs_spiffs, name, SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        return false;
    }
    else
    {

        *size_template = SPIFFS_read(&fs_spiffs, fd, template,file_size);
        if (*size_template != file_size)
        {
            return false;
        }
    }
    SPIFFS_close(&fs_spiffs, fd);
    return true;
}

bool create_template(char *name, char *template, uint16_t size_template)
{
    spiffs_file fd;
    SPIFFS_remove(&fs_spiffs, name);
    fd = SPIFFS_open(&fs_spiffs, name, SPIFFS_CREAT | SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        return false;
    }
    if (SPIFFS_write(&fs_spiffs, fd, template, size_template) < 0)
    {
        return false;
    }
    SPIFFS_close(&fs_spiffs, fd);
    return true;
}

bool delete_template(char *name)
{
    SPIFFS_remove(&fs_spiffs, name);
    return true;
}

bool lis_dir()
{
    FRESULT fresult; // FATFS result variable
    DIR dir; // Directory object
    FILINFO fno;
    UInt64 nfile = 0;
    if (open_dirjson(&dir) == false)
    {
        return false;
    }
    while (true)
    {
        fresult = f_readdir(&dir, &fno); /* Read a directory item */
        if (fresult != FR_OK || fno.fname[0] == 0)
            break; /* Error or end of dir */
        if (!(fno.fattrib & AM_DIR))
        { /* Directory */
            System_printf("%s \n", fno.fname);
            System_flush();
            nfile++;
        }
    }
    f_closedir(&dir);
    return true;
}

bool create_json(Json_Handle *hObject, Json_Handle *hTemplate, char *cmd)
{
    char template_aux[300];
    int16_t retVal;
    uint16_t template_size = 0;
    if (get_template(cmd, template_aux, &template_size) == false)
    {
        System_printf("Get Template Error Memory\n");
        System_flush();
        return false;
    }

    char *template = mem_alloc(template_size);
    strncpy(template, template_aux, template_size);

    retVal = Json_createTemplate(hTemplate, template, template_size);
    if (retVal != 0)
    {
        System_printf("Get Template Error Create Template: %d\n", retVal);
        System_flush();
        return false;
    }
    retVal = Json_createObject(hObject, *hTemplate, 0);
    if (retVal != 0)
    {
        System_printf("Get Template Create object\n");
        System_flush();
        return false;
    }
    mem_free(template, template_size);
    return true;
}

bool parse_json(Json_Handle hObject, char *json_text, uint16_t json_textlen)
{
    int16_t retVal;
    retVal = Json_parse(hObject, json_text, json_textlen);
    if (retVal != 0)
    {
        return false;
    }
    return true;
}

char* get_jsonstring(Json_Handle hObject, const char *p_key, uint16_t *value_size)
{
    int16_t retVal;
    char *value;
    retVal = Json_getValue(hObject, p_key, NULL, value_size);
    if (retVal != 0)
    {
        return false;
    }

    value = calloc(*value_size + 1, sizeof(char));
    retVal = Json_getValue(hObject, p_key, value, value_size);
    if (retVal != 0)
    {
        return NULL;
    }
    *value_size = *value_size + 1;
    return value;
}

bool set_jsonstring(Json_Handle hObject, const char *p_key, char *p_value)
{
    int16_t retVal;
    retVal = Json_setValue(hObject, p_key, p_value, strlen(p_value));
    if (retVal != 0)
    {
        return false;
    }
    return true;
}
