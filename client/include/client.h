/*
 * File name: client.h
 * Functionality: contains data structures and function prototypes 
 * used by the client.
 */
#include <stdio.h>
#include <pthread.h>

/*
 * Data structures
 */
struct files_hosted{
	char * filename;
	char * last_line;
	struct files_hosted * next;
};

struct request{
	//int socket_fd;
	char cli_id[30];
	long int t_stamp;
	//char filename[20];
	struct request * next;
};

/*
 * Global variables
 */
fd_set readfds;
struct request *head;
long int local_clock;
int max_clients; //initialized to 10
int cli_conn_fds[10];
int no_of_conn; // initialized to 0
int peer_cli_replies; // initialized to 0
int release_sent; // initialised to 1
int server_fds[3];
pthread_t ptid[10];
int num;
long int local_clock_send_timer;
int file_no;

/*
 * Function prototypes
 */
int initialize();
int server();
int peer_connect( char *ip_address);
int show_hosted_files();
void * peer_conn(void * arg);
int cli_server_connect( char *ip_addr);
void client_send_request(int sock_fd);
int is_local_request_head_of_queue();
void send_release(int sock_fd);
