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
