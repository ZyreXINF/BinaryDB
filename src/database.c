#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include "database.h"

#define DB_FOLDER "db"
#define DB_EXTENSION ".khdb"

static int get_executable_dir(char *out, size_t out_size)
{
#ifdef _WIN32
    DWORD len = GetModuleFileNameA(NULL, out, (DWORD)out_size);
    if (len == 0 || len == out_size) {return -1;}
#else
    ssize_t len = readlink("/proc/self/exe", out, out_size - 1);
    if (len == -1) {return -1;}
    out[len] = '\0';
#endif

    char *slash = strrchr(out, '/');
    char *bslash = strrchr(out, '\\');
    if (bslash != NULL && (slash == NULL || bslash > slash)) {slash = bslash;}
    if (slash != NULL) {*slash = '\0';}

    return 0;
}

static int is_valid_db_name(const char *name)
{
    if (name == NULL) {return 0;}

    size_t len = strlen(name);
    if (len == 0 || len > 50) {return 0;}

    for (size_t i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)name[i];
        if (!isalnum(c) && c != '_' && c != '-') {return 0;}
    }

    return 1;
}
int db_resolve_path(const char *name, char *out, size_t out_size)
{
    if (name == NULL || out == NULL) {return -1;}
    if (!is_valid_db_name(name)) {return -3;} /* rejects slashes, "..", dots, etc. */

    char exe_dir[512];
    if (get_executable_dir(exe_dir, sizeof(exe_dir)) != 0) {return -1;}

    char folder[560];
    snprintf(folder, sizeof(folder), "%s/%s", exe_dir, DB_FOLDER);

#ifdef _WIN32
    _mkdir(folder);
#else
    mkdir(folder, 0755);
#endif

    snprintf(out, out_size, "%s/%s%s", folder, name, DB_EXTENSION);

    return 0;
}

int db_create(const char *db_name)
{
    if (db_name == NULL) {return -1;}

    char path[600];
    int resolve_result = db_resolve_path(db_name, path, sizeof(path));
    if (resolve_result != 0) {return resolve_result;}

    FILE *existing = fopen(path, "rb");
    if (existing != NULL)
    {
        fclose(existing);
        return -2; /* file already exists, refuse to overwrite */
    }

    FILE *dir = fopen(path, "wb");
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
}
FILE *db_open(const char *db_name)
{
    if (db_name == NULL) {return NULL;}

    char path[600];
    if (db_resolve_path(db_name, path, sizeof(path)) != 0) {return NULL;}

    return fopen(path, "rb+");
}
int db_close(FILE *dir) {return dir == NULL ? -1 : (!fclose(dir) ? 1 : -1);} //TODO EOF error handling
int db_optimize(FILE *dir)
{
    return 1;
} //TODO Optimize Database via rewriting based on n% of deleted entries

int db_add(BinaryObject *object, FILE *dir)
{
    if (dir == NULL || object == NULL) {return -1;}

    DataBase db;

    fseek(dir, 0, SEEK_SET);
    if (fread(&db, sizeof(DataBase), 1, dir) != 1) {return -1;}

    db.last_id++;
    db.records_amount++;

    fseek(dir, 0, SEEK_SET);
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

int db_list(FILE *dir, BinaryObject **objects, unsigned int *count)
{
    if (dir == NULL || objects == NULL || count == NULL) {return -1;}

    fseek(dir, 0, SEEK_SET);
    DataBase db;
    if (fread(&db, sizeof(DataBase), 1, dir) != 1) {return -1;}

    BinaryObject *arr = NULL;
    if (db.records_amount > 0)
    {
        arr = malloc(sizeof(BinaryObject) * db.records_amount);
        if (arr == NULL) {return -1;}
    }

    unsigned int found = 0;
    BinaryObject object;
    while (found < db.records_amount && fread(&object, sizeof(BinaryObject), 1, dir) == 1)
    {
        if (object.deleted) {continue;}
        arr[found++] = object;
    }

    *objects = arr;
    *count = found;
    return 1;
}