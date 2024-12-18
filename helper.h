#include "lib_tar.h"
#include <string.h>
#include <stdio.h>
// Normalize the path to handle special cases like './', '../', or '//'
void normalize_path(char *path)
{
    char *src = path, *dst = path;
    while (*src)
    {
        if (*src == '/' && *(src + 1) == '/')
        {
            src++; // Ignore duplicate '/'
        }
        else if (*src == '/' && *(src + 1) == '.' && (*(src + 2) == '/' || *(src + 2) == '\0'))
        {
            src += 2; // Ignore '/./'
        }
        else if (*src == '/' && *(src + 1) == '.' && *(src + 2) == '.' && (*(src + 3) == '/' || *(src + 3) == '\0'))
        {
            src += 3; // Ignore '/../'
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

int calculate_chksum(tar_header_t *header) {
    unsigned char *p = (unsigned char *)header;
    int sum = 0;

    // Pour le champ checksum, tous les octets sont considérés comme des espaces
    for (int i = 0; i < 512; i++) {
        if (i >= 148 && i < 156) { // Champs du checksum
            sum += ' ';
        } else {
            sum += p[i];
        }
    }
    return sum;
}

int read_header(int tar_fd, tar_header_t *header) {
    // Lire un bloc de 512 octets depuis le descripteur de fichier
    ssize_t bytes_read = read(tar_fd, header, sizeof(tar_header_t));

    // Vérifier si la lecture a réussi
    if (bytes_read != sizeof(tar_header_t)) {
        return -1; // Erreur si on n'a pas lu exactement un bloc
    }

    // Vérifier si on est arrivé à la fin de l'archive (blocs vides successifs)
    if (header->name[0] == '\0') {
        return 0; // Fin de l'archive
    }

    // Si nécessaire, ajouter d'autres vérifications, comme le checksum
    if (calculate_chksum(header) != TAR_INT(header->chksum)) {
        return -2; // Erreur de checksum
    }

    return 1; // Succès
}

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
