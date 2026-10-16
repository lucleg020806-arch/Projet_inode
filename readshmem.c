#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared_memory.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: %s [string to overflow]\n", argv[0]);
        return -1;
    }

    char *block = attach_memory_block(FILENAME, BLOCK_SIZE);
    if (block == NULL) {
        printf("ERROR: couldn't get block\n");
        return -1;
    }

    // Écriture au-delà de BLOCK_SIZE (Buffer Overflow)
    strcpy(block, argv[1]);  // Pas de limite : risque de débordement !

    detach_memory_block(block);
    return 0;
}
