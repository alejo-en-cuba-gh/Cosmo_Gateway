/*
 * jsonc.h
 *
 *  Created on: Oct 7, 2024
 *      Author: Lzaro
 */

#ifndef JSONC_H_
#define JSONC_H_

#include <ti/utils/json/json.h>

bool get_template(char *name, char *template, uint16_t *size_template);
bool create_template(char *name, char *template, uint16_t size_template);
bool delete_template(char *name);
bool lis_dir();
bool create_json(Json_Handle *hObject, Json_Handle *hTemplate, char *cmd);
bool parse_json(Json_Handle hObject, char *json_text, uint16_t json_textlen);
char* get_jsonstring(Json_Handle hObject, const char *p_key, uint16_t *value_size);
bool set_jsonstring(Json_Handle hObject, const char *p_key, char *p_value);

#endif /* JSONC_H_ */
