#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200

int main() {

    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        printf("Shell> ");
        
        /*Reads a line of input from the user from the standard input (stdin) and stores it in the variable command */
        fgets(command, sizeof(command), stdin);
        
        /* Removes the newline character that fgets captures after the user presses Enter */
        command[strcspn(command, "\n")] = '\0';

        /* If the user enters an empty string (just pressed Enter), restart loop */
        if (command[0] == '\0') {
            continue;
        }

        /* If the user enters "exit", break the loop and exit shell */
        if (strcmp(command, "exit") == 0) {
            break;
        }

        /* Split the command line into individual commands separated by '|' */
        command_count = 0;
        char *token = strtok(command, "|");
        while (token != NULL) 
        {
            /* Remove leading spaces */
            while (*token == ' ') {
                token++;
            }
            /* Remove trailing spaces */
            char *end = token + strlen(token) - 1;
            while (end > token && *end == ' ') {
                *end = '\0';
                end--;
            }

            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        /* You should start programming from here... */
        int num_pipes = command_count - 1;
        int pipefd[num_pipes][2];
        for (int i = 0; i < num_pipes; i++) {
            if (pipe(pipefd[i]) < 0) {
                perror("pipe");
                /* Close any previously opened pipes */
                for (int j = 0; j < i; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                goto next_iteration;
            }
        }

        for (int i = 0; i < command_count; i++) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                /* On fork error, close all pipes and wait for already forked children */
                for (int j = 0; j < num_pipes; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                for (int k = 0; k < i; k++) {
                    wait(NULL);
                }
                goto next_iteration;
            }
            if (pid == 0) {
                /* Child process: set up stdin/stdout redirection as needed */

                /* If not the first command, redirect stdin to the read end of the previous pipe */
                if (i > 0) {
                    if (dup2(pipefd[i-1][0], STDIN_FILENO) < 0) {
                        perror("dup2 stdin");
                        exit(EXIT_FAILURE);
                    }
                }
                /* If not the last command, redirect stdout to the write end of the current pipe */
                if (i < command_count - 1) {
                    if (dup2(pipefd[i][1], STDOUT_FILENO) < 0) {
                        perror("dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }

                /* Close all pipe fds in the child (they have been duplicated or unused) */
                for (int j = 0; j < num_pipes; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }

                /* Now parse the i-th command into argv for execvp */
                char *args[64];
                int argc_argv = 0;
                char *arg = strtok(commands[i], " ");
                while (arg != NULL && argc_argv < 63) {
                    args[argc_argv++] = arg;
                    arg = strtok(NULL, " ");
                }
                args[argc_argv] = NULL;

                /* Execute the command */
                if (args[0] == NULL) {
                    /* No command to execute */
                    exit(EXIT_SUCCESS);
                }
                execvp(args[0], args);
                /* If execvp returns, there was an error */
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            /* Parent continues to fork next command */
        }

        /* Parent process: close all pipe fds */
        for (int i = 0; i < num_pipes; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }

        /* Wait for all child processes to finish */
        for (int i = 0; i < command_count; i++) {
            wait(NULL);
        }

    next_iteration:
        ;
    }
    return 0;
}
