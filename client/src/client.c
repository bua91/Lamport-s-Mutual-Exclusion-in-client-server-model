/*
 * FILE NAME: client.c
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
#include "client.h"

/*
 * Insert remote client data to request queue
 */
int insert_into_request_queue(char * t_hstnm, char * t_tmstmp)
{
	struct request *temp = (struct request*) malloc(sizeof(struct request));
	long int t = atoi(t_tmstmp);
	if (t == 0){
		return 0;
	}
	strcpy(temp->cli_id, t_hstnm);
	temp->t_stamp = t;
	temp->next = head;
	head = temp;
	
	//update the local clock
	if (local_clock >= t){
		local_clock += 1;
	}
	else if (local_clock < t){
		local_clock = t+1;
	}
	return 1;
}

/*
 * delete data from request queue
 */
int delete_from_request_queue(char * t_hstnm, char * t_tmstmp)
{
	struct request *temp = head;
	struct request *prev = head;
	if (temp == NULL){
		fprintf(stderr, "clientsh: no nodes in the request list!!\n");
	}
	//if only one node or first node to be deleted
	else if ((temp->next == NULL) || (!strcmp(temp->cli_id, t_hstnm))){
		if (!strcmp(temp->cli_id, t_hstnm)){
			head = head->next;
			temp->next = NULL;
		}
	}
	else {
		temp = temp->next;
		while (temp != NULL){
			if (!strcmp(temp->cli_id, t_hstnm)){
				prev->next = temp->next;
				temp->next = NULL;
			}
			prev = temp;
			temp = temp->next;
		}
	}
	//update the local clock
	local_clock ++;

	return 1;
}

/*
 * server part of the client
 */
int server()
{
        int master_sock_fd, new_conn_fd, fd;
	struct sockaddr_in server_addr, cli_addr;
	socklen_t cli_len;
	int i = 0;
	int max_fd, activity;
	char send_buffer[1024] = {0};
	char recv_buffer[1024] = {0};
	strcpy(send_buffer, "client");
	
	//Inotialize all client socket fds to 0.
	for (i = 0; i < max_clients; i++){
		cli_conn_fds[i] = 0;
	}

	
	//Open a socket connection
	if ((master_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "clientsh(SERVER CODE): Error in socket creation!!\n");
		return 0;
	}

	//clear the server_addr structure
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(9000);
	
	//BIND the socket to localhost port 9000
	if (bind(master_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		fprintf(stderr, "clientsh(SERVER CODE): Error in bind!!\n");
		return 0;
	}

	//Put the server in the listening passive mode with max pending connections as 10
	if (listen(master_sock_fd, 10)<0){
		fprintf(stderr, "clientsh(SERVER CODE): Error in listen!!\n");
		return 0;
	}
	

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
			fprintf(stderr, "clientsh(SERVER CODE): Error in select call!!\n");
		}
		
		//check if there is a new incoming client connection
		if (FD_ISSET(master_sock_fd, &readfds)){
			if ((new_conn_fd = accept(master_sock_fd, (struct sockaddr *)&cli_addr, &cli_len))<0){
				fprintf(stderr, "clientsh(SERVER CODE): Error in accept!!\n");
			}

			if (read(new_conn_fd, recv_buffer, 1024) < 0){
				fprintf(stderr, "clientsh(SERVER CODE): Error in reading hello from client!!\n");
			}

			if (!strcmp(recv_buffer, "hello")){
				//send "client" message as a reply to the hello message from other client
				if (send(new_conn_fd, send_buffer, strlen(send_buffer), 0) < 0){
					fprintf(stderr,"clientsh (SERVER CODE): error in sending reply to the other client's hello msg!!\n");
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
			// At last clear the recv and send buffer so that it can be used later
			memset(recv_buffer, 0, sizeof(recv_buffer));
			memset(send_buffer, 0, sizeof(send_buffer));
		}
		//else its some IO operation for already connected clients
		else{
			for (i = 0; i < max_clients; i++){
				if (FD_ISSET(cli_conn_fds[i], &readfds)){
					if(read(cli_conn_fds[i], recv_buffer, 1024) < 0){
					        fprintf(stderr, "clientsh(SERVER CODE): Error in reading hello from client!!\n");
			                }
					if (!strncmp(recv_buffer, "req", 3)){
						//parse recv buffer
						char *token;
						int m =0;
						char temp_hostname[30];
						char temp_timestamp[30];
						char ck[30];
						token = strtok (recv_buffer, " ");
						while(token != NULL){
							if (m == 1){
								strcpy(temp_hostname, token);
							}
							else if (m == 2){
								strcpy(temp_timestamp, token);
							}
							m++;
							token = strtok (NULL, " ");
						}

						//insert remote client request to the request queue and increment the local clock
						if (!insert_into_request_queue(temp_hostname, temp_timestamp)){
							fprintf(stderr, "clientsh(SERVER CODE): error in inserting remote request to queue!!\n");
						}

						//send "reply" message to the requesting client
						strcpy(send_buffer, "reply ");
						sprintf(ck, "%ld", local_clock);
						strcat(send_buffer, ck);
						if (send(cli_conn_fds[i], send_buffer, strlen(send_buffer), 0) < 0){
							fprintf(stderr,"clientsh (SERVER CODE): error in sending reply to the other client's REQUEST msg!!\n");
						}
					}
					else if (!strncmp(recv_buffer, "rel", 3)){
						//parse the recv buffer
						char *token;
						int m =0;
						char temp_hostname[30];
						char temp_timestamp[30];
						token = strtok(recv_buffer, " ");
						while(token != NULL){
							if (m == 1){
								strcpy(temp_hostname, token);
							}
							else if (m == 2){
								strcpy(temp_timestamp, token);
							}
							m++;
							token = strtok(NULL, " ");
						}

						//delete data from request queue
						if (!delete_from_request_queue(temp_hostname, temp_timestamp)){
							fprintf(stderr, "clientsh(SERVER CODE): error in inserting remote request to queue!!\n");
						}
					}
				}
   			}
                        memset(recv_buffer, 0, sizeof(recv_buffer));
	                memset(send_buffer, 0, sizeof(send_buffer));
		}
	}

	return 1;
}

/*
 * This function checks if the socket represented by the sock_fd is client to server
 * socket or not
 */
int is_client_to_server_socket( int sock_fd)
{
	int m = 0;
	int ret = 0;
	for (m=0; m<3; m++){
		if(server_fds[m] == sock_fd){
			ret = 1;
		}
	}
	return ret;
}

/*
 * This fuction checks if the local request is at the head of the queue
 */
int is_local_request_head_of_queue()
{
	//check if local request is head of queue.
	struct request * temp = head;
	char temp_host[30];
	char hostname[30];
	long int min = temp->t_stamp;
	strcpy(temp_host, temp->cli_id);
	temp = temp->next;
	while (temp != NULL){
		if (temp->cli_id < min){
			min = temp->t_stamp;
			strcpy(temp_host, temp->cli_id);
		}
		temp = temp->next;
	}
	gethostname(hostname, 30);
	if (!strcmp(temp_host, hostname)){
		return 1;
	}
	return 0;
}

/*
 * Client functionality part of client node
 */
int peer_connect(char *ip_address)
{
        int sock_fd;
	struct sockaddr_in server_addr;
	char send_buffer[1024] = {0};
	char recv_buffer[1024] = {0};
	int i, m, rd;
	int random = 0;
	int check = 0;
	int release_check = 0;
	strcpy(send_buffer, "hello");

        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	        fprintf(stderr, "clientsh(CLIENT PART): Error in socket creation!!\n");
	        return 0;
	}
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(9000);
	
	if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0){
		fprintf(stderr, "clientsh(CLIENT PART): Error in IP address!!\n");
		return 0;
	}

	if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		fprintf(stderr, "clientsh(CLIENT PART): Error in socket connect!!\n");
		return 0;
	}
	
	//send hello to the new client/server
	if (send(sock_fd, send_buffer, strlen(send_buffer), 0) < 0){
		fprintf(stderr,"clientsh: error in sending request to other client!!\n");
	}

	//read the greeting reply from the other clients and the servers
	rd = read(sock_fd, recv_buffer, 1024);
	if ((strncmp(recv_buffer, "server", 6) == 0)){
		for ( m = 0; m<3; m++){
			if(server_fds[m] == 0){
				server_fds[m] = sock_fd;
			}
		}
	}

	no_of_conn++;
	srand(time(0));

	//infinite loop to let the client socket open untill the program ends
	while (1){
		//set check to 0 again once you got replies from all the other clients
		if (peer_cli_replies >= 4){
			check = 0;
			release_check = 0;
			local_clock++;
			//local_clock_send_timer = 0;
		}
		random = (rand() % 10) +1;
		sleep (random);

		//Check if all servers and clients are connected
		if (no_of_conn >= 7) {
			if(peer_cli_replies >= 4){
				if (is_client_to_server_socket(sock_fd) && is_local_request_head_of_queue()){
					request_to_server_send(sock_fd);
					/* set the peer_cli_replies back to 0 so that other client-to-server threads
					 * dont send write request in parallel */
					release_sent = 0;
					peer_cli_replies = 0;
					local_clock_send_timer = 0;
				}
			}
			else {
				if ((!is_client_to_server_socket(sock_fd)) && (check == 0) && (release_sent >= 4)){
					client_send_request(sock_fd);
					peer_cli_replies++;
					/* So that the same client-to-client connection dosen't send requests multiple times
					 * before gitting replies from other clients. */
					check = 1;
					release_check = 0;
				}
				else if ((!is_client_to_server_socket(sock_fd)) && (release_check == 0) && (release_sent < 4)){
					send_release(sock_fd);
					release_sent++;
					release_check = 1;
				}
			}
		}

	}
	
	close(sock_fd);
	return 1;
}

/*
 * send request to server for writing to a file hosted by the server 
 */
int request_to_server_send(int sock_fd)
{
	char send_buffer[1024] = {0};
	char hst_nm[30];
	char cli_id_temp[30];

	if (file_no == 1){
		strcpy(send_buffer, "test1 ");
		file_no = 2;
	}
	else {
		strcpy(send_buffer, "test2 ");
		file_no = 1;
	}
	gethostname(hst_nm, 30);
	strcat(send_buffer, hst_nm);
	strcat(send_buffer, " ");
	sprintf(cli_id_temp, "%ld", local_clock_send_timer);
	strcat(send_buffer, cli_id_temp);
	if (send(sock_fd, send_buffer, strlen(send_buffer), 0) < 0){
		fprintf(stderr,"clientsh: error in sending write request to server!!\n");
	}
	return 1;
}

/*
 * send release after completing file write
 */

void send_release(int sock_fd)
{
	 char send_buffer[1024] = {0};
	 char ck[30];
	 char hostname_temp[30];
	 
	 gethostname(hostname_temp, 30);
	 strcpy(send_buffer, "release ");
         strcat(send_buffer, hostname_temp);
	 strcat(send_buffer, " ");
	 sprintf(ck, "%ld", local_clock);
	 strcat(send_buffer, ck);

	if (send(sock_fd, send_buffer, strlen(send_buffer), 0) < 0){
		fprintf(stderr,"clientsh: error in sending request to other client!!\n");
	}

}

/*
 * Lamports mutual exclusion implementation. REQUEST send to other clients
 */
void client_send_request(int sock_fd)
{
	struct request *temp = (struct request*) malloc(sizeof(struct request));
	char send_buffer[1024] = {0};
	char recv_buffer[1024] = {0};
	char ck[30];
	int rd;
	char hostname_temp[30];

	//fetching the hostname
	gethostname(hostname_temp, 30);
	
	if (local_clock_send_timer == 0){
		temp->next = head;
		//temp->socket_fd = sock_fd;
		//fetching the local clock time
		temp->t_stamp = local_clock;
		//copy hostname to the structure.
		strcpy(temp->cli_id, hostname_temp);
		head = temp;
		local_clock_send_timer = local_clock;
	}

	//construct the message to be sent
	strcpy(send_buffer, "request ");
	strcat(send_buffer, hostname_temp);
	strcat(send_buffer, " ");
	sprintf(ck, "%ld", local_clock_send_timer);
	strcat(send_buffer, ck);
	
	//Send request to other clients.
	if (send(sock_fd, send_buffer, strlen(send_buffer), 0) < 0){
		fprintf(stderr,"clientsh: error in sending request to other client!!\n");
	}

	rd = read(sock_fd, recv_buffer, 1024);
	if ((strncmp(recv_buffer, "reply", 5) != 0)){
		fprintf(stderr, "something else is being replied by the other client!!\n");
	}
}

