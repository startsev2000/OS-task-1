#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {
    int fd1[2], fd2[2];
    pid_t pid1, pid2, pid3;

    if (pipe(fd1) == -1 || pipe(fd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();

    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // Первый процесс считывает данные из файла и добавляет их в первый пайп
        close(fd1[0]);
        close(fd2[0]);
        close(fd2[1]);

        FILE *fp = fopen(argv[1], "r");
        if (fp == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        ssize_t read_size;

        while ((read_size = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
            write(fd1[1], buffer, read_size);
        }

        close(fd1[1]);
        fclose(fp);

        exit(EXIT_SUCCESS);
    } else {
        pid2 = fork();

        if (pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid2 == 0) {
            // Второй процесс получает данные из первого пайпа от первого процесса и осуществляет необходимые процедуры
            // в соответствии с заданием
            close(fd1[1]);
            close(fd2[0]);

            char buffer[BUFFER_SIZE];
            int uppercase_count = 0, lowercase_count = 0;
            ssize_t read_size;

            while ((read_size = read(fd1[0], buffer, BUFFER_SIZE)) > 0) {
                for (int i = 0; i < read_size; i++) {
                    char c = buffer[i];
                    if (c >= 'A' && c <= 'Z') {
                        uppercase_count++;
                    } else if (c >= 'a' && c <= 'z') {
                        lowercase_count++;
                    }
                }
            }

            // Записываем данные в пайп
            char result_buffer[BUFFER_SIZE];
            sprintf(result_buffer, "Uppercase count: %d\nLowercase count: %d\n", uppercase_count, lowercase_count);
            write(fd2[1], result_buffer, sizeof(result_buffer));

            close(fd1[0]);
            close(fd2[1]);

            exit(EXIT_SUCCESS);
        } else {
            pid3 = fork();

            if (pid3 == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid3 == 0) {
                // Третий процесс получает данные из второго пайпа и записывает их в указанный в командной строке файл
                close(fd1[0]);
                close(fd1[1]);
                close(fd2[1]);


                FILE *out_fp = fopen(argv[2], "w");
                if (out_fp == NULL) {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }

                char result_buffer[BUFFER_SIZE];
                ssize_t result_size = read(fd2[0], result_buffer, BUFFER_SIZE);
                if (result_size > 0) {
                    fwrite(result_buffer, sizeof(char), strlen(result_buffer), out_fp);
                }

                fclose(out_fp);
                close(fd2[0]);

                exit(EXIT_SUCCESS);
            } else {
                // В родительском процессе ожидаем, когда все процессы завершатся, после чего завершаем работу программы
                close(fd1[0]);
                close(fd1[1]);
                close(fd2[0]);
                close(fd2[1]);

                wait(NULL);
                wait(NULL);
                wait(NULL);

                exit(EXIT_SUCCESS);
            }
        }
    }
}
