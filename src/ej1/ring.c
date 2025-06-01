#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{	
	int start, status, pid, n;
	int buffer[1];

	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}
    
    /* Parsing of arguments */
	n = atoi(argv[1]);
	buffer[0] = atoi(argv[2]);
	start = atoi(argv[3]);

    if (n < 3 || start < 1 || start > n) {
        fprintf(stderr, "Argumentos inválidos. Asegúrese de que n ≥ 1 y 1 ≤ s ≤ n.\n");
        exit(EXIT_FAILURE);
    }
    printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], start);
    
	/* 1) Crear n pipes */
    int pipes[n][2];
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

	/* 2) Fork de los n hijos, indexados de 1 a n */
	for (int i = 1; i <= n; i++) {
    	pid = fork();
    	if (pid < 0) {
        	perror("fork");
        	exit(EXIT_FAILURE);
    	}
    	if (pid == 0) {
        	/* --- Código del hijo número i (1-based) --- */
	        int idx  = i;
    	    int prev = (idx == 1 ? n : idx - 1);
        	int next = (idx == n ? 1 : idx + 1);

	        /* Descriptores que este hijo usará */
    	    int read_fd  = pipes[prev - 1][0];  /* lee de (prev → idx) */
        	int write_fd = pipes[idx - 1][1];   /* escribe a (idx → next) */

	        /* 2.a) Cerrar todos los extremos de pipe que NO se usarán */
    	    for (int j = 0; j < n; j++) {
        	    if (j != (prev - 1)) {
            	    close(pipes[j][0]);
	            }
    	        if (j != (idx - 1)) {
        	        close(pipes[j][1]);
            	}
	        }

    	    /* 2.b) Leer un entero, incrementarlo y reenviar */
        	int valor;
	        if (read(read_fd, &valor, sizeof(int)) != sizeof(int)) {
    	        perror("read en hijo");
        	    exit(EXIT_FAILURE);
        	}
	        valor++;  /* Incrementa el mensaje */

	        if (write(write_fd, &valor, sizeof(int)) != sizeof(int)) {
    	        perror("write en hijo");
        	    exit(EXIT_FAILURE);
	        }

    	    /* 2.c) Cerrar los descriptores que usó */
        	close(read_fd);
        	close(write_fd);

	        /* Termina este hijo */
    	    _exit(EXIT_SUCCESS);
	    }
    /* El padre continúa creando el siguiente hijo */
	}
	int prev_start = (start == 1 ? n : start - 1);
    int write_fd_parent = pipes[prev_start - 1][1];
    int read_fd_parent = pipes[prev_start - 1][0];

    /* 4) Cerrar en el padre todos los extremos que NO usará */
    for (int j = 0; j < n; j++) {
        if (j != (prev_start - 1)) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
    }

    /* 5) Inyectar el mensaje inicial en el anillo */
    if (write(write_fd_parent, buffer, sizeof(int)) != sizeof(int)) {
        perror("write inicial en padre");
        exit(EXIT_FAILURE);
    }

    /* 6) Leer el mensaje final que regresa */
    int resultado;
    if (read(read_fd_parent, &resultado, sizeof(int)) != sizeof(int)) {
        perror("read final en padre");
        exit(EXIT_FAILURE);
    }
    printf("Mensaje final recibido en el padre: %d\n", resultado);

    /* 7) Cerrar los fds usados */
    close(write_fd_parent);
    close(read_fd_parent);

    /* 8) Esperar a que terminen todos los hijos */
    for (int i = 0; i < n; i++) {
        wait(&status);
    }

    return 0;
}