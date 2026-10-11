/*
 * config-db.h
 *
 *  Created on: 25 ott 2023
 */

#ifndef CONFIG_DB_H_
#define CONFIG_DB_H_

#define DBEV_OVERHEAD 10

bool db_add_item_raw(int item_size, dbt_generic **db_item, dbt_generic **last_item);
static uint16_t get_offset(char **item, char** element);
void empty_table(dbt_generic **item, dbt_generic **last, db_type type, bool handle_sem);
static int free_element(dbt_generic **e, db_type type);
static int get_el_size(db_type type);
static uint8_t get_next_acid ();
bool check_cfg_file(char *file);
bool copy_file(const char *src, const char *dst);
uint8_t import_config(dbt_db *db, char *file_name);
uint8_t export_config(dbt_db *db, char *file_name);


void sem_db_start_read ();
void sem_db_end_read ();

#endif /* CONFIG_DB_H_ */
