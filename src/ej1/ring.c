#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void die(const char *msg) { perror(msg); exit(EXIT_FAILURE); }

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int n = atoi(argv[1]), val = atoi(argv[2]), s = atoi(argv[3]);
    if (n < 3 || s < 1 || s > n) {
        fprintf(stderr, "Argumentos inválidos\n");
        return EXIT_FAILURE;
    }

    /* 1) Pipes del anillo entre hijos */
    int ring[n][2];
    for (int i = 0; i < n; ++i) if (pipe(ring[i])) die("pipe");

    /* 2) Pipe hijo_last → padre  y  padre → hijo_s                    */
    int p_last2par[2], p_par2start[2];
    if (pipe(p_last2par) || pipe(p_par2start)) die("pipe-parent");

    int last = (s + n - 2) % n;            /* hijo que cierra el lazo   */

    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) die("fork");

        if (pid == 0) {                    /* ====== HIJO i ====== */
            /* Elegir fds de entrada y salida */
            int in_fd  = (i == (s-1)) ? p_par2start[0]
                       : ring[(i-1+n)%n][0];

            int out_fd = (i == last) ? p_last2par[1]
                       : ring[i][1];

            /* Cerrar todo lo que no se usa */
            for (int j = 0; j < n; ++j) {
                close(ring[j][0]); close(ring[j][1]);
            }
            close(p_par2start[1]);               /* ya lo usa el padre  */
            close(p_last2par[0]);                /* idem               */
            if (i != (s-1))  close(p_par2start[0]);
            if (i != last )  close(p_last2par[1]);

            /* Trabajo: leer, ++, escribir y terminar */
            if (read(in_fd, &val, sizeof(val)) != sizeof(val)) exit(EXIT_FAILURE);
            ++val;
            if (write(out_fd, &val, sizeof(val)) != sizeof(val)) exit(EXIT_FAILURE);

            close(in_fd); close(out_fd);
            _exit(EXIT_SUCCESS);
        }
        /* ====== PADRE continúa el bucle para crear más hijos ====== */
    }

    /* ====== PADRE ====== */
    /* Cerrar extremos inútiles */
    for (int i = 0; i < n; ++i) { close(ring[i][0]); close(ring[i][1]); }
    close(p_par2start[0]);  close(p_last2par[1]);

    /* Enviar valor inicial y esperar resultado */
    write(p_par2start[1], &val, sizeof(val));
    close(p_par2start[1]);

    if (read(p_last2par[0], &val, sizeof(val)) != sizeof(val))
        die("read resultado");
    printf("Resultado final: %d\n", val);
    close(p_last2par[0]);

    /* Esperar a todos los hijos */
    while (wait(NULL) > 0);
    return EXIT_SUCCESS;
}
