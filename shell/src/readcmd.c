/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "readcmd.h"
#include "csapp.h"
#include <sys/types.h>



static void memory_error(void)
{
	errno = ENOMEM;
	perror(0);
	exit(1);
}


static void *xmalloc(size_t size)
{
	void *p = malloc(size);
	if (!p) memory_error();
	return p;
}


static void *xrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);
	if (!p) memory_error();
	return p;
}


/* Read a line from standard input and put it in a char[] */
static char *readline(void)
{
	size_t buf_len = 16;
	char *buf = xmalloc(buf_len * sizeof(char));

	if (fgets(buf, buf_len, stdin) == NULL) {
		free(buf);
		return NULL;
	}

	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	do {
		size_t l = strlen(buf);
		if ((l > 0) && (buf[l-1] == '\n')) {
			l--;
			buf[l] = 0;
			return buf;
		}
		if (buf_len >= (INT_MAX / 2)) memory_error();
		buf_len *= 2;
		buf = xrealloc(buf, buf_len * sizeof(char));
		if (fgets(buf + l, buf_len - l, stdin) == NULL) return buf;
	} while (1);
}


/* Split the string in words, according to the simple shell grammar. */
static char **split_in_words(char *line)
{
	char *cur = line;
	char **tab = 0;
	size_t l = 0;
	char c;

	while ((c = *cur) != 0) {
		char *w = 0;
		char *start;
		switch (c) {
		case ' ':
		case '\t':
			/* Ignore any whitespace */
			cur++;
			break;
		case '<':
			w = "<";
			cur++;
			break;
		case '>':
			w = ">";
			cur++;
			break;
		case '|':
			w = "|";
			cur++;
			break;
			case '&':
				w = "&";
				cur++;break;
		default:
			/* Another word */
			start = cur;
			while (c) {
				c = *++cur;
				switch (c) {
				case 0:
				case ' ':
				case '\t':
				case '<':
				case '>':
				case '|':
				case '&':
					c = 0;
					break;
				default: ;
				}
			}
			w = xmalloc((cur - start + 1) * sizeof(char));
			strncpy(w, start, cur - start);
			w[cur - start] = 0;
		}
		if (w) {
			tab = xrealloc(tab, (l + 1) * sizeof(char *));
			tab[l++] = w;
		}
	}
	tab = xrealloc(tab, (l + 1) * sizeof(char *));
	tab[l++] = 0;
	return tab;
}


static void freeseq(char ***seq)
{
	int i, j;

	for (i=0; seq[i]!=0; i++) {
		char **cmd = seq[i];

		for (j=0; cmd[j]!=0; j++) free(cmd[j]);
		free(cmd);
	}
	free(seq);
}


/* Free the fields of the structure but not the structure itself */
static void freecmd(struct cmdline *s)
{
	if (s->in) free(s->in);
	if (s->out) free(s->out);
	if (s->seq) freeseq(s->seq);
}


struct cmdline *readcmd(void)
{
	static struct cmdline *static_cmdline = 0;
	struct cmdline *s = static_cmdline;
	char *line;
	char **words;
	int i;
	char *w;
	char **cmd;
	char ***seq;
	size_t cmd_len, seq_len;

	line = readline();
	if (line == NULL) {
		if (s) {
			freecmd(s);
			free(s);
		}
		return static_cmdline = 0;
	}

	cmd = xmalloc(sizeof(char *));
	cmd[0] = 0;
	cmd_len = 0;
	seq = xmalloc(sizeof(char **));
	seq[0] = 0;
	seq_len = 0;

	words = split_in_words(line);
	free(line);

	if (!s)
		static_cmdline = s = xmalloc(sizeof(struct cmdline));
	else
		freecmd(s);
	s->err = 0;
	s->in = 0;
	s->out = 0;
	s->seq = 0;

	i = 0;
	while ((w = words[i++]) != 0) {
		switch (w[0]) {
		case '<':
			/* Tricky : the word can only be "<" */
			if (s->in) {
				s->err = "only one input file supported";
				goto error;
			}
			if (words[i] == 0) {
				s->err = "filename missing for input redirection";
				goto error;
			}
			s->in = words[i++];
			break;
		case '>':
			/* Tricky : the word can only be ">" */
			if (s->out) {
				s->err = "only one output file supported";
				goto error;
			}
			if (words[i] == 0) {
				s->err = "filename missing for output redirection";
				goto error;
			}
			s->out = words[i++];
			break;
		case '|':
			/* Tricky : the word can only be "|" */
			if (cmd_len == 0) {
				s->err = "misplaced pipe";
				goto error;
			}

			seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
			seq[seq_len++] = cmd;
			seq[seq_len] = 0;

			cmd = xmalloc(sizeof(char *));
			cmd[0] = 0;
			cmd_len = 0;
			break;
			case '&':
				if (s->fond)  {
				//	s->err = "misplaced '&' ";
					goto error;
				}
				s->fond = '&';
				break;
		default:
			cmd = xrealloc(cmd, (cmd_len + 2) * sizeof(char *));
			cmd[cmd_len++] = w;
			cmd[cmd_len] = 0;
		}
	}

	if (cmd_len != 0) {
		seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
		seq[seq_len++] = cmd;
		seq[seq_len] = 0;
	} else if (seq_len != 0) {
		s->err = "misplaced pipe";
		i--;
		goto error;
	} else
		free(cmd);
	free(words);
	s->seq = seq;
	return s;
error:
	while ((w = words[i++]) != 0) {
		switch (w[0]) {
		case '<':
		case '>':
		case '|':
		case '&':
			break;
		default:
			free(w);
		}
	}
	free(words);
	freeseq(seq);
	for (i=0; cmd[i]!=0; i++) free(cmd[i]);
	free(cmd);
	if (s->in) {
		free(s->in);
		s->in = 0;
	}
	if (s->out) {
		free(s->out);
		s->out = 0;
	}
	return s;
}
// l'utiliser pour le siganl child
void handler_CHILD(int sig){
 pid_t p;
 while((p = waitpid(-1, NULL, WNOHANG)) > 0);
}

	// executer une seule commande
	void execcmd(struct cmdline *l){
	int status;
	//pid_t p;

		/* code */

	//struct cmdline *l;
	// commande simple
	pid_t pid;
	//char *in;char *out;
	int red;int back =0; int cpt=0;
	// siganl child en utilisant handler_CHILD
	signal(SIGCHLD, handler_CHILD);
	if (l->fond=='&') {
		 back = 1;}
	pid= fork();
	if(pid==0){
		if(l->in !=0){
			red = open(l->in,O_RDONLY);
			if(red<0){
				printf(" %s : Permission denied. \n", l->in);exit(1);
			}
			else {
				dup2(red,0); close(red);
						}
		}

		if (l->out != 0) {
			red= open(l->out,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
			if (red < 0) {
				printf(" %s : Permission denied. \n",l->out);exit(1);
			}
			else {
				dup2(red, 1);close(red);
			}
		}



		//
	int exec =execvp(l->seq[0][0],l->seq[0]);
	if (exec ==-1){
	printf(" %s : command not found! \n",l->seq[0][0]);
	exit(EXIT_FAILURE);
	}}

	if (back) {
	cpt++;
		printf(" (%d)+ %d\n",cpt,pid);
return;
}

	    //pr les zombies

	 while(waitpid(-1, &status, WNOHANG|WUNTRACED)!=-1);
}




	void pipeline(struct cmdline *l){
 pid_t pid;
 int i, ind, outd, status, ex;i=0;
 int p1[2]; int p2[2];int last,first;
 last=0;first=1;






	if (pipe(p2)<0) {
 		printf("ERROR, le pipe n'est pas crée ");
		exit(1);
 			}


 			// verifier s'il reste une commande:
			while (l->seq[i]!=0) {
				pid=fork();

				if (pid==0) {
					/* code */
							if (first==1) {
								/* code */
									if (l->in!=0) {
											ind=open(l->in, O_RDONLY);
											if (ind<0) {
											  printf(" ERROR, on peut pas ouvrir le fichier \n" );
												exit(1);

																 }
											if (dup2(ind,0) < 0 ) {
												printf(" ERROR du pulication 1");
												exit(1);
																						}
																  }

						                	}
							else{
								  close(p1[1]);
									if (dup2(p1[0],0)<0) {
										printf("ERROR de duplication 2" );
										exit(1);
									 												}


						     	  }

							// not last cmnd
							if (last==0) {
								close(p2[0]);

								if (dup2(p2[1],1)<0) {
									printf("ERROR de duplication 3");exit(1);
																			}

														}
							// the last cmnd

							if (last==1 && l->out!=0) {
								outd=open(l->out,O_WRONLY | O_CREAT );
								if (outd<0) {
									printf(" ERROR, on peut pas ouvrir le fichier " );
									exit(1);
								   					}

									if (dup2(outd,STDOUT_FILENO)<0) {
										printf("ERROR de duplication 4 ");
										exit(1);
																			}

																						}

				//exec
				ex=execvp(l->seq[i][0],l->seq[i]);
				if (ex== -1) {
				 printf("ERROR command not found");exit(1);
									}
}
							else{
								if (first==0) {
									close(p1[0]);

							               	}
									if (l->seq[i+1]!=0) {
										close(p2[1]);
									}
								//
								p1[1]=p2[1];
								p1[0]=p2[0];


								// zombies
								while (waitpid(-1,&status, WNOHANG|WUNTRACED)!=-1);
									}

i++;
		first=0;
					// not the last cmnd
					if (l->seq[i+1]==0) {
						last=1;
															}
															if (last==0) {
																if (pipe(p2)<0) {
																	printf("ERROR, le pipe n'est pas crée");
																	exit(1);
																								}
															}

							}
							for (int i = 0;l->seq[i]!=0; i++) {
								close(p1[0]);
	          		close(p2[0]);
	 							close(p1[1]);
	 							close(p2[1]);

																								}








}
