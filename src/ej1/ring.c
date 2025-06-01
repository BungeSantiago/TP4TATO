#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]);       // Cantidad de procesos
    int valor = atoi(argv[2]);   // Valor inicial
    int start = atoi(argv[3]);   // Proceso que inicia

    if (start < 0 || start >= n) {
        fprintf(stderr, "Error: el proceso inicial debe estar entre 0 y %d\n", n-1);
        exit(1);
    }


    int pipes[n][2];
    int parent_pipe[2];

    pipe(parent_pipe);  

    for (int i = 0; i < n; i++) {
        pipe(pipes[i]);
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            for (int j = 0; j < n; j++) {
                if (j != i) close(pipes[j][0]);
                if (j != (i + 1) % n) close(pipes[j][1]);
            }
            close(parent_pipe[0]); 

            int val;
            read(pipes[i][0], &val, sizeof(int));
            if (i != start) {
                val++;
            }
            printf("Hijo %d recibió %d\n", i, val);


            if ((i + 1) % n == start) {
                val++;
                write(parent_pipe[1], &val, sizeof(int));  // Último hijo → padre
            } else {
                write(pipes[(i + 1) % n][1], &val, sizeof(int));  // Siguiente hijo
            }

            exit(0);
        }
    }

    close(parent_pipe[1]);  
    write(pipes[start][1], &valor, sizeof(int));  // Inicia el mensaje

    int final;
    read(parent_pipe[0], &final, sizeof(int));  // Espera resultado
    printf("Padre recibe resultado final: %d\n", final);

        for (int i = 0; i < n; i++) {
            wait(NULL);
        }

        return 0;
    }