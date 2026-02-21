#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

typedef struct {
    char **items;
    size_t size;
    size_t capacity;
} PathList;

typedef struct {
    int l; int d; int f; int s;
} Options;

void add_path(PathList *list, const char *path) {
    if (list->size >= list->capacity) {
        list->capacity = (list->capacity == 0) ? 16 : list->capacity * 2;
        char **tmp = realloc(list->items, list->capacity * sizeof(char *));
        if (!tmp) { perror("realloc"); exit(EXIT_FAILURE); }
        list->items = tmp;
    }
    list->items[list->size++] = strdup(path);
}

int compare_paths(const void *a, const void *b) {
    return strcoll(*(const char **)a, *(const char **)b);
}

void dir_walk(const char *dir_path, const Options *opts, PathList *list) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror(dir_path);
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) == -1) {
            perror(full_path);
            continue;
        }

        int match = 0;
        if (S_ISLNK(st.st_mode) && opts->l) match = 1;
        else if (S_ISDIR(st.st_mode) && opts->d) match = 1;
        else if (S_ISREG(st.st_mode) && opts->f) match = 1;

        if (match) {
            if (opts->s) add_path(list, full_path);
            else printf("%s\n", full_path);
        }

        if (S_ISDIR(st.st_mode)) {
            dir_walk(full_path, opts, list);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    Options opts = {0, 0, 0, 0};
    int opt;
    
    optind = 1;
    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
            case 'l': opts.l = 1; break;
            case 'd': opts.d = 1; break;
            case 'f': opts.f = 1; break;
            case 's': opts.s = 1; break;
            default: exit(EXIT_FAILURE);
        }
    }

    if (!opts.l && !opts.d && !opts.f) {
        opts.l = opts.d = opts.f = 1;
    }

    const char *start_dir = "./";
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            start_dir = argv[i];
            break;
        }
    }

    PathList list = {NULL, 0, 0};
    dir_walk(start_dir, &opts, &list);

    if (opts.s && list.items) {
        qsort(list.items, list.size, sizeof(char *), compare_paths);
        for (size_t i = 0; i < list.size; i++) {
            printf("%s\n", list.items[i]);
            free(list.items[i]);
        }
        free(list.items);
    }

    return 0;
}
