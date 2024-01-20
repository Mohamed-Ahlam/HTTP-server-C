#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    const char *serve_dir = argv[1];
    const char *port = argv[2];
    //catch sigact
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
      while (keep_going != 0) { // accept loop 
    
        printf("Waiting for client to connect\n");
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno != EINTR) {
                perror("accept");
                close(sock_fd);
                return 1;
            } else {        // accept failed due to user doing SIGINT
                break;      
            }
        }
     
        printf("New client connected\n");
        char *filename=malloc(sizeof(BUFSIZE));
     
        if (read_http_request(client_fd, filename)==-1){    // get the clients request and get the file name they want
            fprintf(stderr, "returned from read");
            close(sock_fd);
            return 1;
        }
        char dir[200] = "./";
        strcat(dir, serve_dir);
        strcat(dir, filename);     //should == to ./server_files/file.html

        if(write_http_response(client_fd, dir)==-1){    // send back the file contents to the file 
            fprintf(stderr, "returned from write");
            close(sock_fd);
            return 1;
        } 
        if (close(client_fd) != 0) {     // close the dupped client 
        perror("close");
        return -1;
        } 
       // leave loop after sending clients file 

        }
        printf("Client disconnected\n");
            // Don't forget cleanup - reached even if we had SIGINT
    if (close(sock_fd) == -1) {
        perror("close");
        return 1;
    }
    return 0;
}
    





