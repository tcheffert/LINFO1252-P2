#include "lib_tar.h"
#include <string.h>
#include <stdio.h>


/**
 * Counts the number of slashes in a given path.
 */
int count_slashes(char *path) {
    int count = 0;
    for (size_t i = 0; i < strlen(path); i++) {
        if (path[i] == '/') count++;
    }
    return count;
}
