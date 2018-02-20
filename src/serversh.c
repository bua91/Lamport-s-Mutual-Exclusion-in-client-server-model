/*
 * FILE NAME: lmush.c
 * OWNER: ARUNABHA CHAKRABORTY
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include "lmu.h"

#define TOK_BUFF_SIZE 128
#define TOK_DELIMITER " \t\r\n\a"

/* 
 *Function decleration for built in functions
 */
int lmush_cd(char **args);
int lmush_help(char **args);
int lmush_exit(char **args);

/*
 * Built in function implementations.
 * ************************************************************
 * ************************************************************
 * NAME: lmush_help
 * PURPOSE: Display help string for built in cmds.
 */
int lmush_help(char **args)
{
	printf("LMUSH SUPPORTED BUILT IN COMANDS\n");
	printf("help                                   Displayes the helpline\n");
	printf("cd                                     Changes directory\n");
	printf("enquiry                                To fetch details about the hosted files. Issued by clients\n");
	printf("show hosted-files                      To show the list of files hosted by the servers\n");
	printf("exit                                   Exit p2psh\n");
	return 1;
}

/*
 * NAME: lmush_cd
 * PURPOSE: change directory
 */
int lmush_cd(char **args)
{
	if (args[1] == NULL){
		fprintf(stderr, "lmush: Expected argument to cd!!\n");
	}
	else{
		if(chdir(args[1]) !=0){
			perror("lmush");
		}
	}
	return 1;
}

/*
 * NAME: lmush_exit
 * PURPOSE: To exit from lmush shell.
 * WARNING: All configurations will be gone.
 */
int lmush_exit(char **args)
{
	return 0;
}
/*
 * NAME: read_line
 * RETURN_TYPE: char *
 * PURPOSE: To read a cmd typed in the shell
 */
char * read_line()
{
        char *line;
	ssize_t buff_size = 0;
	getline(&line, &buff_size, stdin);
	return line;
}

/*
 * NAME: parse_line
 * RETURN_TYPE: array of \0 terminated strings
 * PURPOSE: To tokenize the input cmd string and return array of \0 terminated string
 *          arguments of the cmd.
 */
char ** parse_line(char *line)
{
	int buff_size = TOK_BUFF_SIZE, i = 0;
	char *token;
	char **cmd_arguments = malloc(buff_size * sizeof(char*));
	if(!cmd_arguments){
		fprintf(stderr, "lmush: allocation error!!\n");
		exit(EXIT_FAILURE);
	}
	token = strtok(line, TOK_DELIMITER);
	while (token != NULL){
		cmd_arguments[i] = token;
		i++;

		if (i >= buff_size){
			buff_size += TOK_BUFF_SIZE;
			cmd_arguments = realloc(cmd_arguments, buff_size*sizeof(char*));
			if (!cmd_arguments){
			 	fprintf(stderr, "lmush: allocation error!!\n");
			 	exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, TOK_DELIMITER);
	}
	cmd_arguments[i] = NULL;
	return cmd_arguments;
}

/*
 * NAME: cmd_launch
 * RETURN_TYPE: integer
 * PURPOSE: To create a child process and execute the native linux shell cmds
 *          with exception of exit and cd.
 */
int cmd_launch(char **args)
{
	pid_t pid, wpid;
  	int status;

  	pid = fork();
  	if (pid < 0){
		 // Error forking
		 perror("lmush");
  	}
  	else if (pid == 0){
  		// Child process
    		if (execvp(args[0], args) == -1){
			perror("lmush");
		}
		exit(EXIT_FAILURE);
  	}
  	else{
  		// Parent process
		do{
			wpid = waitpid(pid, &status, WUNTRACED);
		}while (!WIFEXITED(status) && !WIFSIGNALED(status));
  	}	
  	return 1;
}

/*
 * NAME: cmd_execute
 * RETURN_TYPE: integer
 * PURPOSE: To execute the p2psh built in cmds and other native linux shell cmds.
 */
int cmd_execute (char **args)
{
	int i;
	if (args[0] == NULL)
		return 1;
#if 0
	for(i = 0; i < p2psh_num_built_in; i++){
	if (strcmp(args[0],built_in_cmd_str[i]) == 0){
		return (*built_in_cmd_fun_callbacks[i])(args);
		}
	}
#endif
	if (strcmp (args[0],"help") == 0){
		return (p2psh_help(args));
	}
	else if (strcmp (args[0], "cd") == 0){
		return (p2psh_cd(args));
	}
	else if (strcmp (args[0], "exit") == 0){
		return (p2psh_exit(args));
	}
	else if (strcmp (args[0], "peer") == 0){
		if (args[2] == NULL){
			fprintf(stderr, "p2psh: Port not provided!!\n");
			return 1;
		}
		else if (strcmp(args[2],"9000") != 0){
			fprintf(stderr, "p2psh: Peer port should be 9000!!\n");
			return 1;
		}
		return (peer_connect(args[1], args[2]));
	}
	else if (strcmp (args[0], "publish") == 0){
		if ( args[1] == '\0'){
			fprintf(stderr, "p2psh: filename not provided!!\n");
			return 1;
		}
		if (publish_data(args[1]) == 0){
			fprintf(stderr, "p2psh: publish failed!!\n");
			return 1;
		}
		return 1;
	}
	else if (strcmp (args[0], "unpublish") == 0){
		return unpublish_data(args[1]);
	}
	else if (strcmp (args[0], "show") == 0){
		if (strcmp (args[1], "peer") == 0){
			return (show_peers());
		}
		else if (strcmp (args[1], "metadata") == 0){
			return (show_metadata());
		}
		else if (strcmp (args[1], "published") == 0){
			return (show_published());
		}
	}
	return (cmd_launch(args));
}

/*
 * Initialize the global variables.
 */
int initialize()
{
	head = NULL;
	meta_head = NULL;
	return 1;
}

/*
 * function for starting local server
 */
void *local_server(void * arg)
{
	int ret;
	ret = server();
	return ret;
}

/*
 * MAIN FUNCTION
 */
int main(int argc, char **argv)
{
        char *cmd_str;
        char **cmd_args;
        int status;
	pthread_t tid;
	int err;
	initialize();
	printf("=======================================================\n\n");
	printf("                        P2PSH                          \n\n");
	printf("       (CREATORS: ARUNABHA CHAKRABORTY                 \n\n");
	printf("                  AMAN SACHAN                          \n\n");
	printf("=======================================================\n\n");
	/* Run local server in a new thread upon starting p2psh*/
	err = pthread_create(&tid, NULL, local_server, NULL);
	if (err != 0 ){
		fprintf(stderr, "p2psh: server thread creation error");
	}
	/* Create metadata table for the local files*/
	metadata();
	/* Start the shell and execute comands*/
        do{
                printf("P2PSH>>");
                cmd_str = read_line();
                cmd_args = parse_line(cmd_str);
                status = cmd_execute(cmd_args);
        }while(status);
        return EXIT_SUCCESS;
}

