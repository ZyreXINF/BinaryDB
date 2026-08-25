#ifndef DATABASE_H
#define DATABASE_H

typedef struct
{
    unsigned int cid;
} DataBase;
typedef struct
{
    unsigned int id;
    char object_name[32];
    unsigned char object_mask;
} BinaryObject;

#endif