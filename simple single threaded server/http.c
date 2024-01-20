#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "http.h"
#include <stdlib.h>

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name) {
    int sock_fd_copy = dup(fd);
    
    if (sock_fd_copy == -1) {
        fprintf(stderr, "dup is issue\n");
        perror("dup");
        return -1;
    }
    FILE *socket_stream = fdopen(sock_fd_copy, "r");    
    if (socket_stream == NULL) {
        perror("fdopen");
        fprintf(stderr, "file is issue\n");
        close(sock_fd_copy);
        return -1;
    }
    
    // Disable the usual stdio buffering
    if (setvbuf(socket_stream, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        fprintf(stderr, "setvbuf is issue\n");
        fclose(socket_stream);
        return -1;
    }
    char buf[512]; 
    char buf2[512]; 
    memset(buf, 0, 512);
    memset(buf2, 0, 512);
    fgets(buf, 512, socket_stream);
    while(strcmp(fgets(buf2, 512, socket_stream), "\r\n") != 0){ // read in client whole request 
        memset(buf2, 0, 512);
    }

    char *string = malloc(100);
    string=strtok(buf," ");    //will take out GET fro Get /file.txt

    string=strtok(NULL, " ");   //will take out /file.txt from request 
    strcpy(resource_name,string);
    
    if (fclose(socket_stream) != 0) {       // close the stream
        fprintf(stderr, "close is issue\n");
        perror("fclose");   
        return -1;
    }

    return 0;
}

 int write_http_response(int fd, const char *resource_path) {
 
    char *headerfail = "HTTP/1.1 404 Not Found \r\n";  
    char *headerpass = "HTTP/1.1 200 OK \r\n";
    char *contentStat = "Content-Length: ";
    char *newline = "\r\n";



    int file = open(resource_path,O_RDWR,S_IRUSR|S_IWUSR);	//get the file for the client 
    if (file < 0) {
        if(write(fd, headerfail, strlen(headerfail)) == -1){return -1;} // if file isnt found then send back a 404 NOT FOUND back to the client 
        if(write(fd, contentStat, strlen(contentStat)) == -1){return -1;}
        char newzero[2] = "0";
        if(write(fd,newzero , strlen(newzero))== -1){return -1;}
       if(write(fd, newline, strlen(newline))== -1){return -1;}
        return 0;
        }
    
    if(write(fd, headerpass, strlen(headerpass))== -1){return -1;} // write the 200 OK if you found file 

    char *filetype=malloc(50);
    char save [100];
    strcpy(save, resource_path);
    char *string=malloc(100);

    string = strtok(save, "/"); // break the server_files/file.html
    string = strtok(NULL, " "); // now its /file.html
    string = strtok(string, "/"); // now its file.html
    string = strtok(NULL, "."); // STRING 
    filetype = strtok(NULL, " "); // .html
    
    char out[5] = ".";
    strcat(out, filetype);
    const char *type = get_mime_type(out);  // get back the type of the file
 

    char *contentType = "Content-Type: ";       // write back the content type to the client header
   if( write(fd, contentType, sizeof(contentType))== -1){return -1;}  
    if(write(fd, type, strlen(type))== -1){return -1;}
 
    if(write(fd, newline, strlen(newline))== -1){return -1;}
    

    struct stat stat_buf;
     if (stat(resource_path, &stat_buf) != 0) { // get the size of the file to return that # bacl to the client 
        perror("failed to stat");
        return -1;
    }
    
   // if(check==0){
    if(write(fd, contentStat, strlen(contentStat))== -1){return -1;}
    
    char *size = malloc(sizeof(int)*3);             // write the size 
    sprintf(size,"%lu",stat_buf.st_size);
    if(write(fd, size, strlen(size))== -1){return -1;}
    if(write(fd, newline, strlen(newline))== -1){return -1;}
    
    if(write(fd, newline, strlen(newline))== -1){return -1;}


    char buf[512];
    int nbytes=0;
    memset(buf, 0, 512);
    while((nbytes = read(file, buf, 512)) >0){      // read the file and write it back to the client 
		if(write(fd, buf, nbytes)== -1){return -1;}
        memset(buf, 0, 512);
        }
    if(write(fd, newline, strlen(newline))== -1){return -1;}
    close(file);
    return 0;

}
