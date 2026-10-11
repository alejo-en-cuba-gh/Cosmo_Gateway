/*
 * pem_processor.h
 *
 *  Created on: 19 Dec 2024
 *      Author: Lzaro
 */

#ifndef PEM_PROCESSOR_H_
#define PEM_PROCESSOR_H_


#include "common.h"

bool validate_certificate(const char *pem_data);
uint64_t extract_expiry_date(const char *input, size_t input_len);
uint64_t convert_tm_to_number(const struct tm *time_info);
int base64_decode(const unsigned char *input, size_t input_length, unsigned char *output);
int pem_to_der(const unsigned char *pem_data, size_t pem_length, unsigned char *der_data);

#endif /* PEM_PROCESSOR_H_ */
