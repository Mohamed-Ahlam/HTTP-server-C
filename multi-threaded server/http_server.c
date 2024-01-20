#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#include "connection_queue.h"
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define N_THREADS 5

//  typedef struct {
//   //  int socket;
//     connection_queue_t *queue;
// } thread_args_t;

int keep_going = 1;
const char *serve_dir;

void handle_sigint(int signo) {
    keep_going = 0;
}
int doIt = 0;

void *consumer_thread_func(void *arg){
    connection_queue_t *que = (connection_queue_t *) arg;

    while(doIt == 0){
    int client_fd;
    
    if((client_fd=connection_dequeue(que)) == -1){ // get the client from the que
        close(client_fd);
        doIt = 1;
        connection_queue_shutdown(que);
    }

    char *filename=malloc(sizeof(BUFSIZE));
        
     if (read_http_request(client_fd, filename)== -1){  // read the request from the client and get the filename and save it to this filename
            fprintf(stderr, "returned from read");
            connection_queue_shutdown(que);  
            close(client_fd);
        }
        char dir[100];
        const char *serve_dir = "./server_files";
        strcpy(dir, serve_dir);
        strcat(dir, filename);     //should == to server_files/file.html

        if(write_http_response(client_fd, dir)==-1){        // use the filename we got from client to find it in the server_files and send it back to the client
            fprintf(stderr, "returned from write");
            close(client_fd);  
            connection_queue_shutdown(que);  
          
        } 
        
        close(client_fd);  // close the dupped client 
       
    }
    return NULL;
}

int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
        if (argc != 3) {
            printf("Usage: %s <directory> <port>\n", argv[0]);
            return 1;
        }
        // Uncomment the lines below to use these definitions:
        serve_dir = argv[1];
        const char *port = argv[2];

        // TODO Implement the rest of this function
        struct sigaction sigact;
        sigact.sa_handler = handle_sigint;
        sigfillset(&sigact.sa_mask);
        sigact.sa_flags = 0; // Note the lack of SA_RESTART
        if (sigaction(SIGINT, &sigact, NULL) == -1) {
            perror("sigaction");
            return 1;
        }
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE; // We'll be acting as a server
        struct addrinfo *server;//do we need this
        
        //getaddrinfo
        int ret_val = getaddrinfo(NULL, port, &hints, &server);
        if (ret_val != 0) {
            fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
            return 1;
        }
            // Initialize socket file descriptor
        int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
        if (sock_fd == -1) {
            perror("socket");
            freeaddrinfo(server);
            return 1;
        }
        // Bind socket to receive at a specific port
        if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
            perror("bind");
            freeaddrinfo(server);
            close(sock_fd);
            return 1;
        }
        freeaddrinfo(server);
        // Designate socket as a server socket
        if (listen(sock_fd, LISTEN_QUEUE_LEN) == -1) {
            perror("listen");
            close(sock_fd);
            return 1;
        }
    sigset_t oset;
    sigset_t nset;
    sigemptyset(&nset);
    sigfillset(&nset);
    sigprocmask(SIG_BLOCK, &nset, &oset);  // block all signals and later unblock sigint so main can accept it 

    connection_queue_t queue;       
    if (connection_queue_init(&queue) != 0) {       // make an empty que and set it up
        fprintf(stderr, "Failed to initialize queue\n");
        return 1;
    }

    pthread_t consumer_threads[N_THREADS]; 
    int result;
   
    for (int i = 0; i < N_THREADS; i++) {
        if ((result=pthread_create(consumer_threads + i, NULL, consumer_thread_func, &queue)) != 0) { 
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            return 1;
        }
    }

    sigprocmask(SIG_SETMASK, &oset, NULL);

    while (keep_going != 0) { // accept loop 
        int client_fd = accept(sock_fd, NULL, NULL); // accept a client socket 
        if(client_fd == -1){
            connection_queue_shutdown(&queue);// do the steps for a shut down 
            return 1;
            }

        if (connection_enqueue(&queue, client_fd) == -1) {      // put this client into the que so a thread can take it and listen to its request and send back the file it wants
            return 1; 
            }           
    }
 for (int i = 0; i < N_THREADS; i++) {//waiting for all consumer threads  to finish up with their clients
        if ((result = pthread_join(consumer_threads[i], NULL)) != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
            return 1;
        }
    } 

     if (close(sock_fd) == -1) {
        perror("close");
        return 1;
    }
    return 0;
}
