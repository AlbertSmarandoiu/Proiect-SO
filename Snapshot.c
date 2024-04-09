#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<time.h>
#include<sys/sysmacros.h>

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dir_path = argv[1];

    struct MetodaSnapShot initial_snapshot[1000];
    int initial_snapshot_count = 0;
    update_snapshot(dir_path, initial_snapshot, &initial_snapshot_count);

    printf("Initial snapshot:\n");
    for (int i = 0; i < initial_snapshot_count; i++) {
        printf("%s - Last modified: %s", initial_snapshot[i].name, ctime(&initial_snapshot[i].ultima_modificare));
    }

    return EXIT_SUCCESS;
}
