// Yuval Anteby 212152896

#include "BoundedBuffer.h"

/**
 * Initializes the buffer
 * @param bufferSize size of the buffer
 * @param id id of the buffer
 * @return pointer to the initialized buffer
*/
BoundedBuffer* initBuffer(int bufferSize, int id) {
    BoundedBuffer *bb = (BoundedBuffer*) malloc(sizeof(BoundedBuffer));
    bb->size = bufferSize;
    bb->buffer = (char**) malloc(sizeof(char*) * bufferSize);
    bb->head = 0;
    bb->tail = 0;
    bb->id = id;
    bb->isDone = 0;
    pthread_mutex_init(&bb->lock, NULL);
    sem_init(&bb->readSemaphore, 1, 0);
    sem_init(&bb->writeSemaphore, 1, bufferSize);
    return bb; 
}

/**
 * Inserts a new message to the buffer
 * @param bb pointer to the buffer
 * @param msg message to insert
 * @return 0 on success
*/
int insertToBuffer(BoundedBuffer *bb, char *msg) {
    // lock the buffer using semaphore and mutex
    sem_wait(&bb->writeSemaphore);
    pthread_mutex_lock(&bb->lock);
    // Critical section is inserting the message
    bb->buffer[bb->tail] = msg;
    bb->tail = (bb->tail + 1) % bb->size;
    // unlock
    pthread_mutex_unlock(&bb->lock);
    sem_post(&bb->readSemaphore);
    return 0;
}

/**
 * Removes a message from the buffer
 * @param bb pointer to the buffer
 * @return the removed message
*/
char* removeFromBuffer(BoundedBuffer* bb) {
    // lock the buffer using semaphore and mutex
    sem_wait(&bb->readSemaphore);
    pthread_mutex_lock(&bb->lock);
    // Critical section is removing the message
    char *msgToReturn = bb->buffer[bb->head];
    bb->head = (bb->head + 1) % bb->size;
    // unlock
    pthread_mutex_unlock(&bb->lock);
    sem_post(&bb->writeSemaphore);
    // return the message that was removed
    return msgToReturn;
}

/**
 * Checks if the buffer is empty
 * @param bb pointer to the buffer
 * @return 1 if the buffer is empty, 0 otherwise
*/
int isBufferEmpty(BoundedBuffer* bb) {
    if (bb->head == bb->tail) 
        return 1;
    return 0;
}