#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{	
	int start, status, pid, n;
	int buffer[1];

	if (argc != 4){ printf("Uso: anillo <n> <c> <s>\n"); exit(EXIT_FAILURE); }

	/* Parsing of arguments */
	n = atoi(argv[1]);
	buffer[0] = atoi(argv[2]);
	start  = atoi(argv[3]);

	if (n < 3 || start < 0 || start >= n){
		fprintf(stderr,"Error: n >= 3 y 0 <= s<n\n");
		exit(EXIT_FAILURE);
	}

	printf("Se crearán %i procesos, se enviará el valor %i desde el proceso %i\n",
	       n, buffer[0], start);

	// Pipes del ring hijo-hijo
	int ring[n][2];
	for (int i = 0; i < n; ++i)
		if (pipe(ring[i]) == -1){ perror("pipe"); exit(EXIT_FAILURE); }

	// Pipes padre si proceso start
	int p2s[2], s2p[2];
	if (pipe(p2s) == -1 || pipe(s2p) == -1){ perror("pipe"); exit(EXIT_FAILURE); }

	// Crea los procesos hijos
	for (int i = 0; i < n; ++i){
		pid = fork();
		if (pid == -1){ perror("fork"); exit(EXIT_FAILURE); }

		if (pid == 0){               
			int in_fd  = ring[(i + n - 1) % n][0]; 
			int out_fd = ring[i][1];               

			int read_parent  = -1, write_parent = -1;
			if (i == start){
				read_parent  = p2s[0];
				write_parent = s2p[1];
			}
			for (int j = 0; j < n; ++j){
				if (ring[j][0] != in_fd) close(ring[j][0]);
				if (ring[j][1] != out_fd) close(ring[j][1]);
			}
			close(p2s[1]); close(s2p[0]);
			if (i != start){ close(p2s[0]); close(s2p[1]); }

			int val;

			if (i == start){

				read(read_parent, &val, sizeof val);
				++val;
				write(out_fd, &val, sizeof val);

				read(in_fd, &val, sizeof val);
				++val;
				write(write_parent, &val, sizeof val);
			}else{
				read(in_fd, &val, sizeof val);
				++val;
				write(out_fd, &val, sizeof val);
			}
			_exit(EXIT_SUCCESS);
		}
	}

	// Padre
	for (int i = 0; i < n; ++i){
		close(ring[i][0]);
		close(ring[i][1]);
	}

	close(p2s[0]);
	close(s2p[1]);

	write(p2s[1], buffer, sizeof buffer);

	read(s2p[0], buffer, sizeof buffer);
	printf("Valor final recibido por el padre: %d\n", buffer[0]);

	while (wait(&status) > 0);

	return 0;
}

