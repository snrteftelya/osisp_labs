#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <locale.h>

static int show_symlinks = 0;
static int show_dirs = 0;
static int show_files = 0;
static int sort_output = 0;

void process_dir(const char *path);

int compare_strings(const void *a, const void *b) {
    char buf1[PATH_MAX], buf2[PATH_MAX];

    strxfrm(buf1, *(const char * const *)a, sizeof(buf1));
    strxfrm(buf2, *(const char * const *)b, sizeof(buf2));

    return strcmp(buf1, buf2);
}

void process_entry(const char *path, const char *name) {
    char full_path[PATH_MAX];
    size_t len = snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
    if (len >= sizeof(full_path)) {
        fprintf(stderr, "Ошибка: переполнение буфера при формировании пути\n");
        return;
    }

    struct stat st;
    if (lstat(full_path, &st) == -1) {
        fprintf(stderr, "Ошибка: не удалось получить информацию о %s: %s\n", full_path, strerror(errno));
        return;
    }

    if (S_ISLNK(st.st_mode) && show_symlinks) {
        printf("%s\n", full_path);
    } else if (S_ISDIR(st.st_mode)) {
        if (show_dirs)
            printf("%s\n", full_path);
        if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
            process_dir(full_path);
        }
    } else if (S_ISREG(st.st_mode) && show_files) {
        printf("%s\n", full_path);
    }
}

void process_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Ошибка: не удалось открыть каталог %s: %s\n", path, strerror(errno));
        return;
    }

    struct dirent *entry;
    char **entries = NULL;
    size_t count = 0;

    while ((entry = readdir(dir)) != NULL) {
        entries = realloc(entries, (count + 1) * sizeof(char *));
        if (entries == NULL) {
            fprintf(stderr, "Ошибка: не удалось выделить память\n");
            exit(EXIT_FAILURE);
        }

        entries[count] = strdup(entry->d_name);
        if (entries[count] == NULL) {
            fprintf(stderr, "Ошибка: не удалось выделить память\n");
            exit(EXIT_FAILURE);
        }
        count++;
    }
    closedir(dir);

    if (sort_output) {
        qsort(entries, count, sizeof(char *), compare_strings);
    }

    for (size_t i = 0; i < count; i++) {
        process_entry(path, entries[i]);
        free(entries[i]);
    }
    free(entries);
}

int main(int argc, char *argv[]) {
    setlocale(LC_COLLATE, "");

    int opt;
    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
            case 'l': show_symlinks = 1; break;
            case 'd': show_dirs = 1; break;
            case 'f': show_files = 1; break;
            case 's': sort_output = 1; break;
            default:
                fprintf(stderr, "Использование: %s [директория] [-l -d -f -s]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!(show_symlinks || show_dirs || show_files)) {
        show_symlinks = show_dirs = show_files = 1;
    }

    const char *start_dir = (optind < argc) ? argv[optind] : "./";
    process_dir(start_dir);
    return EXIT_SUCCESS;
}
