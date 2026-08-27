#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.c"
#include "cli.h"
#include "database.h"

#define INPUT_SIZE 256
#define MAX_ARGS 10

static void cli_parse(char *input, char **argv, int *argc);
static int  cli_execute(FILE **dir, char *current_db, int argc, char **argv);

static void cmd_create(FILE **dir, char *current_db, int argc, char **argv);
static void cmd_open(FILE **dir, char *current_db, int argc, char **argv);
static void cmd_add(FILE *dir, int argc, char **argv);
static void cmd_get(FILE *dir, int argc, char **argv);
static void cmd_update(FILE *dir, int argc, char **argv);
static void cmd_delete(FILE *dir, int argc, char **argv);
static void cmd_list(FILE *dir, int argc, char **argv);
static void cmd_help(void);
static int  require_open_db(FILE *dir);
static void print_bits(unsigned char byte);
static int  parse_binary_mask(const char *str, unsigned char *out);

static void cli_parse(char *input, char **argv, int *argc)
{
    *argc = 0;

    char *token = strtok(input, " \t\r\n");
    while (token != NULL && *argc < MAX_ARGS)
    {
        argv[(*argc)++] = token;
        token = strtok(NULL, " \t\r\n");
    }
}
static int cli_execute(FILE **dir, char *current_db, int argc, char **argv)
{
    if (strcmp(argv[0], "create") == 0)
    {
        cmd_create(dir, current_db, argc, argv);
    }
    else if (strcmp(argv[0], "open") == 0)
    {
        cmd_open(dir, current_db, argc, argv);
    }
    else if (strcmp(argv[0], "add") == 0)
    {
        cmd_add(*dir, argc, argv);
    }
    else if (strcmp(argv[0], "get") == 0)
    {
        cmd_get(*dir, argc, argv);
    }
    else if (strcmp(argv[0], "update") == 0)
    {
        cmd_update(*dir, argc, argv);
    }
    else if (strcmp(argv[0], "delete") == 0)
    {
        cmd_delete(*dir, argc, argv);
    }
    else if (strcmp(argv[0], "list") == 0)
    {
        cmd_list(*dir, argc, argv);
    }
    else if (strcmp(argv[0], "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(argv[0], "quit") == 0)
    {
        return 0; /* signal exit */
    }
    else
    {
        printf("Unknown command: %s\n", argv[0]);
    }

    return 1;
}

static int require_open_db(FILE *dir)
{
    if (dir == NULL)
    {
        printf("No database open. Use 'create <file>' or 'open <file>' first.\n");
        return 0;
    }
    return 1;
}

static void cmd_create(FILE **dir, char *current_db, int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: create <name> (letters/digits/'_'/'-' only, no extension)\n");
        return;
    }

    int result = db_create(argv[1]);
    if (result == -2)
    {
        printf("A database named '%s' already exists. Use 'open %s' to use it instead.\n", argv[1], argv[1]);
        return;
    }
    if (result == -3)
    {
        printf("Invalid name '%s'. Use only letters, digits, '_' or '-' (no slashes or dots).\n", argv[1]);
        return;
    }
    if (result != 1)
    {
        printf("Failed to create database '%s'\n", argv[1]);
        return;
    }

    if (*dir != NULL)
    {
        db_close(*dir);
    }

    *dir = db_open(argv[1]);
    if (*dir == NULL)
    {
        printf("Created '%s' but failed to open it\n", argv[1]);
        current_db[0] = '\0';
        return;
    }

    strncpy(current_db, argv[1], 63);
    current_db[63] = '\0';
    printf("Created and opened database '%s'\n", argv[1]);
}
static void cmd_open(FILE **dir, char *current_db, int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: open <name> (letters/digits/'_'/'-' only, no extension)\n");
        return;
    }

    FILE *new_dir = db_open(argv[1]);
    if (new_dir == NULL)
    {
        printf("Failed to open database '%s'\n", argv[1]);
        return;
    }

    if (*dir != NULL)
    {
        db_close(*dir);
    }

    *dir = new_dir;
    strncpy(current_db, argv[1], 63);
    current_db[63] = '\0';
    printf("Opened database '%s'\n", argv[1]);
}
static void cmd_add(FILE *dir, int argc, char **argv)
{
    if (!require_open_db(dir)) return;

    if (argc < 3)
    {
        printf("Usage: add <name> <mask> (mask is 8 binary digits, e.g. 01010101)\n");
        return;
    }

    BinaryObject obj;
    memset(&obj, 0, sizeof(obj));

    strncpy(obj.object_name, argv[1], sizeof(obj.object_name) - 1);
    obj.object_name[sizeof(obj.object_name) - 1] = '\0';

    if (parse_binary_mask(argv[2], &obj.object_mask) != 0)
    {
        printf("Mask must be exactly 8 binary digits (0s and 1s), e.g. 01010101\n");
        return;
    }

    if (db_add(&obj, dir) == 1)
    {
        printf("Added '%s' with id %u\n", obj.object_name, obj.id);
    }
    else
    {
        printf("Failed to add record\n");
    }
}
static void cmd_get(FILE *dir, int argc, char **argv)
{
    if (!require_open_db(dir)) return;

    if (argc < 2)
    {
        printf("Usage: get <id>\n");
        return;
    }

    char *endptr;
    unsigned long id = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0')
    {
        printf("Invalid id '%s'\n", argv[1]);
        return;
    }

    BinaryObject obj;
    int result = db_get(&obj, (unsigned int)id, dir);
    if (result == 1)
    {
        printf("ID: %u; Name: %s; Mask: ", obj.id, obj.object_name);
        print_bits(obj.object_mask);
        printf("\n");
    }
    else if (result == 0)
    {
        printf("No record found with id %lu\n", id);
    }
    else
    {
        printf("Error reading database\n");
    }
}
static void cmd_update(FILE *dir, int argc, char **argv)
{
    if (!require_open_db(dir)) return;

    if (argc < 4)
    {
        printf("Usage: update <id> <name> <mask> (mask is 8 binary digits, e.g. 01010101)\n");
        return;
    }

    char *endptr;
    unsigned long id = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0')
    {
        printf("Invalid id '%s'\n", argv[1]);
        return;
    }

    BinaryObject obj;
    memset(&obj, 0, sizeof(obj));
    strncpy(obj.object_name, argv[2], sizeof(obj.object_name) - 1);
    obj.object_name[sizeof(obj.object_name) - 1] = '\0';

    if (parse_binary_mask(argv[3], &obj.object_mask) != 0)
    {
        printf("Mask must be exactly 8 binary digits (0s and 1s), e.g. 01010101\n");
        return;
    }

    int result = db_update(dir, &obj, (unsigned int)id);
    if (result == 1)
    {
        printf("Updated id %lu\n", id);
    }
    else if (result == 0)
    {
        printf("No record found with id %lu\n", id);
    }
    else
    {
        printf("Error updating database\n");
    }
}
static void cmd_delete(FILE *dir, int argc, char **argv)
{
    if (!require_open_db(dir)) return;

    if (argc < 2)
    {
        printf("Usage: delete <id>\n");
        return;
    }

    char *endptr;
    unsigned long id = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0')
    {
        printf("Invalid id '%s'\n", argv[1]);
        return;
    }

    BinaryObject obj;
    int result = db_delete(dir, &obj, (unsigned int)id);
    if (result == 1)
    {
        printf("Deleted id %lu ('%s')\n", id, obj.object_name);
    }
    else if (result == 0)
    {
        printf("No record found with id %lu\n", id);
    }
    else
    {
        printf("Error deleting from database\n");
    }
}

static void print_bits(unsigned char byte)
{
    for (int i = 7; i >= 0; i--)
    {
        printf("%d", (byte >> i) & 1);
    }
}
static int parse_binary_mask(const char *str, unsigned char *out)
{
    if (strlen(str) != 8) return -1;

    unsigned char value = 0;
    for (int i = 0; i < 8; i++)
    {
        if (str[i] != '0' && str[i] != '1') return -1;
        value = (unsigned char)((value << 1) | (str[i] - '0'));
    }

    *out = value;
    return 0;
}
static void cmd_list(FILE *dir, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!require_open_db(dir)) return;

    BinaryObject *objects = NULL;
    unsigned int count = 0;

    if (db_list(dir, &objects, &count) != 1)
    {
        printf("Failed to read database\n");
        return;
    }

    printf("Entries: %u\n", count);
    for (unsigned int i = 0; i < count; i++)
    {
        printf("ID: %u; Name: %s; Mask: ", objects[i].id, objects[i].object_name);
        print_bits(objects[i].object_mask);
        printf("\n");
    }

    free(objects);
}

static void cmd_help(void)
{
    printf("Available commands:\n");
    printf("  create <name>              create a new database and open it (no extension, no slashes/dots)\n");
    printf("  open <name>                open an existing database\n");
    printf("  add <name> <mask>          add a record (mask is 8 binary digits, e.g. 01010101)\n");
    printf("  get <id>                   print a record by id\n");
    printf("  update <id> <name> <mask>  overwrite a record by id (mask is 8 binary digits)\n");
    printf("  delete <id>                soft-delete a record by id\n");
    printf("  list                       list all records\n");
    printf("  help                       show this message\n");
    printf("  quit                       exit the program\n");
}

void cli_run(void)
{
    FILE *dir = NULL;
    char current_db[64] = "";
    char input[INPUT_SIZE];
    char *argv[MAX_ARGS];
    int argc;
    int running = 1;

    printf("BinaryDB v1.0\n");
    printf("Type 'help' for available commands.\n\n");

    while (running)
    {
        printf("%s> ", current_db[0] ? current_db : "(no db)");
        fflush(stdout);

        if (fgets(input, INPUT_SIZE, stdin) == NULL)
        {
            break;
        }

        cli_parse(input, argv, &argc);
        if (argc == 0)
        {
            continue;
        }

        running = cli_execute(&dir, current_db, argc, argv);
    }

    if (dir != NULL)
    {
        db_close(dir);
    }
}