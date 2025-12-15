// Yuval Anteby 212152896

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "BoundedBuffer.h"
#include <string.h> // for strcmp
#include <unistd.h>

#define FINISH_MSG "DONE"
#define MAX_MSG_LEN 256

typedef struct ForProducer {
    BoundedBuffer* buf;
    int messages;
} ForProducer;

typedef struct SizeAndMessages {
    int producerId;
    int numOfMessages;
    int queueSize;
} SizeAndMessages;

typedef struct ConfigData {
    int numOfProducers;
    SizeAndMessages *producersInfo;
    int coEditorQueueSize;
} ConfigData;

typedef struct ForDispatcher {
    BoundedBuffer** producers;
    BoundedBuffer* sportBuf;
    BoundedBuffer* newsBuf;
    BoundedBuffer* weatherBuf;
    int producersCount;
} ForDispatcher;

typedef struct AllBufs {
    BoundedBuffer* buf1;
    BoundedBuffer* buf2;
} AllBufs;

/**
 * Manages the screen output by removing messages from the buffer
 * and printing them to the screen until it receives the FINISH_MSG
 */
void* screenManagerFunc(void* arg) {
    BoundedBuffer* buf = (BoundedBuffer*)arg;
    int doneCount = 0;

    while (1) {
        char *message;
        message = removeFromBuffer(buf);

        if (strcmp(message, FINISH_MSG) == 0) {
            doneCount++;
            if (doneCount == 3) break;
            continue;
        }
        printf("%s\n", message);
        free(message);
    }
    
    printf("%s\n", FINISH_MSG);
    pthread_exit(EXIT_SUCCESS);
}

/**
 * Transfers messages from change buffer to the screen buffer
 */
void* coEditor(void* arg) {
    AllBufs* allBufs = (AllBufs*)arg;
    BoundedBuffer* changeBuf = allBufs->buf1;
    BoundedBuffer* toScreenBuf = allBufs->buf2;
    free(allBufs);
    
    // transfer messages from changeBuf to toScreenBuf
    while (1) {
        char *message;
        message = removeFromBuffer(changeBuf);
        usleep(100000);
        insertToBuffer(toScreenBuf, message);
        if (strcmp(message, FINISH_MSG) == 0) break;
    }

    pthread_exit(EXIT_SUCCESS);
}

/**
 * Generates messages and inserts them into the bounded buffer correctly
 */
void* producer(void* arg) {
    int sport = 0, news = 0, wheather = 0;
    ForProducer *forProd = (ForProducer*)arg;
    BoundedBuffer* buffer = forProd->buf;
    int mes = forProd->messages;
    free(forProd);

    for (int i = 0; i < mes; i++) {
        char* message = (char*)malloc(MAX_MSG_LEN * sizeof(char));
        if (message == NULL) {
            printf("Failed to allocate memory for message\n");
            pthread_exit(EXIT_FAILURE);
        }
        int r = rand() % 3;
        switch(r) {
            case 0:
                snprintf(message, MAX_MSG_LEN, "producer %d SPORTS %d", buffer->id, sport);
                sport++;
                break;
            case 1:
                snprintf(message, MAX_MSG_LEN, "producer %d NEWS %d", buffer->id, news);
                news++;
                break;
            case 2:
                snprintf(message, MAX_MSG_LEN, "producer %d WEATHER %d", buffer->id, wheather);
                wheather++;
                break;
            default:
                printf("Probably aint gonna happen I hope\n");
                break;   
        }
        insertToBuffer(buffer, message);
    }

    insertToBuffer(buffer, FINISH_MSG);
    pthread_exit(EXIT_SUCCESS);
}

/**
 * Moves messages from producers to the correct category buffers
*/
void* dispatcherFunc(void* arg) {
    ForDispatcher* allTheBufs = (ForDispatcher*)arg;
    BoundedBuffer** producersBufs = allTheBufs->producers;
    BoundedBuffer* sportBuf = allTheBufs->sportBuf;
    BoundedBuffer* newsBuf = allTheBufs->newsBuf;
    BoundedBuffer* weatherBuf = allTheBufs->weatherBuf;
    //int doneCount = 0;

    while (1) {
        // Assume done until we find an active one
        int allProducersDone = 1; 

        for (int i = 0; i < allTheBufs->producersCount; i++) {
            if (producersBufs[i]->isDone && isBufferEmpty(producersBufs[i])) continue; 
            
            // Found an active producer
            allProducersDone = 0; 
            
            char* message = tryRemoveFromBuffer(producersBufs[i]);
            
            if (message != NULL) {
                if (strcmp(message, FINISH_MSG) == 0) {
                    producersBufs[i]->isDone = 1;
                } else {
                    if (strstr(message, "SPORTS")) {
                        insertToBuffer(sportBuf, message);
                    } else if (strstr(message, "NEWS")) {
                        insertToBuffer(newsBuf, message);
                    } else if (strstr(message, "WEATHER")) {
                        insertToBuffer(weatherBuf, message);
                    }
                }
            }
        }
        
        // Only exit if all are effectively done
        if (allProducersDone) {
            insertToBuffer(sportBuf, FINISH_MSG);
            insertToBuffer(newsBuf, FINISH_MSG);
            insertToBuffer(weatherBuf, FINISH_MSG);
            return NULL;
        }
    }
}

/**
 * Parses the config file and fills the ConfigData struct
 */
void parseConfigFile(FILE *file, ConfigData* data) {
    char *line = NULL;
    ssize_t read; 
    size_t length = 0;
    
    // Count producers and get co editor queue size
    while ((read = getline(&line, &length, file)) != -1) {    
        // Skip empty lines
        if (read <= 1) continue;
        
        // Remove newline character
        if (line[read - 1] == '\n') line[read - 1] = '\0';
        
        // Count number of producers
        if (strstr(line, "PRODUCER")) data->numOfProducers++;
        
        // Get co editor queue size
        if(strstr(line, "Co-Editor")) {
            const char s[2] = " ";
            char *token = strtok(line, s);
            for (int i = 0; i < 4; i++) {
                token = strtok(NULL, s);
            }
            data->coEditorQueueSize = atoi(token);
        }
    }

    // Allocate memory for producers info
    fseek(file, 0, SEEK_SET);
    data->producersInfo = (SizeAndMessages*) malloc(sizeof(SizeAndMessages) * data->numOfProducers);
    if (data->producersInfo == NULL) {
        printf("Failed to allocate memory for producers info\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    int currentProducerIndex = 0;

    // Get producers info
    while ((read = getline(&line, &length, file)) != -1) {
        // Skip empty lines
        if (read <= 1) continue;
        line[read - 1] = '\0';
        
        // Get producer info
        if (strstr(line, "PRODUCER")) {
            const char s[2] = " ";
            char *token = strtok(line, s);
            token = strtok(NULL, s);
            int id = atoi(token);
            
            // Store ID but use sequential index
            data->producersInfo[currentProducerIndex].producerId = id;

            // if (index < 0 || index >= data->numOfProducers) {
            //     printf("\n\n%d\n", index);
            //     return;
            // }
            
            // Read number of messages
            read = getline(&line, &length, file);
            line[read - 1] = '\0';
            data->producersInfo[currentProducerIndex].numOfMessages = atoi(line);

            // Read queue size
            read = getline(&line, &length, file);
            line[read - 1] = '\0';
            token = strtok(line, " ");
            token = strtok(NULL, " "); // skip "queue"
            token = strtok(NULL, " "); // skip "size"
            token = strtok(NULL, " "); // skip "="
            data->producersInfo[currentProducerIndex].queueSize = atoi(token);        
            currentProducerIndex++; // Increment specific counter
}
    }

    fclose(file);
}

int main(int argc, char* argv[]) {
    // Check for config file argument
    if (argc < 2) {
        printf("Give me a file to parse\n");
        return 1;
    }

    srand(time(NULL)); // MOVED HERE

    // Open config file
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Failed to open file: %s\n", "shit");
        return 1;
    }

    // Parse config file
    ConfigData *dataOfConfig = (ConfigData*) malloc(sizeof(ConfigData));
    if (dataOfConfig == NULL) {
        printf("Failed to allocate memory\n");
        fclose(file);
        return 1;
    }

    // INitialize
    dataOfConfig->numOfProducers = 0;
    parseConfigFile(file, dataOfConfig);

    int producersCount = dataOfConfig->numOfProducers;
    BoundedBuffer** producersBufs = (BoundedBuffer**) malloc(sizeof(BoundedBuffer*) * producersCount);
    if (producersBufs == NULL) {
        printf("Failed to allocate memory\n");
        if (dataOfConfig->producersInfo != NULL) 
            free(dataOfConfig->producersInfo); 
        free(dataOfConfig);
        return 1;
    }
    for (int i = 0; i < producersCount; i++) {
        producersBufs[i] = 
        initBuffer(
            dataOfConfig->producersInfo[i].queueSize, 
            dataOfConfig->producersInfo[i].producerId
        );
    }
    pthread_t producers[producersCount];
    pthread_t dispatcher;
    BoundedBuffer* sportBuf,* newsBuf,* weatherBuf, *toScreenBuf;
    sportBuf = initBuffer(dataOfConfig->coEditorQueueSize, -1);
    newsBuf = initBuffer(dataOfConfig->coEditorQueueSize, -1);
    weatherBuf = initBuffer(dataOfConfig->coEditorQueueSize, -1);
    toScreenBuf = initBuffer(dataOfConfig->coEditorQueueSize, -1);

    // Start dispatcher thread
    ForDispatcher* forDispatcher = (ForDispatcher*) malloc(sizeof(ForDispatcher));
    if (forDispatcher == NULL) {
        printf("Failed to allocate memory\n");
        for(int i = 0; i < producersCount; i++) 
            destroyBuffer(producersBufs[i]);
        free(producersBufs);
        destroyBuffer(sportBuf);
        destroyBuffer(newsBuf);
        destroyBuffer(weatherBuf);
        destroyBuffer(toScreenBuf);
        if (dataOfConfig->producersInfo != NULL) 
            free(dataOfConfig->producersInfo); 
        free(dataOfConfig);
        return 1;
    }
    forDispatcher->newsBuf = newsBuf;
    forDispatcher->producers = producersBufs;
    forDispatcher->weatherBuf = weatherBuf;
    forDispatcher->sportBuf = sportBuf;  
    forDispatcher->producersCount = producersCount; 

    pthread_create(&dispatcher, NULL, dispatcherFunc, (void*)forDispatcher);

    for (int i = 0; i < producersCount; i++) {
        ForProducer *forProd = malloc(sizeof(ForProducer));
        if (forProd == NULL) {
            printf("Failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
        forProd->buf = producersBufs[i];
        forProd->messages = dataOfConfig->producersInfo[i].numOfMessages;
        pthread_create(&producers[i], NULL, producer, (void*)forProd);
    }
    pthread_t coEditors[3], screenManager;
    AllBufs* allbufs1 = (AllBufs*) malloc(sizeof(AllBufs));
    if (allbufs1 == NULL) {
        printf("Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    AllBufs* allbufs2 = (AllBufs*) malloc(sizeof(AllBufs));
    if (allbufs2 == NULL) {
        printf("Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    AllBufs* allbufs3 = (AllBufs*) malloc(sizeof(AllBufs));
    if (allbufs3 == NULL) {
        printf("Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    // Start co editor threads
    allbufs1->buf1 = sportBuf;
    allbufs1->buf2 = toScreenBuf;
    pthread_create(&coEditors[0], NULL, coEditor, (void*)allbufs1);
    allbufs2->buf1 = newsBuf;
    allbufs2->buf2 = toScreenBuf;
    pthread_create(&coEditors[1], NULL, coEditor, (void*)allbufs2);
    allbufs3->buf1 = weatherBuf;
    allbufs3->buf2 = toScreenBuf;
    pthread_create(&coEditors[2], NULL, coEditor, (void*)allbufs3);

    pthread_create(&screenManager, NULL, screenManagerFunc, (void*)toScreenBuf);
    pthread_join(screenManager, NULL);

    // memory cleanup
    for(int i = 0; i < producersCount; i++) 
        destroyBuffer(producersBufs[i]);

    free(producersBufs);

    destroyBuffer(sportBuf);
    destroyBuffer(newsBuf);
    destroyBuffer(weatherBuf);
    destroyBuffer(toScreenBuf);

    free(forDispatcher);
    if (dataOfConfig->producersInfo != NULL) 
        free(dataOfConfig->producersInfo); 
    free(dataOfConfig);

    return 0;
}