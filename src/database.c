#include "database.h"

int db_create(const char *db_name)
{
    if (db_name == NULL) {return -1;}

    FILE *dir = fopen(db_name, "wb");
    if (dir == NULL) {return -1;}

    DataBase db = {
        .last_id = 0,
        .records_amount = 0,
        .deleted_records = 0
    };
    if (!fwrite(&db, sizeof(DataBase), 1, dir))
    {
        fclose(dir);
        return -1;
    }
    fclose(dir);

    return 1;
} //TODO Handle existing db
FILE *db_open(const char *db_name) {return db_name == NULL ? NULL : fopen(db_name, "rb+");}
int db_close(FILE *dir) {return dir == NULL ? -1 : (!fclose(dir) ? 1 : -1);} //TODO EOF error handling
int db_optimize(FILE *dir)
{
    return 1;
} //TODO Optimize Database via rewriting based on n% of deleted entries

int db_add(BinaryObject *object, FILE *dir)
{
    if (dir == NULL || object == NULL) {return -1;}

    DataBase db;

    fseek(dir, 0, SEEK_END);
    long size = ftell(dir);

    // TODO Change to File corruption check (not possible amount of bytes written), former if can be taken down (create() writes header instead)
    if (size == 0)
    {
        db.last_id = 0;
        db.records_amount = 1;
        db.deleted_records = 0;
        fseek(dir, 0, SEEK_SET);
    }
    else
    {
        fseek(dir, 0, SEEK_SET);
        fread(&db, sizeof(DataBase), 1, dir);
        db.last_id++;
        db.records_amount++;
        fseek(dir, 0, SEEK_SET);
    }
    fwrite(&db, sizeof(DataBase), 1, dir);

    object->id = db.last_id;
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
int db_update(FILE *dir, BinaryObject *object, unsigned int id)
{
    if (dir == NULL || object == NULL) {return -1;}
    BinaryObject temp_obj;
    bool found = false;

    fseek(dir, sizeof(DataBase), SEEK_SET);
    while (fread(&temp_obj, sizeof(BinaryObject), 1, dir))
    {
        if (temp_obj.id == id && !temp_obj.deleted)
        {
            found = true;
            break;
        }
    }
    if (!found) {return 0;}

    object->id = temp_obj.id;
    object->deleted = 0;
    fseek(dir, -(long)sizeof(BinaryObject), SEEK_CUR);
    fwrite(object, sizeof(BinaryObject), 1, dir);

    fflush(dir);
    return 1;
}
int db_delete(FILE *dir, BinaryObject *object, unsigned int id)
{
    if (dir == NULL || object == NULL) {return -1;}

    BinaryObject temp_obj;
    bool found = false;

    fseek(dir, sizeof(DataBase), SEEK_SET);
    while (fread(&temp_obj, sizeof(BinaryObject), 1, dir))
    {
        if (temp_obj.id == id && !temp_obj.deleted)
        {
            found = true;
            break;
        }
    }
    if (!found) {return 0;}

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
//TODO Redesign (no cli)
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