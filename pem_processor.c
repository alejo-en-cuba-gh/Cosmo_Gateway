/*
 * pem_processor.c
 *
 *  Created on: 19 Dec 2024
 *      Author: Lzaro
 */


#include "pem_processor.h"


static const unsigned char base64_table[256] = {
    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,  ['G'] = 6,  ['H'] = 7,
    ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11, ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
    ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31,
    ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39,
    ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
    ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59, ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63
};

#define begin_delim "-----BEGIN CERTIFICATE-----"
#define end_delim   "-----END CERTIFICATE-----"

bool validate_certificate(const char *pem_data) {
    const char *start = strstr(pem_data, begin_delim);
    const char *end = strstr(pem_data, end_delim);
    struct tm current_time;

    if (!start || !end) {
        printf("Delimiters not found: %s ... %s\n", begin_delim, end_delim);
        return false;
    }

    start += strlen(begin_delim);
    while (*start == '\n' || *start == '\r' || *start == ' ') start++;

    size_t pem_length = end - start;

    uint64_t expiry_date = extract_expiry_date(start, pem_length);

    if (expiry_date == 0) {
        printf("Expiration date not found\n");
        return false;
    }

    get_ltime(&current_time);
    uint64_t current_time_number = convert_tm_to_number(&current_time);

    if (expiry_date < current_time_number) {
        return false;
    }

    return true;
}

uint64_t extract_expiry_date(const char *input, size_t input_length) {
    unsigned char output[2048];
    if (input == NULL || output == NULL) return 0;

    size_t output_index = 0;
    unsigned int buffer = 0;
    int buffer_bits = 0;

    size_t i;
    for (i = 0; i < input_length; i++) {
        unsigned char c = input[i];

        if (c == '=' || c == '\r' || c == '\n' || c == ' ') {
            continue;
        }

        if (c > 255 || base64_table[c] == 0 && c != 'A') {
            return 0;
        }

        buffer = (buffer << 6) | base64_table[c];
        buffer_bits += 6;

        if (buffer_bits >= 8) {
            buffer_bits -= 8;
            if (output_index >= 2048) return 0;
            output[output_index++] = (buffer >> buffer_bits) & 0xFF;
        }
    }

    char expiry_date[16];

    const unsigned char validity_identifier[] = {0x30}; // SEQUENCE
    const unsigned char utc_time_identifier = 0x17;     // UTCTime
    const unsigned char gen_time_identifier = 0x18;     // GeneralizedTime

    const unsigned char *ptr = output;
    const unsigned char *end = output + (int)output_index;

    while (ptr < end) {
        if (*ptr == validity_identifier[0] && ptr + 1 < end && *(ptr + 1) == 0x1E) {
            ptr += 2;
            break;
        }
        ptr++;
    }

    if (ptr >= end) {
        printf("Validity section not found.\n");
        return 0;
    }

    if (ptr + 2 < end && *ptr == utc_time_identifier) {
        // UTCTime (13 bytes)
        size_t time_length = 12;
        if (ptr + time_length > end) {
            printf("Insufficient data for notAfter.\n");
            return 0;
        }
        strncpy(expiry_date, (const char *)ptr + 17, time_length);
        expiry_date[time_length] = '\0';
        printf("Expiration date: %s\n", expiry_date);
        return strtoull(expiry_date, NULL, 10);
    } else if (ptr + 2 < end && *ptr == gen_time_identifier) {
        // GeneralizedTime (15 bytes)
        size_t time_length = 14;
        if (ptr + time_length > end) {
            printf("Insufficient data for notAfter.\n");
            return 0;
        }
        strncpy(expiry_date, (const char *)ptr + 17, time_length);
        expiry_date[time_length] = '\0';
        printf("Expiration date: %s\n", expiry_date);
        return strtoull(expiry_date, NULL, 10);
    }

    printf("Unsupported time format.\n");
    return 0;
}

uint64_t convert_tm_to_number(const struct tm *time_info) {
    uint64_t formatted_time = 0;

    formatted_time =
        ((time_info->tm_year % 100) * 10000000000LL) +
        ((time_info->tm_mon + 1) * 100000000LL) +
        (time_info->tm_mday * 1000000LL) +
        (time_info->tm_hour * 10000LL) +
        (time_info->tm_min * 100LL) +
        (time_info->tm_sec);

    return formatted_time;
}

int base64_decode(const unsigned char *input, size_t input_length, unsigned char *output) {
    if (input == NULL || output == NULL) return 0;

    size_t output_index = 0;
    unsigned int buffer = 0;
    int buffer_bits = 0;

    size_t i;
    for (i = 0; i < input_length; i++) {
        unsigned char c = input[i];

        // Omitir caracteres no válidos en Base64
        if (c == '=' || c == '\r' || c == '\n' || c == ' ') {
            continue;
        }

        // Verificar que el carácter esté en la tabla de Base64
        if (c > 255 || base64_table[c] == 0 && c != 'A') {
            return 0;
        }

        // Acumular los bits del carácter decodificado
        buffer = (buffer << 6) | base64_table[c];
        buffer_bits += 6;

        // Cuando hay al menos 8 bits, extraer el byte
        if (buffer_bits >= 8) {
            buffer_bits -= 8;
            if (output_index >= 2048) return 0; // Verificar desbordamiento de buffer
            output[output_index++] = (buffer >> buffer_bits) & 0xFF;
        }
    }

    return output_index; // Retorna la cantidad de bytes escritos en el output
}

int pem_to_der(const unsigned char *pem_data, size_t pem_length, unsigned char *der_data) {
    if (pem_data == NULL || der_data == NULL) return 0;

    size_t i = 0;
    size_t output_index = 0;

    // Buscar el comienzo del certificado (después de "-----BEGIN CERTIFICATE-----")
    while (i < pem_length && (pem_data[i] != '\n')) {
        i++;
    }
    i++; // Para saltar la línea nueva

    // Buscar el final del certificado (antes de "-----END CERTIFICATE-----")
    size_t start_index = i;
    while (i < pem_length && !(pem_data[i] == '-' && pem_data[i+1] == '-' && pem_data[i+2] == '-' && pem_data[i+3] == '-')) {
        i++;
    }

    size_t cert_data_len = i - start_index;
    return base64_decode(&pem_data[start_index], cert_data_len, der_data);
}
