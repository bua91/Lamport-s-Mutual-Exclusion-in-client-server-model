/*
 * FILE NAME: server.c
 * OWNER: ARUNABHA CHAKRABORTY
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "server.h"

/*
 * server code
 */
int server()
{
        int master_sock_fd, new_conn_fd, fd, cli_conn_fds[6];
	int max_clients = 6;
	struct sockaddr_in server_addr, cli_addr;
	socklen_t cli_len;
	fd_set readfds;
	int max_fd;
	int i = 0;
	FILE *fp;
	char send_buffer[1024] = {0};
	char recv_buffer[1024] = {0};
	strcpy(send_buffer, "server");
	int activity;
	
	//Inotialize all client socket fds to 0.
	for (i = 0; i < max_clients; i++){
		cli_conn_fds[i] = 0;
	}
	
	//Open a socket connection
	if ((master_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "serversh: Error in socket creation!!\n");
		return 0;
	}

	//clear the server_addr structure
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(9000);
	
	//BIND the socket to localhost port 9000
	if (bind(master_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		fprintf(stderr, "serversh: Error in bind!!\n");
		return 0;
	}

	//Put the server in the listening passive mode with max pending connections as 10
	if (listen(master_sock_fd, 10)<0){
		fprintf(stderr, "serversh: Error in listen!!\n");
		return 0;
	}
	
	printf("server started\n\n");

	while(1){
		//clear the socket set and add the master socket to the set
		FD_ZERO(&readfds);
		FD_SET(master_sock_fd, &readfds);
		max_fd = master_sock_fd;
		cli_len = sizeof(cli_addr);
		
		//Add client specific socket fds to the set
		for (i = 0; i < max_clients; i++){
			fd = cli_conn_fds[i];
			if ( fd > 0){
				//That means client socket exists
				FD_SET(fd, &readfds);
			}
			if ( fd > max_fd ){
				max_fd = fd;
			}
		}

		//wait for some new connection 
		activity = select (max_fd+1, &readfds, NULL, NULL, NULL);

		if ( (activity < 0) && (errno != EINTR)){
			fprintf(stderr, "serversh: Error in select call!!\n");
		}
		
		//check if there is a new incoming client connection
		if (FD_ISSET(master_sock_fd, &readfds)){
			if ((new_conn_fd = accept(master_sock_fd, (struct sockaddr *)&cli_addr, &cli_len))<0){
				fprintf(stderr, "serversh: Error in accept!!\n");
				return 0;
			}
			if (read(new_conn_fd, recv_buffer, 1024) < 0){
				fprintf(stderr, "serversh: Error in reading hello from client!!\n");
			}
			if (!strcmp(recv_buffer, "hello")){
				if (send(new_conn_fd, send_buffer, strlen(send_buffer), 0) < 0){
					fprintf(stderr,"serversh: error in sending reply to the other client's hello msg!!\n");
				}
			}
			 //add the new connection fd to the client connection fd list
			 for (i = 0; i < max_clients; i++){
			 	//check for empty slot
				if (cli_conn_fds[i] == 0){
					cli_conn_fds[i] = new_conn_fd;
					break;
				}
			}
			memset(recv_buffer, 0, sizeof(recv_buffer));
			memset(recv_buffer, 0, sizeof(send_buffer));
		}
		//else its some IO operation for already connected clients
		else{
			for (i = 0; i < max_clients; i++){
				if (FD_ISSET(cli_conn_fds[i], &readfds)){
					if (read(cli_conn_fds[i], recv_buffer, 1024) <0){
						fprintf(stderr, "serversh: Error in reading message from client!!\n");
					}
					char *token;
					int m =0;
					char file_name[10];
					char write_string[100];
					token = strtok(recv_buffer, " ");
					strcpy(file_name, token);
					while (token != NULL){
						if (m == 1){
							strcpy(write_string, token);
							strcat(write_string, " ");
						}
						if (m == 2){
							strcat(write_string, token);
						}
						m++;
						token = strtok(NULL, " ");
					}
					if (!strncmp(file_name, "test1", 5)){
						fp = fopen("server1_files/test1.txt", "a+");
					}
					else{
						fp = fopen("server1_files/test2.txt", "a+");
					}
					fprintf(fp, "%s\n", write_string);
					fclose(fp);
					//send(cli_conn_fds[i], send_buffer, strlen(sennd_buffer), 0);
					//read write data to file
				}
			}

		}
	}

}
