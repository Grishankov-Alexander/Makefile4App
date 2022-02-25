/**
 * Usage: ./finddeps.exe <file.c> <includedir> [includedir ...]
 * 
 * Print dependency headers of specified source file
 *      to the stdout.
 */




#include <stdio.h> /* fscanf, fprintf, fseek, fgetc */
#include <stdlib.h> /* NULL, malloc, free */
#include <stddef.h> /* size_t */
#include <stdbool.h> /* bool */
#include <string.h> /* strlen, strcpy, strcat */




#define IGNORE_RETURN_VALUE(fncall) (void) ((fncall) + 1)
#define IGNORE_PTRTYPE_WARNING(pointer) (void *) (pointer)




/* Returns true if string str is in string array arr */
static bool strInArr(const char *str, const char * const *arr);

/**
 * Recursive algorithm for finding dependency headers
 * and writing them into array *deps_p.
 * 
 * fn - filename in which to search for dependencies.
 * incdirs - array of directories that may contain headers.
 *      Should not end with '/' !
 * deps_p - address of the array with all found header names.
 */
static void finddeps(const char *fn, const char * const *incdirs, char ***deps_p);




int main(int argc, char **argv)
{
    char **deps;
    size_t num_deps;

    if (argc < 3)
        return 1;

    deps = malloc(sizeof(*deps));
    *deps = NULL;

    finddeps(*(argv + 1), IGNORE_PTRTYPE_WARNING(argv + 2), &deps);

    /* print deps */
    for (num_deps = 0; deps[num_deps]; num_deps++) {
        fprintf(stdout, " %s", deps[num_deps]);
        free(deps[num_deps]);
    }

    free(deps);

    return 0;
}




static bool strInArr(const char *str, const char * const *arr)
{
    while (*arr)
        if (strcmp(str, *arr++) == 0)
            return true;
    return false;
}


static void finddeps(const char *fn, const char * const *incdirs, char ***deps_p)
{
    FILE *f, *h;
    int b1, b2; /* For computing number of characters in the header name */
    int num_assigned; /* stores 1 on match with '#include "NAME.h"' */
    char dquote; /* dummy var: gets assigned '"' when header is found */
    char *hn; /* header name */
    char *fhn; /* full header name with path */
    size_t num_deps; /* number of saved dependencies */
    size_t id_i; /* index into incdirs */

    if (!(f = fopen(fn, "rb")))
        return;

    while ( /* scan for #include "*.h"  directive */
        (num_assigned = fscanf(f, " # include \"%n%*[^\"]%n%c", &b1, &b2, &dquote))
        != EOF
    ) {
        /* if match not found - skip line and continue */
        if (!num_assigned) {
            IGNORE_RETURN_VALUE(fscanf(f, "%*[^\n]"));
            fgetc(f);
            continue;
        }

        /* point file position to the beginning of the header name */
        fseek(f, -(b2 - b1 + 1), SEEK_CUR);
        hn = malloc(b2 - b1 + 1); /* header file name */
        IGNORE_RETURN_VALUE(fscanf(f, "%[^\"]", hn));

        for (id_i = 0; incdirs[id_i] != NULL; id_i++) {
            /* save full header name into fhn */
            fhn = malloc(strlen(incdirs[id_i]) + strlen(hn) + 2); /* 2 for '/' and '\0' */
            strcpy(fhn, incdirs[id_i]); /* copy dir name */
            strcat(fhn, "/"); /* concat '/' to dir name */
            strcat(fhn, hn); /* concat header name to dir */

            /* Does it already exists ? */
            if (strInArr(fhn, IGNORE_PTRTYPE_WARNING(*deps_p))) {
                free(fhn);
                break;
            }

            /* Can it be opened ? */
            if (!(h = fopen(fhn, "rb"))) {
                free(fhn);
                continue;
            }
            fclose(h);

            /* Save it */
            for (num_deps = 0; (*deps_p)[num_deps]; num_deps++)
                ;
            *deps_p = realloc(*deps_p, sizeof(**deps_p) * (num_deps + 2)); /* 1 - for fhn, 2 - for NULL */
            (*deps_p)[num_deps++] = fhn;
            (*deps_p)[num_deps] = NULL;

            /* Call finddeps on it */
            finddeps(fhn, incdirs, deps_p);
            break;
        }

        free(hn);
    }

    fclose(f);
    return;
}


