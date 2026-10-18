/*
 * spiffsexternal.c
 *
 *  Created on: 16 oct 2024
 */

#include "common.h"
#include <xdc/runtime/System.h>

/* SPIFFS configuration parameters */
#define SPIFFS_LOGICAL_BLOCK_SIZE    (16384)
#define SPIFFS_LOGICAL_PAGE_SIZE     (128)
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (44)


/*
 * SPIFFS needs RAM to perform operations on files.  It requires a work buffer
 * which MUST be (2 * LOGICAL_PAGE_SIZE) bytes.
 */
static uint8_t spiffsWorkBuffer[SPIFFS_LOGICAL_PAGE_SIZE * 2];

/* The array below will be used by SPIFFS as a file descriptor cache. */
static uint8_t spiffsFileDescriptorCache[SPIFFS_FILE_DESCRIPTOR_SIZE * 4];

/* The array below will be used by SPIFFS as a read/write cache. */
static uint8_t spiffsReadWriteCache[SPIFFS_LOGICAL_PAGE_SIZE * 2];

#define MESSAGE_LENGTH    (22)

// Static files that we want to check for existence
const char* static_files[] = {
    "firmware.fw",
    "sw.cfg",
    "sw.cfg.bu"
};

const int num_static_files = 3;

SPIFFSNVS_Data spiffsnvsData;
Semaphore_Handle sem_spiffs_mount;

void init_spiffs_external(){

    spiffs_config  fsConfig;
    int32_t        status;

    status = SPIFFSNVS_config(&spiffsnvsData, EXTERNAL, &fs_spiffs, &fsConfig,
        SPIFFS_LOGICAL_BLOCK_SIZE, SPIFFS_LOGICAL_PAGE_SIZE);
    if (status != SPIFFSNVS_STATUS_SUCCESS)
    {
        while(1);
    }
    status = SPIFFS_mount(&fs_spiffs, &fsConfig, spiffsWorkBuffer,
        spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
        spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);
    if (status == SPIFFS_ERR_NOT_A_FS)
    {
            SPIFFS_unmount(&fs_spiffs);
            status = SPIFFS_format(&fs_spiffs);
            if (status != SPIFFS_OK)
            {
                while(1);
            }
            status = SPIFFS_mount(&fs_spiffs, &fsConfig, spiffsWorkBuffer,
                spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
                spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);
    }

    Semaphore_post(sem_spiffs_mount);
    Task_exit();
}

void erase_flash()
{
    spiffs_config  fsConfig;
    int32_t        status;

    SPIFFS_unmount(&fs_spiffs);
    status = SPIFFS_format(&fs_spiffs);
    if (status != SPIFFS_OK)
    {
        printf("Error formateo");
    }
    Semaphore_post(sem_reboot);
}

bool log2_spiffs(bool event, char *msg)
{
    spiffs_file fd;
    char p, file[60];
    struct tm now;
    uint32_t size_bytes = 0;
    char *size_string;

    p = event ? 'e' : 's';

    // Get current time and format the file name
    get_ltime(&now);
    sprintf(file, "%cl-%02d%02d%02d.log", p, now.tm_mday, now.tm_mon + 1, now.tm_year - 100);

    // Open or create the file
    fd = SPIFFS_open(&fs_spiffs, file, SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        fd = SPIFFS_open(&fs_spiffs, file, SPIFFS_CREAT | SPIFFS_RDWR, 0);
        if (fd < 0)
        {
            return false;
        }
        else
        {
            save_item_spiffs(file, LOG_PATH);
        }
    }
    // Move the file pointer to the end
    if (SPIFFS_lseek(&fs_spiffs, fd, 0, SPIFFS_SEEK_END) < 0)
    {
        SPIFFS_close(&fs_spiffs, fd);
        return false;
    }
    // Write the message to the file
    size_bytes = SPIFFS_write(&fs_spiffs, fd, msg, strlen(msg));
    if (size_bytes < 0)
    {
        SPIFFS_close(&fs_spiffs, fd);
        return false;
    }

    SPIFFS_close(&fs_spiffs, fd);
    return true;
}

char* format_string_array(char** strings, uint16_t num_elements, const char* end_string)
{

    size_t total_size = 0;
    uint16_t i = 0;

    for (i = 0; i < num_elements - 1; i++)
    {
        total_size += strlen(strings[i]) + 1;
    }

    total_size += strlen(end_string) + 1;
    char* result = calloc(total_size, sizeof(char));
    if (result == NULL)
    {
        return NULL;
    }

     for (i = 0; i < num_elements - 1; i++) {
         strcat(result, strings[i]);
         strcat(result, ",");
     }

     strcat(result, end_string);

     return result;
}

uint16_t digit_counter( uint32_t number)
{
    uint16_t digit_counter = 0;
    while (number > 0)
    {
        number /= 10;
        digit_counter++;
    }
    return digit_counter;
}

bool save_item_spiffs(char *item, char *type)
{
    spiffs_file    fd;
    fd = SPIFFS_open(&fs_spiffs, type, SPIFFS_RDWR, 0);
    if(fd < 0)
    {
        fd = SPIFFS_open(&fs_spiffs, type, SPIFFS_CREAT | SPIFFS_RDWR, 0);
        if (fd < 0)
        {
            return false;
        }
    }
    else {
        if (SPIFFS_lseek(&fs_spiffs, fd, 0, SPIFFS_SEEK_END) < 0)
        {
            return false;
        }
    }
    if (SPIFFS_write(&fs_spiffs, fd, item, strlen(item)) < 0)
    {
        return false;
    }
    if (SPIFFS_write(&fs_spiffs, fd, ",", 1) < 0)
    {
        return false;
    }
    SPIFFS_close(&fs_spiffs, fd);
    return true;
}

void delete_old_files_by_size_spiffs(char *type, float mb_high, float mb_low)
{
    uint16_t counter = 0;
    char paths[MAX_ITEMS][MAX_STR_LEN_2];
    get_elements_file(type, &counter, paths);
    manage_files(paths, &counter, mb_high, mb_low);
}

int32_t get_fileSize(const char* filePath)
{
    spiffs_stat statBuf;
    if (SPIFFS_stat(&fs_spiffs, filePath, &statBuf) == SPIFFS_OK)
    {
        return (int32_t)statBuf.size;
    }
    return -1;
}

char* concatenate_elements(char paths[MAX_ITEMS][MAX_STR_LEN_2], uint16_t lowerIndex, uint16_t upperIndex)
{
    if (lowerIndex >= upperIndex)
    {
        return NULL;  // Return an empty string instead of NULL
    }

    size_t totalSize = 1;  // Start with space for null terminator
    uint16_t i;
    for (i = lowerIndex; i < upperIndex; i++)
    {
        totalSize += strlen(paths[i]) + 1; // +1 for ',' or null terminator
    }

    char *result = malloc(totalSize);
    if (!result) return NULL;

    result[0] = '\0';  // Initialize empty string

    for (i = lowerIndex; i < upperIndex; i++)
    {
        strcat(result, paths[i]);
        if (i < upperIndex - 1) strcat(result, ",");
    }

    return result;
}

// Manage files: delete if exceeding the maximum allowed size
void manage_files(char paths[MAX_ITEMS][MAX_STR_LEN_2], uint16_t *pathCount, float maxMB, float minMB)
{
    const uint32_t maxBytes = (uint32_t)(maxMB * 1024 * 1024);
    const uint32_t minBytes = (uint32_t)(minMB * 1024 * 1024);
    uint32_t totalSize = 0;
    uint16_t newCount = 0;
    uint16_t i;
    // Calculate total file size
    for (i = 0; i < *pathCount; i++)
    {
        int32_t fileSize = get_fileSize(paths[i]);
        if (fileSize < 0)
        {
            System_printf("Error getting file size: %s\n", paths[i]);
            System_flush();
            continue;
        }
        totalSize += (uint32_t)fileSize;
    }

    // If total size exceeds maxBytes, delete files until reaching minBytes
    if (totalSize > maxBytes)
    {
        for (i = 0; i < *pathCount; i++)
        {
            if (totalSize <= minBytes) break;

            int32_t fileSize = get_fileSize(paths[i]);
            if (fileSize < 0) continue;

            if (SPIFFS_remove(&fs_spiffs, paths[i]) == SPIFFS_OK)
            {
                System_printf("File deleted: %s\n", paths[i]);
                System_flush();
                totalSize -= (uint32_t)fileSize;
                newCount++;  // Reduce file count
            } else
            {
                System_printf("Error deleting file: %s\n", paths[i]);
                System_flush();
            }
        }

        // Create the new file list after deletion
        char *paths_new = NULL;
        paths_new = concatenate_elements(paths, newCount, *pathCount);
        if (newCount > 0)
        {
            SPIFFS_remove(&fs_spiffs, LOG_PATH);
            if (paths_new)
            {
                save_item_spiffs(paths_new, LOG_PATH);
                free(paths_new);  // Free allocated memory
            }
        }

        *pathCount = newCount;  // Update the file count
    }
}

void get_elements_file(char *type, uint16_t *counter, char items[MAX_ITEMS][MAX_STR_LEN_2])
{
    spiffs_file fd;
    char *items_string = NULL;
    int32_t data_read = 0;

    fd = SPIFFS_open(&fs_spiffs, type, SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        return;
    }

    items_string = calloc(MAX_NUMBER_BAYTE_ITEM, sizeof(char));
    data_read = SPIFFS_read(&fs_spiffs, fd, items_string, MAX_NUMBER_BAYTE_ITEM);
    if ( data_read < 0)
    {
        SPIFFS_close(&fs_spiffs, fd);
        free(items_string);
        return;
    }

    char items_str[data_read+1];
    strncpy(items_str, items_string, data_read);
    items_str[data_read] = '\0';
    free(items_string);
    *counter = match_counter(items_str, ',');
    if (*counter > 0)
    {
        uint16_t *indexes = calloc(*counter, sizeof(uint16_t));
        get_index_match(items_str, ',', *counter, indexes);
        parse_string(items_str, indexes, *counter, items);
        free(indexes);
    }


    SPIFFS_close(&fs_spiffs, fd);
}

void get_element_file(char *type, uint16_t *counter, char item[MAX_STR_LEN], const uint16_t index)
{
    spiffs_file fd = 1;
    char *items_string = NULL;
    uint32_t data_read = 0;
    SPIFFS_close(&fs_spiffs, fd);
    fd = SPIFFS_open(&fs_spiffs, type, SPIFFS_RDWR, 0);
    if (fd < 0)
    {
        return;
    }

    items_string = calloc(MAX_NUMBER_BAYTE_ITEM, sizeof(char));
    data_read = SPIFFS_read(&fs_spiffs, fd, items_string, MAX_NUMBER_BAYTE_ITEM);
    if ( data_read < 0)
    {
        SPIFFS_close(&fs_spiffs, fd);
        free(items_string);
        return;
    }

    char items_str[data_read];
    strncpy(items_str, items_string, data_read);
    free(items_string);
    *counter = match_counter(items_str, ',');
    if (*counter > 0)
    {
        uint16_t *indexes = calloc(*counter, sizeof(uint16_t));
        get_index_match(items_str, ',', *counter, indexes);
        get_element_string(items_str, indexes, *counter, item, index);
        free(indexes);
    }

    SPIFFS_close(&fs_spiffs, fd);
}

void get_element_string(const char *string, const uint16_t *indexes, uint16_t counter, char item[MAX_STR_LEN], const uint16_t index)
{
    uint16_t start;
    if(index == 0)
    {
        start = 0;
    }
    else
    {
        start = indexes[index - 1] + 1;
    }
    uint16_t length = indexes[index] - start;
    if (length >= MAX_STR_LEN) length = MAX_STR_LEN - 1;

    strncpy(item, string + start, length);
    item[length] = '\0';
}

uint16_t  match_counter(char *string, const char *ch)
{
    char *aux;
    aux = strchr(string, (int)ch);
    if ((aux == NULL) == 0)
    {
        aux++;
        return 1 + match_counter(aux, ch);
    }
    else
    {
        return 0;
    }
}

void get_index_match(const char *string, const char *ch, const uint16_t counter_match, uint16_t *indexes)
{
    uint16_t i;
    const char *aux = string;
    for(i = 0; i < counter_match; i++)
    {
        aux = strchr(aux, (int)ch);
        indexes[i] = (strlen(string) - strlen(aux));
        aux++;
    }
}

void parse_string(const char *string, const uint16_t *indexes, uint16_t counter, char items[MAX_ITEMS][MAX_STR_LEN_2]) {
    uint16_t start = 0;
    uint16_t i;
    for (i = 0; i < counter && i < MAX_ITEMS; i++)
    {
        uint16_t length = indexes[i] - start;
        if (length >= MAX_STR_LEN) length = MAX_STR_LEN - 1;

        strncpy(items[i], string + start, length);
        items[i][length] = '\0';

        start = indexes[i] + 1;
    }
}

bool externalflash_ready()
{
    bool r;
    Semaphore_pend(sem_spiffs_mount, BIOS_WAIT_FOREVER);

    if (fs_spiffs.mounted)
    {
        r = true;
    }
    else
    {
        r = false;
    }

    Semaphore_post(sem_spiffs_mount);
    return r;
}

// Function to check if a file exists
int file_exists(const char* file_path)
{
    spiffs_file fd;

    fd = SPIFFS_open(&fs_spiffs, file_path, SPIFFS_RDWR, 0);

    if (fd < 0) {
        return 0;
    }

    return 1;
}

int16_t send_files()
{
    char item[MAX_STR_LEN];
    uint16_t counter_logs = 0;
    uint16_t counter_templates = 0;

    // Retrieve the number of log and template files
    get_element_file(LOG_PATH, &counter_logs, item, 0);
    get_element_file(TEMPLATE_PATH, &counter_templates, item, 0);

    uint16_t files_sent = 0;
    uint16_t part_number = 1;
    int i = 0;
    int j = 0;
    int k = 0;
    uint16_t total_files = counter_logs + counter_templates + num_static_files;

    while (files_sent < total_files)
    {
        char json[500];  // JSON buffer
        int json_index = 0;
        int written;

        // Start JSON structure
        written = snprintf(json, sizeof(json), "{\"Part\":%d, \"Last\":%d, \"Files\":[", part_number, (files_sent + 10 >= total_files) ? 1 : 0);
        json_index += written;

        int files_in_part = 0;

        // Add log files to JSON in batches of 10
        for (; i < counter_logs && files_in_part < 10; ++i, ++files_sent, ++files_in_part)
        {
            get_element_file(LOG_PATH, &counter_logs, item, i);
            int file_size = get_fileSize(item);
            if (file_size > 0)
            {
                written = snprintf(json + json_index, sizeof(json) - json_index, "{\"Name\":\"%s\", \"Size\":%d}", item, file_size);
                json_index += written;
                if (files_in_part < 4 && files_sent + 1 < total_files)
                {
                    json[json_index++] = ',';
                }
            }
        }

        // Add template files to JSON
        for (; j < counter_templates && files_in_part < 10; ++j, ++files_sent, ++files_in_part)
        {
            get_element_file(TEMPLATE_PATH, &counter_templates, item, j);
            int file_size = get_fileSize(item);
            if (file_size > 0)
            {
                written = snprintf(json + json_index, sizeof(json) - json_index, "{\"Name\":\"%s\", \"Size\":%d}", item, file_size);
                json_index += written;
                if (files_in_part < 4 && files_sent + 1 < total_files)
                {
                    json[json_index++] = ',';
                }
            }
        }

        // Add static files to JSON
        for (; k < num_static_files && files_in_part < 10; ++k, ++files_sent, ++files_in_part)
        {
            if (file_exists(static_files[k]))
            {
                int file_size = get_fileSize(static_files[k]);
                written = snprintf(json + json_index, sizeof(json) - json_index, "{\"Name\":\"%s\", \"Size\":%d}", static_files[k], file_size);
                json_index += written;
                if (files_in_part < 4 && files_sent + 1 < total_files)
                {
                    json[json_index++] = ',';
                }
            }
            else
            {
                printf("Debug: Static file '%s' does not exist, reducing total file count.\n", static_files[k]);
                total_files--;
            }
        }

        // Close JSON structure
        json[json_index++] = ']';
        json[json_index++] = '}';
        json[json_index] = '\0';

        printf("Debug: JSON Payload: %s\n", json);

        // Retrieve MAC address
        char mac[18];
        db_get_mac(mac);

        // Prepare MQTT message
        msg_mqtt_t msg;
        msg.size_content = json_index + 1;
        msg.content = (char*)malloc(msg.size_content);
        if (!msg.content)
        {
            printf("Error: Insufficient memory for msg.content.\n");
            return 0;
        }
        memcpy(msg.content, json, msg.size_content);

        // Prepare MQTT topic with MAC address
        msg.size_topic = 50;
        msg.topic = (char*)malloc(msg.size_topic);
        if (!msg.topic)
        {
            printf("Error: Insufficient memory for msg.topic.\n");
            free(msg.content);
            return 0;
        }
        snprintf(msg.topic, msg.size_topic, "smaway/%s/tocloud/config/124", mac);

        printf("Debug: MQTT Topic: %s\n", msg.topic);

        // Initialize optional buffer
        msg.buf = NULL;
        msg.size_buf = 0;

        // Send message via MQTT
        sendToMqttCheckingMailbox(&msg);
        //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
        Task_sleep(1000);

        printf("Debug: MQTT message sent (Part %d).\n", part_number);

        part_number++;
    }
    return 1;
}


// Function to send a response for file operations
void send_file_operation_response(const char *filename, bool is_format)
{
    char mac[18];
    db_get_mac(mac);  // Retrieve MAC address

    // If formatting, FileName should be an empty string
    const char *file_value = is_format ? "" : filename;

    // Create JSON message with FileName always present
    char json[256];
    snprintf(json, sizeof(json), "{\"Action\":\"%s\", \"FileName\":\"%s\"}",
             is_format ? "Format" : "Delete", file_value);
    json[sizeof(json) - 1] = '\0';  // Ensure null termination

    // Prepare MQTT message structure
    msg_mqtt_t msg;
    msg.size_content = strlen(json) + 1;  // Include null terminator
    msg.content = (char*)malloc(msg.size_content);
    if (!msg.content)
    {
        printf("Error: Insufficient memory for msg.content.\n");
        return;
    }
    memcpy(msg.content, json, msg.size_content);
    msg.content[msg.size_content - 1] = '\0';  // Ensure null termination

    // Allocate memory for the topic
    msg.size_topic = 50;  // Adjust if necessary
    msg.topic = (char*)malloc(msg.size_topic);
    if (!msg.topic)
    {
        printf("Error: Insufficient memory for msg.topic.\n");
        free(msg.content);
        return;
    }

    // Format the topic using the MAC address
    snprintf(msg.topic, msg.size_topic, "smaway/%s/tocloud/config/127", mac);
    msg.topic[msg.size_topic - 1] = '\0';  // Ensure null termination

    // Initialize buffer if necessary
    msg.buf = NULL;
    msg.size_buf = 0;

    // Debug message before sending
    printf("MQTT Message:\nTopic: %s\nContent: %s\n", msg.topic, msg.content);

    // Send MQTT message
    sendToMqttCheckingMailbox(&msg);
    //Mailbox_post(m_mqtt_send, &msg, BIOS_WAIT_FOREVER);
}
