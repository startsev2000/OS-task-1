#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {
    int fd1, fd2;
    pid_t pid;

    if (mkfifo("pipe1", 0666) == -1 || mkfifo("pipe2", 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Дочерний процесс получает данные из первого пайпа, обрабатывает их и записывает во второй пайп
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        fd1 = open("pipe1", O_RDONLY);
        fd2 = open("pipe2", O_WRONLY);
        if (fd1 == -1 || fd2 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        int uppercase_count = 0, lowercase_count = 0;
        ssize_t read_size;

        while ((read_size = read(fd1, buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < read_size; i++) {
                char c = buffer[i];
                if (c >= 'A' && c <= 'Z') {
                    uppercase_count++;
                } else if (c >= 'a' && c <= 'z') {
                    lowercase_count++;
                }
            }
        }

        char result_buffer[BUFFER_SIZE];
        sprintf(result_buffer, "Uppercase count: %d\nLowercase count: %d\n", uppercase_count, lowercase_count);
        write(fd2, result_buffer, sizeof(result_buffer));

        close(fd1);
        close(fd2);

        exit(EXIT_SUCCESS);
    } else {
        // Родительский процесс считывает данные из указанного в командной строке файла, отправляет их в первый пайп,
        // считывает данные из второго пайпа и выводит их в указанный в командной строке файл
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        fd1 = open("pipe1", O_WRONLY);
        if (fd1 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        FILE *fp = fopen(argv[1], "r");
        if (fp == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        ssize_t read_size;

        while ((read_size = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
            write(fd1, buffer, read_size);
        }

        fclose(fp);
        close(fd1);

        // Read results from second named pipe
        fd2 = open("pipe2", O_RDONLY);
        if (fd2 == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char result_buffer[BUFFER_SIZE];
        ssize_t result_size = read(fd2, result_buffer, BUFFER_SIZE);
        if (result_size > 0) {
            FILE *out_fp = fopen(argv[2], "w");
            if (out_fp == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            fwrite(result_buffer, sizeof(char), strlen(result_buffer), out_fp);
            fclose(out_fp);
        }

        close(fd2);
        unlink("pipe1");
        unlink("pipe2");

        exit(EXIT_SUCCESS);
    }
}
