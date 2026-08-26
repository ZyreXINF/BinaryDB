#include <stdio.h>
#include "database.h"

int db_add(BinaryObject *object, FILE *dir)
{
    if (dir == NULL || object == NULL) {return -1;}

    DataBase db;

    fseek(dir, 0, SEEK_END);
    long size = ftell(dir);

    //TODO File corruption check (not possible amount of bytes written)
    if (size == 0)
    {
        db.next_id = 0;
        db.records_amount = 1;
        db.deleted_records = 0;
        fseek(dir, 0, SEEK_SET);
    }
    else
    {
        fseek(dir, 0, SEEK_SET);
        fread(&db, sizeof(DataBase), 1, dir);
        db.next_id++;
        db.records_amount++;
        fseek(dir, 0, SEEK_SET);
    }
    fwrite(&db, sizeof(DataBase), 1, dir);

    object->id = db.next_id;
    object->deleted = 0;
    fseek(dir, 0, SEEK_END);
    fwrite(object, sizeof(BinaryObject), 1, dir);
    fflush(dir);

    return 1;
}

int db_get( BinaryObject *object, unsigned int id, FILE *dir)
{
    if (dir == NULL || object == NULL) {return -1;}
    fseek(dir, sizeof(DataBase), SEEK_SET);

    BinaryObject temp_obj;
    while (fread(&temp_obj, sizeof(BinaryObject), 1, dir) != 0)
    {
        if (temp_obj.id == id && !temp_obj.deleted)
        {
            *object = temp_obj;
            return 1;
        }
    }
    return 0;
}

int db_update(BinaryObject *object, unsigned int id)
{
    return 1;
    //TODO Update function
}

int db_delete(FILE *dir, BinaryObject *object, unsigned int id)
{
    if (dir == NULL || object == NULL) {return -1;}

    BinaryObject temp_obj;
    bool found = false;

    fseek(dir, sizeof(DataBase), SEEK_SET);
    //Check if id is correct
    while (fread(&temp_obj, sizeof(BinaryObject), 1, dir))
    {
        if (temp_obj.id == id && !temp_obj.deleted)
        {
            found = true;
            break;
        }
    }
    if (!found) {return -1;}

    *object = temp_obj;
    fseek(dir, -(long)sizeof(BinaryObject), SEEK_CUR);
    temp_obj.deleted = 1;
    fwrite(&temp_obj, sizeof(BinaryObject), 1, dir);

    fseek(dir, 0, SEEK_SET);
    DataBase db;
    fread(&db, sizeof(DataBase), 1, dir);
    fseek(dir, 0, SEEK_SET);
    db.records_amount--;
    db.deleted_records++;
    fwrite(&db, sizeof(DataBase), 1, dir);

    fflush(dir);
    return 1;
}

static void print_bits(unsigned char byte)
{
    for (int i = 7; i >= 0; i--)
    {
        printf("%d", (byte >> i) & 1);
    }
}

void db_list(FILE *dir)
{
    if (dir == NULL) {return;}
    fseek(dir, 0, SEEK_SET);
    DataBase db;
    fread(&db,sizeof(DataBase),1,dir);
    printf("Entries: %d\n", db.records_amount);

    BinaryObject object;
    while (fread(&object,sizeof(BinaryObject),1, dir) != 0)
    {
        if (object.deleted){continue;}
        printf("ID: %d; Name: %s; Mask: ", object.id, object.object_name);
        print_bits(object.object_mask);
        printf("\n");
    }
}
