#include <stdio.h>
#include "database.h"

int db_add(BinaryObject *object, FILE *dir)
{
    if (dir == NULL)
    {
        return -1;
    }

    DataBase db;

    fseek(dir, 0, SEEK_END);
    long size = ftell(dir);

    if (size == 0)
    {
        db.cid = 0;
        fseek(dir, 0, SEEK_SET);
        fwrite(&db, sizeof(DataBase), 1, dir);
        object->id = db.cid;
    }
    else
    {
        fseek(dir, 0, SEEK_SET);
        fread(&db, sizeof(DataBase), 1, dir);
        db.cid++;
        fseek(dir, 0, SEEK_SET);
        fwrite(&db, sizeof(DataBase), 1, dir);
        object->id = db.cid;
    }

    fseek(dir, 0, SEEK_END);
    fwrite(object, sizeof(BinaryObject), 1, dir);
    fflush(dir);

    return 1;
}

int db_get(BinaryObject *object, int id, FILE *dir)
{
    fseek(dir, sizeof(DataBase), SEEK_SET);

    BinaryObject temp_obj;
    while (fread(&temp_obj, sizeof(BinaryObject), 1, dir) != 0)
    {
        if (temp_obj.id == id)
        {
            *object = temp_obj;
            return 1;
        }
    }
    return 0;
}
int db_update(int id, BinaryObject *object)
{
    //TODO Update function
}
int db_delete(int id)
{
    //TODO Delete function
}

void print_bits(unsigned char byte)
{
    for (int i = 7; i >= 0; i--)
    {
        printf("%d", (byte >> i) & 1);
    }
}
void db_list(FILE *dir)
{
    fseek(dir, 0, SEEK_SET);
    DataBase db;
    fread(&db,sizeof(DataBase),1,dir);
    printf("Entries: %d\n", db.cid+1);

    BinaryObject object;
    while (fread(&object,sizeof(BinaryObject),1, dir) != 0)
    {
        printf("ID: %d; Name: %s; Mask: ", object.id, object.object_name);
        print_bits(object.object_mask);
        printf("\n");
    }
}
