#include "lib_tar.h"
#include <string.h>
#include <stdio.h>


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
