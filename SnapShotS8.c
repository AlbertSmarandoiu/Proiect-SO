#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<time.h>
#include<sys/sysmacros.h>

#define MAX_ENTRIES 1000
#define MAX_ARGS 10

struct MetodaSnapShot
{
    char name[256];
    time_t ultima_modificare;
    mode_t mode;
};

void update_snapshot(const char *director, struct MetodaSnapShot *snapshot , int *count_snap){
    DIR *dir = opendir(director);
    if(dir == NULL){
        perror("nu merge opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name , ".") == 0 || strcmp(entry->d_name , "..") == 0){
            continue;
        }

        char entry_path[512];
        snprintf(entry_path , sizeof(entry_path) , "%s/%s" , director , entry->d_name);

        struct stat entry_stat;
        if(lstat(entry_path , &entry_stat) == -1){
            perror("e problema la lstat");
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        strcpy(snapshot[*count_snap].name , entry->d_name);
        snapshot[*count_snap].ultima_modificare = entry_stat.st_mtime;
        snapshot[*count_snap].mode = entry_stat.st_mode;
        (*count_snap)++;

        if(S_ISDIR(entry_stat.st_mode))
        {
            update_snapshot(entry_path , snapshot , count_snap);
        }
    }
    closedir(dir);
}

void write_snapshot(const char *output_dir, const char *snapshot_name, struct MetodaSnapShot *snapshot, int count_snap) {
    char output_path[512];
    snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, snapshot_name);
    FILE *output_file = fopen(output_path, "w");
    if (!output_file) {
        perror("Eroare la deschiderea fisierului de iesire");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < count_snap; i++) {
        fprintf(output_file, "%s - Last modified: %s", snapshot[i].name, ctime(&snapshot[i].ultima_modificare));
    }

    fclose(output_file);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > MAX_ARGS) {
        fprintf(stderr, "Usage: %s <directory1> <directory2> ... <output_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *output_dir = argv[argc - 1];
    struct MetodaSnapShot snapshots[MAX_ENTRIES];
    int snapshot_counts[MAX_ARGS - 1] = {0};

    for (int i = 1; i < argc - 1; i++) {
        update_snapshot(argv[i], snapshots, &snapshot_counts[i - 1]);
        char snapshot_name[256];
        snprintf(snapshot_name, sizeof(snapshot_name), "snapshot_%d.txt", i);
        write_snapshot(output_dir, snapshot_name, snapshots, snapshot_counts[i - 1]);
    }

    return EXIT_SUCCESS;
}
