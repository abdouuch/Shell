/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"
#include <sys/types.h>


int main()
{
//int status;
	while (1) {
		struct cmdline *l;
		//int i, j;

		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}
	// c'est la cas ou aucune commande n'est tapée
		if(l->seq[0]==0){
			continue;}
	// étape pour quiter le Shell
	if(strcmp(l->seq[0][0],"quit")==0 || strcmp(l->seq[0][0],"q")==0 ) { printf(" quiter le Shell \n");
	exit(0);}



//}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);

		/* Display each command of the pipe */
		/*for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (j=0; cmd[j]!=0; j++) {
				printf("%s ", cmd[j]);
			}
			printf("\n");
		}*/

	// le cas d'une seule commande:
	if (l->seq[1]==0) {
		execcmd(l);
		//printf("hhhhhhhhhhhhhhh");
	}
	else{
		//printf("oooooooooooo");
		pipeline(l);
	}
	//execcmd(l);
	//onecommand(l);
	}

}
