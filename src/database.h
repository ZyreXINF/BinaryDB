#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>

typedef struct
{
    unsigned int last_id;
    unsigned int records_amount;
    unsigned int deleted_records;
} DataBase;
typedef struct
{
    unsigned int id;
    char object_name[32];
    unsigned char object_mask;
    unsigned char deleted;
} BinaryObject;

int db_resolve_path(const char *name, char *out, size_t out_size);
int db_create(const char *db_name);
FILE *db_open(const char *db_name);
int db_close(FILE *dir);

int db_add(BinaryObject *object, FILE *dir);
int db_get(BinaryObject *object, unsigned int id, FILE *dir);
int db_update(FILE *dir, BinaryObject *object, unsigned int id);
int db_delete(FILE *dir, BinaryObject *object, unsigned int id);

int db_list(FILE *dir, BinaryObject **objects, unsigned int *count);
int db_list_databases(char ***names, unsigned int *count);
void db_free_database_list(char **names, unsigned int count);

#endif