#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_PATH_LEN 512
#define MAX_LINE_LEN 512

bool is_corrupted(const char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        perror("eroare la deschiderea fisierului");
        return false;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, "corrupted")!= NULL ||
            strstr(line, "dangerous")!= NULL ||
            strstr(line, "risk") !=NULL ||
            strstr(line, "attack") != NULL ||
            strstr(line, "malware") != NULL ||
            strstr(line, "malicious") != NULL) {
            fclose(file);
            return true; 
        }

        for (int i = 0; line[i] != '\0'; i++) {             //aici verificam caracterele non-ascii
            if (!isascii(line[i])) {
                fclose(file);
                return true; // inseamna ca se gaseste cel putin un caracter
            }
        }
    }

    fclose(file);
    return false; // Nu am gasit nimic
}

void analizam_fisierele(const char *file_name, int pipe_fd) {
    if (is_corrupted(file_name)) {
        // Trimitem informații despre fișierul suspect prin pipe
        char message[MAX_PATH_LEN + 50]; // spațiu pentru numele fișierului și mesajul de notificare
        snprintf(message, sizeof(message), "Fișierul %s este potențial corupt sau maleficent.\n", file_name);
        write(pipe_fd, message, strlen(message) + 1);
    }
}

void check_permissions(const char *dir_name, const char *output_dir, const char *isolated_space_dir, int pipe_fd) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(dir_name);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // doar pentru fișiere obișnuite
           char file_path[MAX_PATH_LEN];
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, entry->d_name);
            file_path[MAX_PATH_LEN - 1] = '\0'; 
            struct stat file_stat;
            if (stat(file_path, &file_stat) == -1) {
                perror("stat");
                continue;
            }

            if ((file_stat.st_mode & S_IRWXU) == 0) { // verifică dacă toate drepturile sunt lipsă
                analizam_fisierele(file_path , pipe_fd);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 5 || strcmp(argv[1], "-o") != 0 || strcmp(argv[3], "-s") != 0) {
        printf("Utilizare: %s -o DIRECTOR_IESIRE -s DIRECTOR_IZOLARE --SI_RESTUL_DIRECTOARELOR\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *output_dir = argv[2];
    const char *isolated_space_dir = argv[4];

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork pentru a crea un proces copil
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Proces copil
        close(pipe_fd[0]); // Închidem capătul de citire al pipe-ului în procesul copil
        for (int i = 5; i < argc; i++) {
            check_permissions(argv[i], output_dir, isolated_space_dir, pipe_fd[1]);
        }
        close(pipe_fd[1]); // Închidem capătul de scriere al pipe-ului în procesul copil
        exit(EXIT_SUCCESS);
    } else { // Proces părinte
        close(pipe_fd[1]); // Închidem capătul de scriere al pipe-ului în procesul părinte

        char message[MAX_LINE_LEN];
        while (read(pipe_fd[0], message, sizeof(message)) != 0) {
            printf("%s", message); // Afișăm mesajele primite de la procesele copil
        }
        close(pipe_fd[0]); // Închidem capătul de citire al pipe-ului în procesul părinte
        wait(NULL); // Așteptăm terminarea procesului copil
    }

    return 0;
}
