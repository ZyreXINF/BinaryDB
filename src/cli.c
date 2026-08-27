#include <stdio.h>
#include <string.h>
#include "cli.h"
#include "database.h"

static void cli_parse();
static void cli_execute(FILE *dir, int argc, char **argv);

static void cmd_add(FILE *dir, int argc, char **argv);
static void cmd_get(FILE *dir, int argc, char **argv);
static void cmd_update(FILE *dir, int argc, char **argv);
static void cmd_delete(FILE *dir, int argc, char **argv);
static void cmd_list(FILE *dir, int argc, char **argv);
static void cmd_help();

#define INPUT_SIZE 256
#define MAX_ARGS 10



static void cli_parse();
static void cli_execute(FILE *dir, int argc, char **argv)
{
    if (strcmp(argv[0], "add") == 0)
    {
        cmd_add(dir, argc, argv);
    }
    else if (strcmp(argv[0], "get") == 0)
    {
        cmd_get(dir, argc, argv);
    }
    else if (strcmp(argv[0], "update") == 0)
    {
        cmd_update(dir, argc, argv);
    }
    else if (strcmp(argv[0], "delete") == 0)
    {
        cmd_delete(dir, argc, argv);
    }
    else if (strcmp(argv[0], "list") == 0)
    {
        cmd_list(dir, argc, argv);
    }
    else if (strcmp(argv[0], "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(argv[0], "quit") == 0)
    {
        // signal exit
    }
    else
    {
        printf("Unknown command: %s\n", argv[0]);
    }
}

static void cmd_add(FILE *dir, int argc, char **argv){}
static void cmd_get(FILE *dir, int argc, char **argv){}
static void cmd_update(FILE *dir, int argc, char **argv){}
static void cmd_delete(FILE *dir, int argc, char **argv){}
static void cmd_list(FILE *dir, int argc, char **argv){}
static void cmd_help(){}

void cli_run()
{
    FILE *dir = NULL;
    char current_db[64] = "";

}