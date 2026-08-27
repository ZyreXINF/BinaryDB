#include <stdio.h>
#include "database.c"
#include "cli.c"


int main(void)
{
    FILE *fileptr;
    fileptr = fopen("../db/bin.khdb", "rb+");
    if (fileptr == NULL) {
        fileptr = fopen("../db/bin.khdb", "w+b");
    }

    db_list(fileptr);

    fclose(fileptr);
    return 0;
}
