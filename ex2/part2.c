// Yuval Anteby 212152896

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define LOCK_FILE "lockfile.lock"

/**
 * writes a message to STDOUT
 * @param message the text to print to STDOUT
 * @param count the number of times to print it
 */
void write_message(const char *message, int count) {
    for (int i = 0; i < count; i++) {
        printf("%s\n", message);
        usleep((rand() % 100) * 1000); // Random delay between 0 and 99 milliseconds
    }
}

/**
 * atomic lock acquisition using O_EXCL
 */ 
void acquire_lock() {
    int fd;
    // O_EXCL makes sure that open() fails if the file already exists
    while ((fd = open(LOCK_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
        if (errno == EEXIST) {
            usleep(10000); // 10ms interval
        } else {
            perror("error acquiring lock");
            exit(1);
        }
    }
    // If reached here, successfully created the file and hold the lock
    close(fd);
}

/**
 * release lock by unlinking the file (which is like deleting the file)
 */
void release_lock() {
    if (unlink(LOCK_FILE) == -1) {
        perror("Error releasing lock");
        exit(1);
    }
}

/**
 * Child logic
 */
void child_process(const char *message, int count) {
    // Seed randomness using PID to ensure different seeds per child
    srand(time(NULL) ^ (getpid() << 16));
    
    // --- CS ENTRY ---
    acquire_lock();
    
    // Writing to STDOUT
    write_message(message, count);
    
    // --- CS EXIT ---
    release_lock();
    
    // sucess, exit the child process
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <msg1> <msg2> ... <count>\n", argv[0]);
        return 1;
    }

    // Last argument is the count
    int count = atoi(argv[argc - 1]);
    if (count <= 0) {
        fprintf(stderr, "Error: count must be positive\n");
        return 1;
    }

    // Number of message arguments
    int num_children = argc - 2; 

    // to prevent an oopsi moment when i rerun it
    unlink(LOCK_FILE);
    
    // Forking Loop to get all possible child processes texts
    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            return 1;
        }
        
        if (pid == 0) {
            // argv[i+1] is the specific message for this child
            child_process(argv[i + 1], count);
        }
    }
    
    // Parent waits for all children
    for (int i = 0; i < num_children; i++) {
        if (wait(NULL) == -1) {
            perror("wait error");
        }
    }
    
    // again to prevent an oopsi moment 
    unlink(LOCK_FILE);
    
    return 0;
}