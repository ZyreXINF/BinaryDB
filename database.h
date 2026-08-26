#ifndef DATABASE_H
#define DATABASE_H

typedef struct
{
    unsigned int next_id;
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

#endif