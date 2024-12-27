/*
 * Hasan Buğra Uslu G211210009  2C
 * Tarık Kalyoncu  G211210030 2C
 * 
 * shell.c
 * Linux Uygulaması Kaynak Kod
 */
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_COMMAND_LEN 1024
#define MAX_ARGS 100
#define MAX_BG_PROCESSES 100
#define MAX_LINE 1024

typedef struct {
    pid_t pid;
    int active;
} BackgroundProcess;

// Global değişkenler
BackgroundProcess bg_processes[MAX_BG_PROCESSES] = {{0}};
int bg_process_count = 0;

// Fonksiyon Prototipleri
void execute_command(char **args); // Komutları çalıştırır
void handle_background_processes(); // Arka plan işlemlerini yönetir
void parse_command(char *input, char **args, int *background); // Komutları ayrıştırır
void handle_single_command(char **args, int background); // Tek bir komutu işler
void handle_redirection(char **args); // Yönlendirme işleme fonksiyonu
void handle_pipe_command(char *input); // Borulu komutları işler
int handle_quit(const char *command); // "quit" komutunu işler ve kabuktan çıkış yapar
void handle_semicolon_commands(char *input); // Noktalı virgülle ayrılmış komutları işler

// Kabuğu çalıştır
void run_shell() {
    char input[MAX_LINE];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) { // Kullanıcı girişini al
            break;
        }

        input[strcspn(input, "\n")] = '\0'; // Yeni satırı kaldır

        if (strlen(input) == 0) { // Boş komut kontrolü
            continue;
        }

        if (handle_quit(input)) { // "quit" komutunu kontrol et
            break;
        }

        if (strchr(input, ';')) { // Noktalı virgülle ayrılmış komut kontrolü
            handle_semicolon_commands(input);
        } else if (strchr(input, '|')) { // Pipe içeren komut kontrolü
            handle_pipe_command(input);
        } else {
            char *args[MAX_ARGS] = {0};
            int background = 0; // Arka plan işlemi kontrolü için değişken
            parse_command(input, args, &background);

            if (args[0] == NULL) { // Geçersiz komut kontrolü
                fprintf(stderr, "Hatalı veya boş komut\n");
                continue;
            }

            handle_single_command(args, background); // Tekli komut işleme
        }

        handle_background_processes(); // Arka plan işlemlerini kontrol et
    }
}

// Noktalı virgülle ayrılmış komutları işleme
void handle_semicolon_commands(char *input) {
    char *commands[MAX_ARGS] = {0};
    int num_commands = 0;

    char *token = strtok(input, ";"); // Noktalı virgülle ayır
    while (token != NULL && num_commands < MAX_ARGS - 1) {
        commands[num_commands++] = token;
        token = strtok(NULL, ";");
    }
    commands[num_commands] = NULL;

    for (int i = 0; i < num_commands; i++) {
        char *args[MAX_ARGS] = {0};
        int background = 0;
        parse_command(commands[i], args, &background); // Komutu ayrıştır

        if (args[0] == NULL) {
            fprintf(stderr, "Hatalı veya boş komut: %s\n", commands[i]);
            continue;
        }

        handle_single_command(args, background); // Komutu çalıştır
    }
}

// Komutu çalıştır
void execute_command(char **args) {
    if (args == NULL || args[0] == NULL) {
        fprintf(stderr, "Geçersiz komut\n");
        exit(EXIT_FAILURE);
    }

    if (execvp(args[0], args) == -1) { // Komutu çalıştır
        perror("Komut çalıştırma hatası");
        exit(EXIT_FAILURE);
    }
}

// Tekli komutları çalıştır
void handle_single_command(char **args, int background) {
    if (args == NULL || args[0] == NULL) { // NULL kontrolü
        fprintf(stderr, "Hatalı veya boş komut\n");
        return;
    }

    pid_t pid = fork(); // Yeni işlem oluştur

    if (pid < 0) { // Fork başarısızsa
        perror("Fork hatası");
    } else if (pid == 0) { // Çocuk işlem
        if (background) { // Arka plan işlemiyse
            setsid();
        }
        handle_redirection(args); // Yönlendirme işle
        execute_command(args); // Komutu çalıştır
    } else { // Ebeveyn işlem
        if (background) { // Arka plan işlemi kaydet
            if (bg_process_count >= MAX_BG_PROCESSES) {
                fprintf(stderr, "Arka plan işlem limiti aşıldı\n");
                return;
            }
            for (int i = 0; i < MAX_BG_PROCESSES; i++) {
                if (!bg_processes[i].active) {
                    bg_processes[i].pid = pid;
                    bg_processes[i].active = 1;
                    bg_process_count++;
                    break;
                }
            }
            printf("[%d] arka plan işlemi başlatıldı.\n", pid);
        } else {
            int status;
            waitpid(pid, &status, 0); // İşlemi bekle
        }
    }
}

// Borulu komutları çalıştır
void handle_pipe_command(char *input) {
    char *commands[MAX_ARGS] = {0};
    int num_commands = 0;

    char *token = strtok(input, "|"); // Pipe ile ayır
    while (token != NULL && num_commands < MAX_ARGS - 1) {
        commands[num_commands++] = token;
        token = strtok(NULL, "|");
    }
    commands[num_commands] = NULL;

    int pipefds[2 * (num_commands - 1)];
    for (int i = 0; i < num_commands - 1; i++) { // Pipe oluştur
        if (pipe(pipefds + i * 2) < 0) {
            perror("Pipe oluşturma hatası");
            return;
        }
    }

    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork(); // Her komut için işlem oluştur

        if (pid < 0) {
            perror("Fork hatası");
            return;
        } else if (pid == 0) { // Çocuk işlem
            if (i > 0) { // Önceki komuttan giriş oku
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < num_commands - 1) { // Çıktıyı sonraki pipe'a yaz
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }

            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipefds[j]); // Tüm pipe'ları kapat
            }

            char *args[MAX_ARGS] = {0};
            int background = 0; // Pipe işlemleri için arka plan durumu yok
            parse_command(commands[i], args, &background); // Komutları ayrıştır
            handle_redirection(args); // Yönlendirme işle
            execute_command(args); // Komutu çalıştır
        }
    }

    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipefds[i]); // Pipe'ları kapat
    }

    for (int i = 0; i < num_commands; i++) {
        wait(NULL); // İşlemleri bekle
    }
}

// Yönlendirme kontrolü ve işleme
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) { // Çıkış yönlendirme
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Yönlendirme için eksik parametre\n");
                exit(EXIT_FAILURE);
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Çıkış dosyası açılamadı");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); // Çıkışı dosyaya yönlendir
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) { // Giriş yönlendirme
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Yönlendirme için eksik parametre\n");
                exit(EXIT_FAILURE);
            }
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("Giriş dosyası açılamadı");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO); // Girişi dosyadan oku
            close(fd);
            args[i] = NULL;
        }
    }
}

// Komutları ayrıştır
void parse_command(char *input, char **args, int *background) {
    if (input == NULL || args == NULL || background == NULL) {
        fprintf(stderr, "Geçersiz giriş\n");
        return;
    }

    *background = 0; // Arka plan işlemi varsayılan olarak kapalı

    char *token = strtok(input, " ");
    int index = 0;

    while (token != NULL && index < MAX_ARGS - 1) {
        if (strcmp(token, "&") == 0) { // "&" ile arka plan işlemini kontrol et
            *background = 1;
        } else {
            args[index++] = token; // Argümanları sıraya ekle
        }
        token = strtok(NULL, " ");
    }

    args[index] = NULL; // Argümanların sonunu belirt
}

// Arka plan işlemlerini yönet
void handle_background_processes() {
    for (int i = 0; i < MAX_BG_PROCESSES; i++) {
        if (bg_processes[i].active) {
            int status;
            pid_t pid = waitpid(bg_processes[i].pid, &status, WNOHANG); // Arka plan işlemlerini kontrol et
            if (pid > 0) {
                printf("[%d] retval: %d\n", pid, WEXITSTATUS(status));
                bg_processes[i].active = 0;
                bg_process_count--;
            } else if (pid < 0) {
                perror("waitpid hatası");
            }
        }
    }
}

// "quit" komutunu işleyerek çıkış
int handle_quit(const char *command) {
    if (strcmp(command, "quit") == 0) { // "quit" kontrolü
        int active_processes = 0;

        for (int i = 0; i < MAX_BG_PROCESSES; i++) {
            if (bg_processes[i].active) {
                active_processes = 1;
                break;
            }
        }

        if (active_processes) { // Arka plan işlemleri varsa uyarı
            printf("Arkaplan işlemleri bekleniyor...\n");
        }

        for (int i = 0; i < MAX_BG_PROCESSES; i++) {
            if (bg_processes[i].active) { // Tüm arka plan işlemlerini bekle
                int status;
                waitpid(bg_processes[i].pid, &status, 0);
                printf("[%d] retval: %d\n", bg_processes[i].pid, WEXITSTATUS(status));
                bg_processes[i].active = 0;
            }
        }
        return 1; // Çıkış sinyali
    }
    return 0; // Devam et
}
