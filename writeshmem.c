#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // Pour sleep()
#include "shared_memory.h"

#define NUM_ITERATIONS 10

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: %s [stuff to write]\n", argv[0]);
        return -1;
    }

    char *block = attach_memory_block(FILENAME, BLOCK_SIZE);
    if (block == NULL) {
        printf("ERROR: couldn't get block\n");
        return -1;
    }

    strncpy(block, argv[1], BLOCK_SIZE);  // Écrit la chaîne initiale

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        printf("Reading: \"%s\"\n", block);  // Affiche le contenu de la mémoire
        sleep(1);  // Pause d'1 seconde
    }

    detach_memory_block(block);
    return 0;
}
