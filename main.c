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

    BinaryObject obj1 = {
        .object_name = "Firewall",
        .object_mask = 0b00001101
    };
    BinaryObject obj2 = {
        .object_name = "HoneyPot",
        .object_mask = 0b11000001
    };

    db_add(&obj1, fileptr);
    db_add(&obj2, fileptr);

    db_list(fileptr);

    fclose(fileptr);
    return 0;
}
