#include <stdio.h>
#include "database.c"
#include "database.h"

//TODO Build CLI

int main(void)
{
    FILE *fileptr;
    fileptr = fopen("bin.khdb", "rb+");
    if (fileptr == NULL) {
        fileptr = fopen("bin.khdb", "w+b");
    }
    //TODO Optimize the db with n % of deleted entries

    db_list(fileptr);

    fclose(fileptr);
    return 0;
}
