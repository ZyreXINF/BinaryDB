#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif
#include "database.h"

#define DB_FOLDER "db"
#define DB_EXTENSION ".khdb"
#define DB_OPTIMIZATION_THRESHOLD 0.4

static int db_should_optimize(FILE *dir);
static int db_optimize(FILE *dir);

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

    FILE *dir = fopen(path, "rb+");
    if (dir == NULL) {return NULL;}

    if (db_should_optimize(dir) == 1)
    {
        db_optimize(dir);
    }

    return dir;
}
int db_close(FILE *dir) {return dir == NULL ? -1 : (!fclose(dir) ? 1 : -1);} //TODO EOF error handling

static int db_should_optimize(FILE *dir)
{
    if (dir == NULL) {return -1;}

    DataBase db;
    fseek(dir, 0, SEEK_SET);
    if (fread(&db, sizeof(DataBase), 1, dir) != 1) {return -1;}

    unsigned int total = db.records_amount + db.deleted_records;
    if (total == 0) {return 0;}

    double deleted_ratio = (double)db.deleted_records / (double)total;
    return deleted_ratio > DB_OPTIMIZATION_THRESHOLD ? 1 : 0;
}
static int db_optimize(FILE *dir)
{
    if (dir == NULL) {return -1;}

    DataBase db;
    fseek(dir, 0, SEEK_SET);
    if (fread(&db, sizeof(DataBase), 1, dir) != 1) {return -1;}

    long read_pos = (long)sizeof(DataBase);
    long write_pos = (long)sizeof(DataBase);
    unsigned int live_count = 0;

    BinaryObject temp_obj;
    fseek(dir, read_pos, SEEK_SET);
    while (fread(&temp_obj, sizeof(BinaryObject), 1, dir) == 1)
    {
        read_pos += (long)sizeof(BinaryObject);

        if (!temp_obj.deleted)
        {
            if (write_pos != read_pos - (long)sizeof(BinaryObject))
            {
                fseek(dir, write_pos, SEEK_SET);
                fwrite(&temp_obj, sizeof(BinaryObject), 1, dir);
                fseek(dir, read_pos, SEEK_SET);
            }
            write_pos += (long)sizeof(BinaryObject);
            live_count++;
        }
    }

    db.records_amount = live_count;
    db.deleted_records = 0;
    fseek(dir, 0, SEEK_SET);
    fwrite(&db, sizeof(DataBase), 1, dir);
    fflush(dir);

#ifdef _WIN32
    if (_chsize(_fileno(dir), write_pos) != 0) {return -1;}
#else
    if (ftruncate(fileno(dir), write_pos) != 0) {return -1;}
#endif

    return 1;
}

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

    if (db_should_optimize(dir) == 1)
    {
        db_optimize(dir);
    }

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

int db_list_databases(char ***names, unsigned int *count)
{
    if (names == NULL || count == NULL) {return -1;}

    char exe_dir[512];
    if (get_executable_dir(exe_dir, sizeof(exe_dir)) != 0) {return -1;}

    char folder[560];
    snprintf(folder, sizeof(folder), "%s/%s", exe_dir, DB_FOLDER);

    char **arr = NULL;
    unsigned int capacity = 0;
    unsigned int found = 0;
    size_t ext_len = strlen(DB_EXTENSION);

#ifdef _WIN32
    char search_pattern[600];
    snprintf(search_pattern, sizeof(search_pattern), "%s\\*%s", folder, DB_EXTENSION);

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(search_pattern, &find_data);
    if (handle == INVALID_HANDLE_VALUE)
    {
        *names = NULL;
        *count = 0;
        return 1; /* db folder doesn't exist yet / no databases, not an error */
    }

    do
    {
        size_t name_len = strlen(find_data.cFileName);
        if (name_len <= ext_len) {continue;}

        if (found >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            char **tmp = realloc(arr, capacity * sizeof(char *));
            if (tmp == NULL)
            {
                for (unsigned int i = 0; i < found; i++) {free(arr[i]);}
                free(arr);
                FindClose(handle);
                return -1;
            }
            arr = tmp;
        }

        size_t base_len = name_len - ext_len;
        arr[found] = malloc(base_len + 1);
        if (arr[found] == NULL)
        {
            for (unsigned int i = 0; i < found; i++) {free(arr[i]);}
            free(arr);
            FindClose(handle);
            return -1;
        }
        memcpy(arr[found], find_data.cFileName, base_len);
        arr[found][base_len] = '\0';
        found++;
    } while (FindNextFileA(handle, &find_data));

    FindClose(handle);
#else
    DIR *d = opendir(folder);
    if (d == NULL)
    {
        *names = NULL;
        *count = 0;
        return 1; /* db folder doesn't exist yet / no databases, not an error */
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        size_t name_len = strlen(entry->d_name);
        if (name_len <= ext_len) {continue;}
        if (strcmp(entry->d_name + name_len - ext_len, DB_EXTENSION) != 0) {continue;}

        if (found >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            char **tmp = realloc(arr, capacity * sizeof(char *));
            if (tmp == NULL)
            {
                for (unsigned int i = 0; i < found; i++) {free(arr[i]);}
                free(arr);
                closedir(d);
                return -1;
            }
            arr = tmp;
        }

        size_t base_len = name_len - ext_len;
        arr[found] = malloc(base_len + 1);
        if (arr[found] == NULL)
        {
            for (unsigned int i = 0; i < found; i++) {free(arr[i]);}
            free(arr);
            closedir(d);
            return -1;
        }
        memcpy(arr[found], entry->d_name, base_len);
        arr[found][base_len] = '\0';
        found++;
    }
    closedir(d);
#endif

    *names = arr;
    *count = found;
    return 1;
}

void db_free_database_list(char **names, unsigned int count)
{
    if (names == NULL) {return;}
    for (unsigned int i = 0; i < count; i++)
    {
        free(names[i]);
    }
    free(names);
}