#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>

#define MAX_ENTRIES 1000

// o structura pentru stocarea metodelor
struct Metadata {
    char name[256];
    time_t last_modified;
    mode_t mode;
};

// functie pentru actualizarea snapshorului
void update_snapshot(const char *output_dir, int num_directories, const char *directories[]) {
    // aici verificam daca directorul de iesire exista, si daca nu il creem noi
    if (access(output_dir, F_OK) != 0) {
        mkdir(output_dir, 0777);
    }

    // Deschidem fisierul snapshot.bin
    char snapshot_file_path[512];
    snprintf(snapshot_file_path, sizeof(snapshot_file_path), "%s/snapshot.bin", output_dir);
    FILE *snapshot_file = fopen(snapshot_file_path, "wb");
    if (snapshot_file == NULL) {
        perror("PROBLEMA LA FOPEN");
        exit(EXIT_FAILURE);
    }

    // Vector pentru a stoca metadatele fiecărui fișier
    struct Metadata metadata[MAX_ENTRIES];
    int num_entries = 0;

    // cu acest for iterăm prin fiecare director din lista de directoare
    for (int i = 0; i < num_directories; i++) {
        // aici facem o deschidere a fisierului
        DIR *dir = opendir(directories[i]);
        if (dir == NULL) {
            perror("PROBLEMA LA OPENDIR");
            continue;
        }

        //aici  iterăm prin fiecare fișier din director
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && num_entries < MAX_ENTRIES) {
            // aici daca sunt  intrari  "." și ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Construim calea absoluta pentru fiecare fisier
            char entry_path[512];
            snprintf(entry_path, sizeof(entry_path), "%s/%s", directories[i], entry->d_name);

            // obtinem inf despre fiecare fisier
            struct stat entry_stat;
            if (lstat(entry_path, &entry_stat) == -1) {
                perror("PROBLEMA LA STAT");
                continue;
            }

            // aici stocam metodele fiecarui fiiser
            strcpy(metadata[num_entries].name, entry->d_name);
            metadata[num_entries].last_modified = entry_stat.st_mtime;
            metadata[num_entries].mode = entry_stat.st_mode;

            // si scriem metodele in fisierul snapshot.bin
            fwrite(&metadata[num_entries], sizeof(struct Metadata), 1, snapshot_file);

            num_entries++;
        }

        // Închidem directorul
        closedir(dir);
    }

    // inchidem fisierul snapshot.bin
    fclose(snapshot_file);

    // afisam continutul snapshotului in terminal
    printf("SNAPSHOT PENTRU DIRECTOARE :\n");
    for (int i = 0; i < num_entries; i++) {
        printf("NUMELE: %s\n", metadata[i].name);
        printf("ULTIMA MODIFICARE: %s", ctime(&metadata[i].last_modified));
        printf("MODUL: %o\n\n", metadata[i].mode);
    }
}

// functie pentru mutarea unui fisier in director specificat
void move_malicious_file(const char *file_path, const char *malicious_dir) {
    // Construim o cale noua pentru fisierul care are un cuvant gresit
    char new_path[512];
    snprintf(new_path, sizeof(new_path), "%s/%s", malicious_dir, strrchr(file_path, '/') + 1);

    // mutam fisierul in directorul de fisiere malitioase
    if (rename(file_path, new_path) == -1) {
        perror("LA RENAME");
        return;
    }

    printf("MUTA FISIERELE CARE SUNT PERICULOASE %s LA %s\n", file_path, new_path);
}
#define NUM_CHILDREN 3

void process_directory(const char *dir_path, const char *output_dir, const char *malicious_dir) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("PROBLEMA LA OPENDIR");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construim o cale absoluta pentru elementul curent
        char element_path[512];
        snprintf(element_path, sizeof(element_path), "%s/%s", dir_path, entry->d_name);

        // verificam daca elen=mentul este un director sau un fisier
        struct stat st;
        if (lstat(element_path, &st) == -1) {
            perror("PROBLEMA LA LSTAT");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // daca il gasim ca si director procesam recursim tot directorul
            process_directory(element_path, output_dir, malicious_dir);
        } else {
            // Daca este fisier verificam daca este periculos si il mutam daca este cazul
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("PROBLEMA LA PIPE");
                continue;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("PROBLEMA LA FORK");
                close(pipefd[0]);
                close(pipefd[1]);
                continue;
            }

            if (pid == 0) { // Procesul fiu
                close(pipefd[0]); // inchidem capatul   de citire al pipeului 

                // redirectionam iesirea standard catre pipe
                dup2(pipefd[1], STDOUT_FILENO);

                // executam scriptul
                execl("./script.sh", "./script.sh", element_path, NULL);

                // daca execul nu merge cum trebuie afusam erriarea
                perror("PROBLEMA LA EXECL");
                exit(EXIT_FAILURE);
            } else { // Procesul parinte
                close(pipefd[1]); // inchidem capatul de scriere al pipeului

                // asteptam finalizarea procesului fiu
                int status;
                waitpid(pid, &status, 0);

                // verifiacm daca scriptul returneaza malitios
                if (WIFEXITED(status)) {
                    // citim rezultatul din pipe
                    int num_files;
                    read(pipefd[0], &num_files, sizeof(num_files));

                    if (WEXITSTATUS(status) == 1) {
                        move_malicious_file(element_path, malicious_dir);
                    }

                    // inchidem capatul de citire al pipeuluui
                    close(pipefd[0]);

                    printf("Procesul copil s a incheiat cu PID-ul %d si cu %d fisiere cu potential periculos.\n",
                           getpid(), num_files);
                } else {
                    perror("Procesul Copil a eșuat");
                }
            }
        }
    }

    closedir(dir);
}




int main(int argc, char *argv[]) {
    // verificam argumentele sa nu fie mai mule sau mai putine
    if (argc < 4 || strcmp(argv[argc - 2], "-o") != 0) {
        fprintf(stderr, "Foloseste: %s <director1> [<directorul2> ...] -o <output_director>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *output_dir = argv[argc - 1];
    const char *malicious_dir = NULL;

    // identificam directorul periculos
    for (int i = 1; i < argc - 2; i++) {
        if (strcmp(argv[i], "-o") != 0) {
            malicious_dir = argv[argc-3];
            break;
        }
    }

    // iteam prin toate directoarele primite ca argumente și aplicăm process_directory pentru fiecare
    for (int i = 1; i < argc - 3; i++) {
        if (strcmp(argv[i], "-o") != 0) {
            printf("Proceseaza Directoarele: %s\n", argv[i]);
            process_directory(argv[i], output_dir, malicious_dir);
        }
    }

    // actualizam snapshotulrile pntru toate directoarele
    update_snapshot(output_dir, argc - 4, argv + 1);

    return EXIT_SUCCESS;
}