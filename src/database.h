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

int db_create(const char *db_name);
FILE *db_open(const char *db_name);
int db_close(FILE *dir);
int db_optimize(FILE *dir);

int db_add(BinaryObject *object, FILE *dir);
int db_get(BinaryObject *object, unsigned int id, FILE *dir);
int db_update(FILE *dir, BinaryObject *object, unsigned int id);
int db_delete(FILE *dir, BinaryObject *object, unsigned int id);

void db_list(FILE *dir);

#endif