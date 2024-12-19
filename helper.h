#include "lib_tar.h"
#include <string.h>
#include <stdio.h>

/**
 * Modif le path pour qu'il finissent toujours par '/'
 *
 * @param path Le path
 * @param normalized_path Buffer pour stocker le new path modifié
 *                        Doit être assez grand pour contenir le résultat, donc, au moins 512 bytes
 * @return La longueur du path modifié avec '/'
 */
size_t normalize_path(const char *path, char *normalized_path) {
    size_t path_len = strlen(path);
    if (path[path_len - 1] != '/') {
        snprintf(normalized_path, 512, "%s/", path);
        return path_len + 1; // Length update avec '/'
    } else {
        strncpy(normalized_path, path, 512);
        return path_len; // Length originale
    }
}
