#include <stdio.h>
#include <string.h>
#include "connection_queue.h"

int connection_queue_init(connection_queue_t *queue) { // sets up everything in que
    queue->length=0;
    queue->write_idx=0; 
    queue->read_idx=0;     //index position of queue
    queue->shutdown=0;  //will turn into one if it is shut down

    int result;
    if ((result = pthread_mutex_init(&queue->lock, NULL)) != 0) {//correct use of synchronization primitives
        fprintf(stderr, "pthread_mutex_init: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_init(&queue->queue_full, NULL)) != 0) {
        fprintf(stderr, "pthread_cond_init: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_init(&queue->queue_empty, NULL)) != 0) {
        fprintf(stderr, "pthread_cond_init: %s\n", strerror(result));
        return -1;
    }


    return 0;
}

int connection_enqueue(connection_queue_t *queue, int connection_fd) {
    // TODO Not yet implemented
  //  fprintf(stderr, "we are in ENQ\n");
    
    int result=0;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {  //correct use of synchronization primitives
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    
    while (queue->length == CAPACITY) {  // if its at capacity then wait until it isnt 
         if ((result = pthread_cond_wait(&queue->queue_full, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            return -1;
        }

    }
    if(queue->shutdown==1){//if shutdown then exit 
        return -1;
    }
    if(queue->write_idx>4){ // circle buffer so start back at 0
        queue->write_idx=0;
    }
    queue->client_fds[queue->write_idx]=connection_fd;
    queue->length++;
    queue->write_idx++;

    if ((result = pthread_cond_signal(&queue->queue_empty)) != 0) {
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }


    return 0;
}

int connection_dequeue(connection_queue_t *queue) {
    int result=0;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;}    
    while (queue->length == 0) {
         if ((result = pthread_cond_wait(&queue->queue_empty, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
    }

  if(queue->shutdown==1){//make sure it is not shutdown
        return -1;
}
    int connection_fd;
    if (queue->read_idx > 4){
        queue->read_idx=0;
    }
    connection_fd=queue->client_fds[queue->read_idx];
    queue->length--;
    queue->read_idx++;//read and write are suppose to be circular index to the queue
    if ((result = pthread_cond_signal(&queue->queue_full)) != 0) {
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    return connection_fd;//Returns the removed socket file descriptor
}

int connection_queue_shutdown(connection_queue_t *queue) {      
    int result=0;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;} 
        
    queue->shutdown=1;

    if ((result = pthread_cond_broadcast(&queue->queue_empty)) != 0) {
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        return -1; }

    if ((result = pthread_cond_broadcast(&queue->queue_full)) != 0) {
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        return -1; }

    //All threads currently blocked on an enqueue or dequeue operation are unblocked and an error is returned to them.
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }
    return 0;
}

int connection_queue_free(connection_queue_t *queue) {
    int result;
    if ((result = pthread_mutex_destroy(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(result));
        return -1;} 
    if ((result = pthread_cond_destroy(&queue->queue_full)) != 0) {
        fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(result));
        return -1;} 
    if ((result = pthread_cond_destroy(&queue->queue_empty)) != 0) {
        fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(result));
        return -1;} 

    return 0;
}
