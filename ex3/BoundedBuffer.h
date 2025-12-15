// Yuval Anteby 212152896

#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define FINISH_MSG "DONE"

/**
 * Struct for a bounded buffer of the  producer consumer
*/
typedef struct BoundedBuffer {
    char **buffer;
    int size;
    int head;
    int tail;
    int id;
    int isDone;
    pthread_mutex_t lock;
    sem_t writeSemaphore;
    sem_t readSemaphore;
} BoundedBuffer;

// Initializes the buffer
BoundedBuffer* initBuffer(int bufferSize, int id);

// Inserts a new message to the buffer
int insertToBuffer(BoundedBuffer *bb, char *msg);

// Removes a message from the buffer
char* removeFromBuffer(BoundedBuffer* bb);

char* tryRemoveFromBuffer(BoundedBuffer* bb);

// Checks if the buffer is empty
int isBufferEmpty(BoundedBuffer* bb);

#endif
