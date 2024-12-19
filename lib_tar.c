#include "lib_tar.h"
#include "helper.h"

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd)
{
    tar_header_t header;
    int valid_headers = 0;

    // Reset file pointer au début
    lseek(tar_fd, 0, SEEK_SET);

    while (read(tar_fd, &header, sizeof(tar_header_t)) > 0 && header.name[0] != '\0')
    {
        // Vérifie la valeur "magic" et "version"
        if (strncmp(header.magic, TMAGIC, TMAGLEN) != 0)
            return -1;
        if (strncmp(header.version, TVERSION, TVERSLEN) != 0)
            return -2;

        // Calcule le checksum
        long int header_chksum = TAR_INT(header.chksum);
        memset(header.chksum, ' ', 8);
        long int chksum_calculated = 0;

        for (int i = 0; i < sizeof(tar_header_t); i++)
        {
            chksum_calculated += ((uint8_t *)&header)[i];
        }

        // Vérifie le checksum
        if (header_chksum != chksum_calculated)
            return -3;

        // Skip file content (alignement des blocs)
        size_t file_size = TAR_INT(header.size);
        size_t skip_blocks = (file_size + 511) / 512; // Aligner à des blocs de 512-bytes
        lseek(tar_fd, skip_blocks * 512, SEEK_CUR);

        valid_headers++;
    }

    // Reset file pointer au début
    lseek(tar_fd, 0, SEEK_SET);
    return valid_headers;
}

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path)
{
    tar_header_t header;
    while (read(tar_fd, &header, sizeof(tar_header_t)) == sizeof(tar_header_t))
    {
        // Check if the header is null => end of archive
        if (header.name[0] == '\0')
        {
            break;
        }

        // Check if the header name matches the path
        if (strncmp(header.name, path, strlen(path)) == 0)
        {
            return 1;
        }
    }
    lseek(tar_fd, 0, SEEK_SET);
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path)
{
    tar_header_t header;
    while (read(tar_fd, &header, sizeof(tar_header_t)) == sizeof(tar_header_t))
    {
        // Check if the header is null => end of archive
        if (header.name[0] == '\0')
        {
            break;
        }

        // Check if the header name matches the path
        if (strncmp(header.name, path, strlen(path)) == 0)
        {
            if (header.typeflag == DIRTYPE)
            { // directory type
                return 1;
            }

            break;
        }
    }
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path)
{
    tar_header_t header;
    while (read(tar_fd, &header, sizeof(tar_header_t)) == sizeof(tar_header_t))
    {
        // Check if the header is null => end of archive
        if (header.name[0] == '\0')
        {
            break;
        }

        // Check if the header name matches the path
        if (strncmp(header.name, path, strlen(path)) == 0)
        {
            if (header.typeflag == REGTYPE || header.typeflag == AREGTYPE)
            {
                // file and regular file type
                return 1; // OK
            }

            break; // L'entrée existe bien MAIS n'est pas un file
        }
    }
    // Reset file pointer au début de l'archive
    lseek(tar_fd, 0, SEEK_SET);
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path)
{
    tar_header_t header;
    while (read(tar_fd, &header, sizeof(tar_header_t)) == sizeof(tar_header_t))
    {
        // Check if the header is null => end of archive
        if (header.name[0] == '\0')
        {
            break;
        }

        // Check if the header name matches the path
        if (strncmp(header.name, path, strlen(path)) == 0)
        {
            if (header.typeflag == SYMTYPE)
            { // symlink type
                return 1;
            }
            break;
        }
    }
    // Reset file pointer au début de l'archive
    lseek(tar_fd, 0, SEEK_SET);
    return 0;
}

/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries)
{
    size_t max_entries = *no_entries; // Nbr max d'entrées ds entries
    size_t i = 0;

    // On change le path pour qu'il finisse toujours par un '/' -> via fct dans helper.h
    char normalized_path[512];
    size_t path_len = normalize_path(path, normalized_path);

    int end = lseek(tar_fd, 0, SEEK_END); // Get la taille tot de l'archive
    lseek(tar_fd, 0, SEEK_SET);

    // Parcours de l'archive jusqu'à end
    while (lseek(tar_fd, 0, SEEK_CUR) < end)
    {
        tar_header_t current; // Stocke le current header
        if (read(tar_fd, &current, sizeof(tar_header_t)) != sizeof(tar_header_t))
        {
            break; // Si on ne peut pas lire un header COMPLET -> break
        }

        // Gestion des liens symboliques
        if (current.typeflag == SYMTYPE && strcmp(current.name, path) == 0)
        {
            // Pointe vers un fichier -> return 0 (non valide)
            if (is_file(tar_fd, current.linkname) == 1)
            {
                return 0;
            }
            // Add '/' si le chemin est un dossier -> puis appel récursif de list (on recommence pour ce nouv chemin)
            char *newPath = strcat(current.linkname, "/");
            if (is_dir(tar_fd, newPath) == 0)
            {
                return 0; // Chemin invalide ou non accessible
            }

            return list(tar_fd, newPath, entries, no_entries);
        }

        // Vérif si chemin a bien été normalisé
        if (strncmp(current.name, normalized_path, path_len) == 0)
        {
            const char *relative_path = current.name + path_len;

            // Si c'est le répertoire lui-même -> on ignore l'entréé (vide ou juste un '/')
            if (strlen(relative_path) == 0 || (strlen(relative_path) == 1 && relative_path[0] == '/'))
            {
                continue;
            }

            // Pass les fichiers dans les sous-répertoires
            const char *slash = strchr(relative_path, '/');
            if (slash != NULL && *(slash + 1) != '\0')
            {
                continue;
            }

            // Si on a encore de la place -> add l'entrée au tab
            if (i < max_entries)
            {
                strncpy(entries[i], current.name, strlen(current.name) + 1);
                i++;
            }
            else
            {
                // Si tab full -> return -1
                lseek(tar_fd, 0, SEEK_SET);
                *no_entries = i;
                return -1;
            }
        }
    }

    // MAJ nbr entrées
    *no_entries = i;
    lseek(tar_fd, 0, SEEK_SET);

    // Aucune entrée -> return 0
    if (i == 0)
    {
        return 0;
    }

    // Sinon, au moins une -> 1
    return 1;
}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len)
{
    tar_header_t header;
    size_t dest_len = *len;
    lseek(tar_fd, 0, SEEK_SET);

    while (read(tar_fd, &header, sizeof(tar_header_t)) == sizeof(tar_header_t) && header.name[0] != '\0')
    {
        if (strcmp(header.name, path) == 0)
        {
            // Handle symlinks
            if (header.typeflag == SYMTYPE || header.typeflag == LNKTYPE)
            {
                return read_file(tar_fd, header.linkname, offset, dest, len);
            }

            // Handle regular files
            if (header.typeflag == AREGTYPE || header.typeflag == REGTYPE)
            {
                size_t file_size = TAR_INT(header.size);

                if (offset >= file_size)
                {
                    *len = 0;
                    return -2;
                }

                lseek(tar_fd, offset, SEEK_CUR);

                size_t to_read = (file_size - offset > dest_len) ? dest_len : file_size - offset;
                ssize_t bytes_read = read(tar_fd, dest, to_read);

                if (bytes_read <= 0)
                {
                    *len = 0;
                    return -1;
                }

                *len = bytes_read;
                return file_size - offset - bytes_read;
            }
        }
    }

    *len = 0; // Path not found or no valid entry
    lseek(tar_fd, 0, SEEK_SET);
    return -1;
}