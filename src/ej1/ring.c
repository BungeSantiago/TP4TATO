#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * Ring communication among n child processes.
 * Usage:  ./ring <n> <c> <s>
 *   <n> = número de procesos hijos en el anillo (must be ≥1)
 *   <c> = valor entero inicial que envía el padre
 *   <s> = índice (1-based) del proceso que inicia la comunicación (1 ≤ s ≤ n)
 *
 * Cada hijo i (1-based) lee un entero de su predecessor
 * en el anillo, lo incrementa en 1, y se lo envía al sucesor.
 * Cuando el mensaje ha dado una vuelta completa, el padre
 * lo recibe y lo imprime. El resultado esperado es c + n.
 */

int main(int argc, char **argv)
{
    int n, start, initial;
    int status;
    pid_t pid;

    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        fprintf(stderr, "  <n> = cantidad de procesos hijos en el anillo (≥1)\n");
        fprintf(stderr, "  <c> = valor entero inicial del mensaje\n");
        fprintf(stderr, "  <s> = índice (1..n) del proceso que inicia la comunicación\n");
        exit(EXIT_FAILURE);
    }

    /* Parseo de argumentos */
    n       = atoi(argv[1]);
    initial = atoi(argv[2]);
    start   = atoi(argv[3]);

    if (n < 1 || start < 1 || start > n) {
        fprintf(stderr, "Argumentos inválidos. Asegúrese de que n ≥ 1 y 1 ≤ s ≤ n.\n");
        exit(EXIT_FAILURE);
    }

    printf("Se crearán %d procesos, se enviará el entero %d desde proceso %d\n",
           n, initial, start);

    /* 1) Crear n pipes: pipes[i] conecta al hijo i+1 con su sucesor (i+2) */
    int pipes[n][2];
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    /* 2) Fork de los n hijos, con índice i=1..n */
    for (int i = 1; i <= n; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            /* --- CÓDIGO DEL HIJO i (1-based) --- */
            int idx  = i;                     /* espejo 1..n */
            int prev = (idx == 1 ? n : idx - 1);
            int next = (idx == n ? 1 : idx + 1);

            /* File descriptors que este hijo usará */
            int read_fd  = pipes[prev - 1][0]; /* leer de (prev → idx) */
            int write_fd = pipes[idx - 1][1];  /* escribir a (idx → next) */

            /* 2.a) Cerrar todos los extremos de pipe que NO se usarán */
            for (int j = 0; j < n; j++) {
                if (j != (prev - 1)) {
                    close(pipes[j][0]);
                }
                if (j != (idx - 1)) {
                    close(pipes[j][1]);
                }
            }

            /* 2.b) Leer un entero del predecessor, incrementarlo y reenviar */
            int valor;
            if (read(read_fd, &valor, sizeof(int)) != sizeof(int)) {
                perror("read en hijo");
                exit(EXIT_FAILURE);
            }
            valor++;  /* Aquí incrementamos de verdad */

            if (write(write_fd, &valor, sizeof(int)) != sizeof(int)) {
                perror("write en hijo");
                exit(EXIT_FAILURE);
            }

            /* 2.c) Cerrar los fds que se usaron */
            close(read_fd);
            close(write_fd);

            /* El hijo termina */
            _exit(EXIT_SUCCESS);
        }
        /* El padre continúa creando el siguiente hijo */
    }

    /* --- CÓDIGO DEL PADRE (solo después de crear los n hijos) --- */

    /* 3) Determinar qué pipe usar para inyectar el mensaje inicial (c)
     *    y para recibir el resultado final. 
     *    El proceso “start” va a leer de pipes[prev_start - 1][0].
     *    Por lo tanto, el padre debe escribir en pipes[prev_start - 1][1],
     *    y, al final, leerá de pipes[prev_start - 1][0].
     */
    int prev_start = (start == 1 ? n : start - 1);
    int write_fd_parent = pipes[prev_start - 1][1];
    int read_fd_parent  = pipes[prev_start - 1][0];

    /* 4) Cerrar en el padre TODOS los extremos de pipe que NO use */
    for (int j = 0; j < n; j++) {
        if (j != (prev_start - 1)) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
    }

    /* 5) Inyectar el mensaje inicial en el anillo */
    if (write(write_fd_parent, &initial, sizeof(int)) != sizeof(int)) {
        perror("write inicial en padre");
        exit(EXIT_FAILURE);
    }

    /* 6) Esperar a que el mensaje recorra todos los hijos y vuelva */
    int resultado;
    if (read(read_fd_parent, &resultado, sizeof(int)) != sizeof(int)) {
        perror("read final en padre");
        exit(EXIT_FAILURE);
    }

    printf("Mensaje final recibido en el padre: %d\n", resultado);

    /* 7) Cerrar los fds usados y esperar a los hijos */
    close(write_fd_parent);
    close(read_fd_parent);

    for (int i = 0; i < n; i++) {
        wait(&status);
    }

    return 0;
}
