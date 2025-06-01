/*------------------------------------------------------------------
 *  ring.c  –  TP 4 (Comunicación en anillo)
 *
 *  Uso:  ./ring <n> <c> <s>
 *        n : número de procesos hijos    (n ≥ 3)
 *        c : entero inicial
 *        s : hijo que inicia el recorrido  (1 ≤ s ≤ n)
 *
 *  Cada hijo incrementa el valor una vez y lo reenvía.
 *  Cuando el último hijo escribe en result_pipe, el padre
 *  lee y muestra el valor final.
 *-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define DIE(msg)  do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[])
{
    /* --- 0. Validación de argumentos ----------------------------------- */
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int n      = atoi(argv[1]);
    int value  = atoi(argv[2]);
    int start  = atoi(argv[3]);          /* índices 1-based */

    if (n < 3 || start < 1 || start > n) {
        fprintf(stderr, "Error:  n ≥ 3  y  1 ≤ s ≤ n\n");
        return EXIT_FAILURE;
    }

    /* --- 1. Reservar estructura de pipes del anillo --------------------- */
    int (*ring)[2] = malloc(n * sizeof *ring);
    if (!ring) DIE("malloc");

    for (int i = 0; i < n; ++i)
        if (pipe(ring[i]) == -1) DIE("pipe ring");

    /* Tubos exclusivos del padre */
    int start_pipe[2], result_pipe[2];
    if (pipe(start_pipe) == -1 || pipe(result_pipe) == -1)
        DIE("pipe padre");

    int last = (start == 1) ? n : start - 1;   /* hijo que cierra el ciclo */

    /* --- 2. Crear los hijos -------------------------------------------- */
    for (int idx = 1; idx <= n; ++idx) {
        pid_t pid = fork();
        if (pid < 0) DIE("fork");

        if (pid == 0) {            /* ======= HIJO idx ======= */

            /* Elegir fds de entrada y salida para este hijo */
            int in_fd  = (idx == start) ? start_pipe[0]
                                        : ring[(idx - 2 + n) % n][0];

            int out_fd = (idx == last)  ? result_pipe[1]
                                        : ring[(idx - 1) % n][1];

            /* Cerrar todo lo que no corresponde */
            for (int j = 0; j < n; ++j) {
                if (ring[j][0] != in_fd)   close(ring[j][0]);
                if (ring[j][1] != out_fd)  close(ring[j][1]);
            }
            /* Extremos del padre */
            close(start_pipe[1]);  /* solo escribe el padre      */
            close(result_pipe[0]); /* solo lee    el padre       */
            if (idx != start) close(start_pipe[0]);
            if (idx != last ) close(result_pipe[1]);

            /* Trabajo: leer, ++ y reenviar */
            if (read(in_fd, &value, sizeof value) != sizeof value)
                DIE("read hijo");
            ++value;
            if (write(out_fd, &value, sizeof value) != sizeof value)
                DIE("write hijo");

            close(in_fd);
            close(out_fd);
            _exit(EXIT_SUCCESS);
        }
        /* El padre sigue creando al resto */
    }

    /* --- 3. PADRE ------------------------------------------------------- */
    /* Cerrar anillo completo: el padre no lo usa */
    for (int i = 0; i < n; ++i) {
        close(ring[i][0]);
        close(ring[i][1]);
    }
    free(ring);

    /* El padre solo escribe por start_pipe[1] y lee por result_pipe[0] */
    close(start_pipe[0]);
    close(result_pipe[1]);

    /* Inyectar valor inicial */
    if (write(start_pipe[1], &value, sizeof value) != sizeof value)
        DIE("write padre");
    close(start_pipe[1]);

    /* Esperar resultado */
    if (read(result_pipe[0], &value, sizeof value) != sizeof value)
        DIE("read resultado");
    close(result_pipe[0]);

    printf("Valor final recibido en el padre: %d\n", value);

    /* Esperar a todos los hijos */
    while (wait(NULL) > 0);
    return EXIT_SUCCESS;
}
