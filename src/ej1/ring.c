#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * Ring communication among n child processes.
 * Usage: ./anillo <n> <c> <s>
 *   <n> = número de procesos hijos en el anillo (must be ≥1)
 *   <c> = valor del mensaje inicial (int)
 *   <s> = índice (1-based) del proceso que inicia la comunicación (1 ≤ s ≤ n)
 *
 * El padre crea n pipes y n hijos. Cada hijo i (1-based) lee de su predecessor
 * en el anillo, incrementa el entero recibido, y se lo envía al sucesor.
 * El padre inyecta el mensaje inicial C en el proceso S escribiendo en el pipe
 * correspondiente al predecessor de S. Cuando el mensaje ha dado una vuelta
 * completa, el proceso predecessor de S escribe el valor final en el mismo pipe,
 * de donde el padre lo lee y lo imprime por pantalla. Cada hijo lee y escribe
 * exactamente una vez y luego sale.
 */

int main(int argc, char **argv)
{	
	int start, status, pid, n;
	int buffer[1];

	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}
    
    /* Parsing of arguments */
  	n = atoi(argv[1]);
  	buffer[0] = atoi(argv[2]);   /* valor inicial C */
  	start = atoi(argv[3]);
  	if (n < 1 || start < 1 || start > n) {
  	    fprintf(stderr, "Argumentos inválidos. Debe ser: n ≥ 1 y 1 ≤ s ≤ n.\n");
  	    exit(EXIT_FAILURE);
  	}

    printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], start);
    
   	/* You should start programming from here... */
   	
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
   	        /* Esto es el hijo con índice 'i' (1-based) */
   	        int idx  = i;
   	        int prev = (idx == 1 ? n : idx - 1);
   	        int next = (idx == n ? 1 : idx + 1);
   	        int read_fd  = pipes[prev - 1][0];  /* lee de (prev)→idx */
   	        int write_fd = pipes[idx - 1][1];   /* escribe de idx→(next) */

   	        /* Cerrar todos los extremos que no va a usar */
   	        for (int j = 0; j < n; j++) {
   	            if (j != (prev - 1)) {
   	                close(pipes[j][0]);
   	            }
   	            if (j != (idx - 1)) {
   	                close(pipes[j][1]);
   	            }
   	        }

   	        /* Leer, incrementar y reenviar */
   	        if (read(read_fd, buffer, sizeof(int)) != sizeof(int)) {
   	            perror("read en hijo");
   	            exit(EXIT_FAILURE);
   	        }
   	        buffer[0]++;

   	        if (write(write_fd, buffer, sizeof(int)) != sizeof(int)) {
   	            perror("write en hijo");
   	            exit(EXIT_FAILURE);
   	        }

   	        /* Cerrar fds usados */
   	        close(read_fd);
   	        close(write_fd);

   	        /* Terminar este hijo */
   	        _exit(EXIT_SUCCESS);
   	    }
   	    /* El padre continúa creando hijos */
   	}

   	/* Código que ejecuta sólo el padre */

   	/* Determinar el índice 0-based del predecessor de 'start' */
   	int prev_start = (start == 1 ? n : start - 1);
   	int write_fd_parent = pipes[prev_start - 1][1];
   	int read_fd_parent  = pipes[prev_start - 1][0];

   	/* Cerrar todos los extremos que el padre no va a usar */
   	for (int j = 0; j < n; j++) {
   	    if (j != (prev_start - 1)) {
   	        close(pipes[j][0]);
   	        close(pipes[j][1]);
   	    }
   	}

   	/* Inyectar el mensaje inicial en el anillo */
   	if (write(write_fd_parent, buffer, sizeof(int)) != sizeof(int)) {
   	    perror("write inicial en padre");
   	    exit(EXIT_FAILURE);
   	}

   	/* Leer el mensaje final que regresa */
   	int resultado;
   	if (read(read_fd_parent, &resultado, sizeof(int)) != sizeof(int)) {
   	    perror("read final en padre");
   	    exit(EXIT_FAILURE);
   	}

   	printf("Mensaje final recibido en el padre: %d\n", resultado);

   	/* Cerrar los fds usados */
   	close(write_fd_parent);
   	close(read_fd_parent);

   	/* Esperar a que terminen todos los hijos */
   	for (int i = 0; i < n; i++) {
   	    wait(&status);
   	}

   	return 0;
}

