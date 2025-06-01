#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_CMDS 20   // máx. comandos por línea
#define MAX_ARGS 20   // máx. argumentos por comando
#define BUF 512 // tamaño del búfer de entrada

int main(void)
{
    char line[BUF];
    char *cmds[MAX_CMDS];

    while (1) {
        printf("shell> ");
        if (!fgets(line, sizeof(line), stdin)) break; // EOF
        line[strcspn(line, "\n")] = '\0';          // saca el salto

        if (strcmp(line, "exit") == 0) break;       // salir

        // separar pipes
        int ncmd = 0;
        char *tok = strtok(line, "|");
        while (tok && ncmd < MAX_CMDS) {
            while (*tok == ' ') tok++;               // saltea espacios iniciales
            cmds[ncmd++] = tok;
            tok = strtok(NULL, "|");
        }
        if (ncmd == 0) continue;                     // linea vacia

        int prev_in = -1;                            // extremo de lectura del pipe anterior

        for (int i = 0; i < ncmd; ++i) {
            int pfd[2];
            if (i < ncmd - 1 && pipe(pfd) == -1) { perror("pipe"); exit(1); }

            pid_t pid = fork();
            if (pid == -1) { perror("fork"); exit(1); }

            if (pid == 0) {
                // hijo
                if (prev_in != -1) dup2(prev_in, STDIN_FILENO);
                if (i < ncmd - 1)   dup2(pfd[1], STDOUT_FILENO);

                if (prev_in != -1) close(prev_in);
                if (i < ncmd - 1) { close(pfd[0]); close(pfd[1]); }

                // Crear argv para execvp
                char *argv[MAX_ARGS];
                int argc = 0;
                char *arg = strtok(cmds[i], " \t");
                while (arg && argc < MAX_ARGS - 1) {
                    argv[argc++] = arg;
                    arg = strtok(NULL, " \t");
                }
                argv[argc] = NULL;
                if (!argv[0]) _exit(0);              // comando vacio

                execvp(argv[0], argv);
                perror("exec");
                _exit(1);
            }
            // Padre 
            if (prev_in != -1) close(prev_in);
            if (i < ncmd - 1) {
                close(pfd[1]);                       // padre solo va a leer
                prev_in = pfd[0];
            }
        }

        if (prev_in != -1) close(prev_in);
        while (wait(NULL) > 0);                       // espera hijos
    }
    return 0;
}
